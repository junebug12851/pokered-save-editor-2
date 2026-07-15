/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * @file mapmodel.cpp
 * @brief Implementation of MapModel -- the loaded map's image + geometry.
 *        See mapmodel.h.
 */

#include <QStringList>
#include <algorithm>

#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/tiletraitsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrysign.h>
#include <pse-db/entries/mapdbentrytext.h>
#include <pse-db/entries/mapdbentrysprite.h>
#include <pse-db/itemsdb.h>
#include <pse-db/spriteSet.h>
#include <pse-db/sprites.h>
#include <pse-db/trainers.h>
#include <QColor>
#include <QMap>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/area/areasign.h>
#include <pse-savefile/expanded/fragments/spritedata.h>
#include <pse-savefile/expanded/fragments/signdata.h>
#include <pse-savefile/expanded/fragments/warpdata.h>
#include <pse-savefile/expanded/fragments/mapconndata.h>
#include <pse-db/entries/mapdbentryconnect.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-db/entries/mapdbentrywarpin.h>
#include <pse-db/entries/mapdbentrywarpout.h>
#include <pse-common/random.h>

#include "./mapmodel.h"
#include "../engine/mapengine.h"

namespace {
/// `SPRITE_RED` -- the player's own picture. It is loaded on every map, ahead of the sprite set.
/// (constants/sprite_constants.asm)
constexpr int SpritePlayerPicture = 1;

/// The DB entry for a tileset id. Defined further down (next to the other DB helpers); declared here
/// because the map/tileset/blockset setters at the top of the file need it.
TilesetDBEntry* canonAt(int tilesetInd);

/// The three tile values the SAVE owns (grass, counters, the border block). Defined next to the
/// other engine helpers; declared here because "zoom to a random door" needs it further up.
MapEngine::SaveTiles saveTilesOf(AreaTileset* tileset);

/// The field kit. Defined further down (next to npcFields, where it was born); declared here
/// because the WARP fields sit above it in the file and use the same kit -- which is the point of
/// having a kit.
QVariantMap field(const QString& group, const QString& key, const QString& label,
                  const QString& blurb, int value, int min, int max,
                  const QString& kind = QStringLiteral("byte"),
                  const QVariantList& options = {},
                  bool scratch = false);
QVariantMap option(int value, const QString& name, bool hack = false);
} // namespace

MapModel::MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset, AreaGeneral* general,
                   AreaLoadedSprites* sprites, AreaSprites* npcs, AreaWarps* warps,
                   WorldGeneral* world, AreaSign* signs)
  : sprites(sprites), npcs(npcs), warps(warps), signsData(signs), world(world),
    map(map), player(player), tileset(tileset), general(general)
{
  // The doors get their own signal for exactly the reason the cast does: `changed()` re-renders the
  // whole map image, and a warp chip sliding one tile does not change a pixel of the map.
  if (warps != nullptr)
    connect(warps, &AreaWarps::warpsChanged, this, &MapModel::warpsChanged);

  // Signs, same story: a placard sliding one tile does not change a pixel of the map, so it rides its
  // own signal instead of the map-wide `changed()`. See MapModel::signsChanged.
  if (signsData != nullptr)
    connect(signsData, &AreaSign::signsChanged, this, &MapModel::signsChanged);

  // `wLastMap` is what every `$FF` ("back outside") door resolves through, so moving it re-labels
  // a dozen chips at once -- which is the whole reason it sits in the toolbar and not in a panel.
  if (world != nullptr) {
    connect(world, &WorldGeneral::lastMapChanged, this, &MapModel::warpsChanged);
    connect(world, &WorldGeneral::lastBlackoutMapChanged, this, &MapModel::warpsChanged);
  }

  // ⚠️ THE CAST GETS ITS OWN SIGNAL, AND THIS IS THE WHOLE REASON THE WALK RAN AT 4 FPS.
  //
  // This used to be `connect(..., &MapModel::changed)`. But `changed()` is wired to `sourceChanged()`
  // (see the constructor's tail), and `source` carries the map's render URL -- so every emission
  // **re-renders the entire map image**: expand every block into tiles, every tile into pixels, for
  // the whole of Route 17 if that is where you are.
  //
  // The walk simulation moves somebody **~60 times a second**. It was therefore re-rendering the
  // whole map ~60 times a second, to move one 16x16 sprite. Twilight: *"the framerate plummets."*
  //
  // A sprite moving does not change one pixel of the MAP. It gets `castChanged()` -- which the
  // canvas's sprite layer listens to and nothing else does.
  if (npcs != nullptr)
    connect(npcs, &AreaSprites::spritesChanged, this, &MapModel::castChanged);

  // The sprite-set cache is part of "where you are", so a change to it redraws the panel that shows
  // it. (It changes nothing about the map itself -- see notes/reference/sprite-sets.md.)
  if (sprites != nullptr) {
    connect(sprites, &AreaLoadedSprites::loadedSpritesChanged, this, &MapModel::changed);
    connect(sprites, &AreaLoadedSprites::loadedSetIdChanged, this, &MapModel::changed);
  }

  // Everything this model publishes is derived from these few values, so one "changed"
  // signal for the lot is honest -- any of them moving redraws the map.
  connect(map, &AreaMap::curMapChanged, this, &MapModel::changed);
  connect(tileset, &AreaTileset::currentChanged, this, &MapModel::changed);
  connect(player, &AreaPlayer::xCoordChanged, this, &MapModel::changed);
  connect(player, &AreaPlayer::yCoordChanged, this, &MapModel::changed);
  connect(general, &AreaGeneral::contrastChanged, this, &MapModel::changed);
  connect(player, &AreaPlayer::playerCurDirChanged, this, &MapModel::changed);

  // The overlay is drawn from the SAVE's grass and counter tiles, so editing either has to
  // redraw it -- otherwise the map would go on showing grass where the grass no longer is.
  connect(tileset, &AreaTileset::typeChanged, this, &MapModel::changed);
  connect(tileset, &AreaTileset::grassTileChanged, this, &MapModel::overlayChanged);
  connect(tileset, &AreaTileset::talkingOverTilesChanged, this, &MapModel::overlayChanged);
  connect(tileset, &AreaTileset::currentChanged, this, &MapModel::overlayChanged);
  connect(map, &AreaMap::curMapChanged, this, &MapModel::overlayChanged);
  connect(map, &AreaMap::outOfBoundsBlockChanged, this, &MapModel::changed);

  // A selection is a block INDEX. Load another map and that index may not exist any more --
  // so re-check it rather than let the inspector point at a block that isn't there.
  connect(map, &AreaMap::curMapChanged, this, &MapModel::revalidateSelection);
  connect(tileset, &AreaTileset::currentChanged, this, &MapModel::selectionChanged);

  // The image's URL changes for TWO reasons -- the map/tileset/palette changed, or the animation
  // moved on a frame. `changed()` covers the first; setFrame() emits the second directly. Keeping
  // them separate is what stops a 3 Hz animation from re-deriving the whole overworld buffer for
  // every listener of `changed()`, three times a second, forever.
  connect(this, &MapModel::changed, this, &MapModel::sourceChanged);
}

int MapModel::mapInd() const     { return map->curMap; }
int MapModel::tilesetInd() const { return tileset->current; }
int MapModel::playerX() const    { return player->xCoord; }
int MapModel::playerY() const    { return player->yCoord; }

bool MapModel::valid() const
{
  return MapEngine::sourceMap(mapInd()) != nullptr
      && BlocksDB::inst()->hasTileset(tilesetInd())
      && blocksWide() > 0 && blocksHigh() > 0;
}

QString MapModel::source() const
{
  if (!valid())
    return QString();

  // The FRAME, the contrast and the animation byte all ride in the URL, so the image is rebuilt
  // whenever any of them changes -- no invalidation logic to get wrong. Frame 0 is the still map
  // (what every screenshot and every test renders); MapClock advances it at the console's own
  // cadence. See notes/reference/map-animation.md.
  //
  // ⚠️ The colour-palette GENERATION rides here too. The GBC/SGB/custom filter is a global on
  // MapEngine, NOT in this URL's other fields -- so without a generation counter the provider would
  // serve a stale cached image after a palette change. Bump-on-change, ride-in-URL: the same trick
  // the frame uses.
  return "image://map/" + QString::number(mapInd())
       + "/" + QString::number(tilesetInd())
       + "/" + QString::number(frame())
       + "/" + QString::number(contrast())
       + "/" + QString::number(tileAnim())
       + "/" + QString::number(blocksetInd())    // whose BLOCKS -- the save's own second pointer
       + "/" + QString::number(borderBlock())     // what fills the ring -- the save's own byte
       + "/" + QString::number(MapEngine::paletteGeneration());   // the colour filter
}

QVariantList MapModel::connectionList() const
{
  QVariantList out;
  if (!valid())
    return out;

  // MapDBEntryConnect::ConnectDir order -- NORTH, SOUTH, EAST, WEST. (Not the order you'd guess, and
  // getting it wrong would label every strip with its neighbour's direction.)
  static const char* dirName[] = { "North", "South", "East", "West" };

  for (const MapEngine::Strip& s : MapEngine::connectionStrips(mapInd())) {
    QVariantMap m;
    m["dir"] = s.dir;
    m["dirName"] = (s.dir >= 0 && s.dir < 4) ? QObject::tr(dirName[s.dir]) : QString();
    m["name"] = s.name;

    // Buffer PIXELS, like every other rectangle this model publishes -- QML multiplies by the zoom
    // and does no arithmetic of its own.
    m["x"] = s.bx * MapEngine::blockPx;
    m["y"] = s.by * MapEngine::blockPx;
    m["w"] = s.cols * MapEngine::blockPx;
    m["h"] = s.rows * MapEngine::blockPx;

    m["blocks"] = QStringLiteral("%1 × %2").arg(s.cols).arg(s.rows);

    out.append(m);
  }

  return out;
}

// ── The connecting routes (edge connections) — EDITING ────────────────────────────────────────────
//
// A connection is neighbour + one signed offset; the other nine bytes are MapEngine::connectionBytes.
// Read notes/reference/map-connections.md. Every write here touches only the flag bit + the one 11-byte
// slot (tst_connections byte-diffs the whole 32 KB save and demands exactly that).

namespace {

/// Write the eight derived save bytes onto a live connection (each with its Changed()). Sets the
/// neighbour id too when the derive is valid; on an invalid (sizeless/glitch) derive it leaves the
/// strip fields alone — the caller has already set mapPtr, and the raw path (break-sync) does the rest.
void applyConnBytes(MapConnData* c, const MapEngine::ConnBytes& b)
{
  if (c == nullptr || !b.valid)
    return;

  c->mapPtr = b.mapPtr;         c->mapPtrChanged();
  c->stripSrc = b.stripSrc;     c->stripSrcChanged();
  c->stripDst = b.stripDst;     c->stripDstChanged();
  c->stripWidth = b.stripWidth; c->stripWidthChanged();
  c->width = b.width;           c->widthChanged();
  c->yAlign = b.yAlign;         c->yAlignChanged();
  c->xAlign = b.xAlign;         c->xAlignChanged();
  c->viewPtr = b.viewPtr;       c->viewPtrChanged();
}

} // namespace

bool MapModel::connectionExists(int dir) const
{
  return map != nullptr && map->connections.contains((var8)dir);
}

int MapModel::connectionRoomLeft() const
{
  return (map == nullptr) ? 0 : std::max(0, 4 - static_cast<int>(map->connections.size()));
}

bool MapModel::connectionsEdited() const
{
  return connectionsWereEdited;
}

int MapModel::connectionOffsetOf(int dir) const
{
  if (!connectionExists(dir))
    return 0;

  MapConnData* c = map->connections.value((var8)dir);
  return MapEngine::connectionOffsetFrom(dir, c->xAlign, c->yAlign);
}

bool MapModel::connectionSynced(int dir) const
{
  if (!connectionExists(dir))
    return false;

  MapConnData* c = map->connections.value((var8)dir);
  MapDBEntry* from = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  MapDBEntry* to   = MapsDB::inst()->getIndAt(QString::number(c->mapPtr));

  const MapEngine::ConnBytes b =
      MapEngine::connectionBytes(from, dir, to, connectionOffsetOf(dir));
  if (!b.valid)
    return false;   // can't derive a sizeless neighbour, so it can't be "in sync"

  // Compare at the width the SAVE actually stores each field in: words for the pointers, bytes for
  // the rest (the derive's alignment values can be negative; the save masks them).
  auto w  = [](int v) { return v & 0xFFFF; };
  auto by = [](int v) { return v & 0xFF; };

  return by(c->mapPtr)   == by(b.mapPtr)
      && w(c->stripSrc)  == w(b.stripSrc)
      && w(c->stripDst)  == w(b.stripDst)
      && by(c->stripWidth) == by(b.stripWidth)
      && by(c->width)    == by(b.width)
      && by(c->yAlign)   == by(b.yAlign)
      && by(c->xAlign)   == by(b.xAlign)
      && w(c->viewPtr)   == w(b.viewPtr);
}

bool MapModel::addConnection(int dir, int toMapInd)
{
  if (map == nullptr || dir < 0 || dir > 3 || map->connections.contains((var8)dir))
    return false;

  map->connNew(dir);   // inserts a blank slot + sets the flag bit on save
  MapConnData* c = map->connections.value((var8)dir);
  if (c == nullptr)
    return false;

  // The neighbour first, so an un-derivable (glitch) map still records who it points at.
  c->mapPtr = toMapInd;
  c->mapPtrChanged();

  MapDBEntry* from = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  MapDBEntry* to   = MapsDB::inst()->getIndAt(QString::number(toMapInd));
  applyConnBytes(c, MapEngine::connectionBytes(from, dir, to, 0));

  connectionsWereEdited = true;
  map->connectionsChanged();
  changed();   // the ring bleeds a new map's edge — the map image really does change
  return true;
}

void MapModel::removeConnection(int dir)
{
  if (!connectionExists(dir))
    return;

  map->connRemove(dir);   // clears the flag bit; the stale 11 bytes are left as they lie
  connectionsWereEdited = true;
  changed();
}

void MapModel::setConnectionMap(int dir, int toMapInd)
{
  if (!connectionExists(dir))
    return;

  const int off = connectionOffsetOf(dir);
  MapConnData* c = map->connections.value((var8)dir);
  c->mapPtr = toMapInd;
  c->mapPtrChanged();

  MapDBEntry* from = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  MapDBEntry* to   = MapsDB::inst()->getIndAt(QString::number(toMapInd));
  applyConnBytes(c, MapEngine::connectionBytes(from, dir, to, off));

  connectionsWereEdited = true;
  map->connectionsChanged();
  changed();
}

void MapModel::setConnectionOffset(int dir, int offset)
{
  if (!connectionExists(dir))
    return;

  MapConnData* c = map->connections.value((var8)dir);
  MapDBEntry* from = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  MapDBEntry* to   = MapsDB::inst()->getIndAt(QString::number(c->mapPtr));

  const MapEngine::ConnBytes b = MapEngine::connectionBytes(from, dir, to, offset);
  if (!b.valid)
    return;   // sizeless neighbour: the offset knob doesn't apply, only the raw path does

  applyConnBytes(c, b);
  connectionsWereEdited = true;
  map->connectionsChanged();
  changed();
}

void MapModel::rehomeConnection(int fromDir, int toDir)
{
  if (fromDir == toDir || !connectionExists(fromDir) || connectionExists(toDir)
      || toDir < 0 || toDir > 3)
    return;

  MapConnData* c = map->connections.value((var8)fromDir);
  const int toMap = c->mapPtr;
  const int off   = connectionOffsetOf(fromDir);

  map->connRemove(fromDir);
  addConnection(toDir, toMap);        // derives for the new edge; sets edited + emits
  setConnectionOffset(toDir, off);    // re-applies the offset on the new edge
}

QVariantList MapModel::connectionEditList() const
{
  QVariantList out;
  if (map == nullptr)
    return out;

  // MapDBEntryConnect::ConnectDir order: NORTH 0, SOUTH 1, EAST 2, WEST 3 — the same order the
  // save's flag byte and connectionList() use. Getting it wrong mislabels every edge.
  static const char* dirName[] = { "North", "South", "East", "West" };

  for (int dir = 0; dir < 4; ++dir) {
    QVariantMap m;
    m["dir"] = dir;
    m["dirName"] = QObject::tr(dirName[dir]);
    // Real headers run −27…+27; a little headroom for the slider, hack values go via break-sync.
    m["offsetMin"] = -32;
    m["offsetMax"] = 32;

    const bool ex = connectionExists(dir);
    m["exists"] = ex;

    if (ex) {
      MapConnData* c = map->connections.value((var8)dir);
      MapDBEntry* to = MapsDB::inst()->getIndAt(QString::number(c->mapPtr));
      const int toW = (to == nullptr) ? 0 : to->getWidth();
      const int toH = (to == nullptr) ? 0 : to->getHeight();
      m["toMap"] = c->mapPtr;
      m["toName"] = (to == nullptr) ? QObject::tr("Map %1").arg(c->mapPtr) : to->bestName();
      m["offset"] = connectionOffsetOf(dir);
      m["synced"] = connectionSynced(dir);
      m["toW"] = toW;
      m["toH"] = toH;

      // The landmark offsets a drag snaps to (Twilight, 2026-07-15): flush at 0, centred, and the
      // far edges flush. N/S snap along width; E/W along height. Computed from the two maps' sizes,
      // deduped, and named so the context bar can say which one you are on.
      const bool ns = (dir == MapDBEntryConnect::ConnectDir::NORTH ||
                       dir == MapDBEntryConnect::ConnectDir::SOUTH);
      const int cur = ns ? map->width : map->height;
      const int nb  = ns ? toW : toH;
      QVariantList snaps;
      auto addSnap = [&](int off, const QString& name) {
        if (to == nullptr) return;
        for (const QVariant& s : snaps)          // dedupe
          if (s.toMap().value("offset").toInt() == off) return;
        QVariantMap sm; sm["offset"] = off; sm["name"] = name;
        snaps.append(sm);
      };
      addSnap(0, QObject::tr("Corner-aligned"));
      if (nb > 0) {
        addSnap((cur - nb) / 2, QObject::tr("Centred"));
        addSnap(cur - nb, QObject::tr("Flush"));
      }
      m["snaps"] = snaps;
    } else {
      m["toMap"] = -1;
      m["toName"] = QString();
      m["offset"] = 0;
      m["synced"] = false;
      m["toW"] = 0;
      m["toH"] = 0;
      m["snaps"] = QVariantList();
    }

    out.append(m);
  }

  return out;
}

// ── The sprite set ("the cached sprites") ─────────────────────────────────────
//
// 12 bytes: eleven sprite pictures and the id of the set they came from. Everything about what they
// mean -- and the fact that the game recomputes them the instant it loads your save -- is in
// notes/reference/sprite-sets.md.

namespace {

/// The ten sets, by the names the game's own source gives them (constants/sprite_set_constants.asm).
/// v1 called these "Static List 1..10", which told nobody anything.
QString spriteSetNameOf(int id)
{
  switch (id) {
    case 0:  return QObject::tr("None");
    case 1:  return QObject::tr("Pallet & Viridian");
    case 2:  return QObject::tr("Pewter & Cerulean");
    case 3:  return QObject::tr("Lavender");
    case 4:  return QObject::tr("Vermilion");
    case 5:  return QObject::tr("Celadon");
    case 6:  return QObject::tr("Indigo");
    case 7:  return QObject::tr("Saffron");
    case 8:  return QObject::tr("Silence Bridge");
    case 9:  return QObject::tr("Cycling Road");
    case 10: return QObject::tr("Fuchsia");
    default: break;
  }

  // $F1-$FC are the SPLIT ids -- real values in the game's map table, but never in wSpriteSetID:
  // the console resolves them to one of the ten above before it stores anything.
  if (id >= 0xF1 && id <= 0xFC)
    return QObject::tr("Split set %1 — the game never stores this").arg(id);

  return QObject::tr("Set %1 — no set in the game").arg(id);
}

SpriteSetDBEntry* setAt(int id)
{
  for (auto* el : SpriteSetDB::inst()->getStore())
    if (el->ind == id)
      return el;

  return nullptr;
}

} // namespace

int MapModel::spriteSetId() const
{
  return (sprites == nullptr) ? 0 : sprites->loadedSetId;
}

QString MapModel::spriteSetName() const
{
  return spriteSetNameOf(spriteSetId());
}

int MapModel::mapSpriteSetId() const
{
  // What InitOutsideMapSprites would put there: MapSpriteSets[curMap], resolved through the split
  // table if it is one of the split ids. Indoor maps get nothing -- the routine returns before it
  // touches anything at all.
  auto* entry = MapEngine::sourceMap(mapInd());
  if (entry == nullptr)
    return 0;

  auto* set = entry->getToSpriteSet();
  if (set == nullptr)
    return 0;

  const auto* resolved = set->getResolvedSet(static_cast<var8>(playerX()),
                                             static_cast<var8>(playerY()));
  return (resolved == nullptr) ? 0 : resolved->ind;
}

QString MapModel::mapSpriteSetName() const
{
  return spriteSetNameOf(mapSpriteSetId());
}

bool MapModel::mapHasSpriteSet() const
{
  return mapSpriteSetId() != 0;
}

bool MapModel::spriteSetMatchesMap() const
{
  // Only meaningful where the game HAS a set for this map. Inside a building the cache is simply the
  // last outdoor set you were in, and that is not a mismatch -- it is what a console holds too.
  return !mapHasSpriteSet() || spriteSetId() == mapSpriteSetId();
}

// ── WHAT THE CONSOLE WOULD ACTUALLY DRAW ─────────────────────────────────────────────────────────
//
// ⚠️ This is the routine the whole "will my sprite render?" question turns on, and until 2026-07-13
// we answered it WRONG -- we asked the save's cached sprite set, which the game **throws away**.
//
// Twilight: *"No matter what sprite set is set to, the map should render exactly and completely
// accurately, like the game would after loading a modified save file."* So we do what the game does.
// The whole of `engine/overworld/map_sprites.asm`, in two halves:
//
// ── OUTDOOR (`wCurMap < FIRST_INDOOR_MAP`, i.e. map id < $25) ────────────────────────────────────
//
//   `InitOutsideMapSprites` reads the set id out of a **ROM table** -- `MapSpriteSets[wCurMap]`,
//   splits resolved against the player's position -- loads *that* set's 11 pictures into VRAM, and
//   then maps each NPC to a VRAM slot by SEARCHING the set for its picture:
//
//       .getPictureIndexLoop
//           inc c
//           ld a, [de]
//           inc de
//           cp b                        ; does the picture ID match?
//           jr nz, .getPictureIndexLoop ; ⚠️ NO BOUNDS CHECK. NO TERMINATION.
//
//   **There is no bounds check.** A picture that is not in the set runs the loop off the end of
//   `wSpriteSet` and on through WRAM until some byte happens to equal it. The VRAM slot it lands on
//   is arbitrary, and the sprite draws whatever tiles are sitting there. That is what "the game
//   would draw it as garbage" actually means, and it is why the warning is worth having.
//
//   The save's cached set is irrelevant: `LoadMapData` zeroes the id and this recomputes the lot.
//
// ── INDOOR (map id >= $25) ───────────────────────────────────────────────────────────────────────
//
//   `InitOutsideMapSprites` returns immediately. `InitMapSprites` then copies **each sprite's OWN
//   picture** into StateData2 and calls `LoadMapSpriteTilePatterns`, which allocates VRAM slots by
//   **first appearance**, deduplicating identical pictures.
//
//   **THERE IS NO SPRITE SET INDOORS.** Every NPC's own artwork is loaded, so *any* picture draws
//   correctly -- right up until you run out of video memory. The capacity is real and it is small:
//
//     * **10** slots for walking sprites (picture < `FIRST_STILL_SPRITE` = $3D), and
//     * **2**  four-tile slots for still ones (Pokéballs, boulders, the fossil: picture >= $3D).
//
//   So indoors the question is not "is it in the set" -- it is "did the map run out of room".
namespace {

constexpr int FirstIndoorMap   = 0x25;  ///< constants/map_constants.asm
constexpr int FirstStillSprite = 0x3D;  ///< constants/sprite_constants.asm -- SPRITE_POKE_BALL up
constexpr int WalkingVramSlots = 10;    ///< map_sprites.asm: `cp 11 ; is it one of the first 10 slots?`
constexpr int StillVramSlots   = 2;     ///< the two 4-tile slots at vSprites tile $78 / $7c

} // namespace

bool MapModel::showScratch() const { return showScratchFields; }

void MapModel::setShowScratch(bool show)
{
  if (showScratchFields == show)
    return;

  showScratchFields = show;

  emit showScratchChanged();
  emit changed();   // the Details panel's field list is a different list now
}

bool MapModel::mapIsIndoors() const
{
  return mapInd() >= FirstIndoorMap;
}

QVector<int> MapModel::vramPictures() const
{
  QVector<int> out;

  // ── Outdoor: the ROM's sprite set. Not the save's copy of it. ──────────────────────────────
  if (!mapIsIndoors()) {
    MapDBEntry* entry = MapEngine::sourceMap(mapInd());
    if (entry == nullptr)
      return out;

    SpriteSetDBEntry* set = entry->getToSpriteSet();
    if (set == nullptr)
      return out;

    const SpriteSetDBEntry* resolved = set->getResolvedSet(static_cast<var8>(playerX()),
                                                           static_cast<var8>(playerY()));
    if (resolved == nullptr)
      return out;

    for (SpriteDBEntry* s : resolved->getSprites(static_cast<var8>(playerX()),
                                                 static_cast<var8>(playerY()))) {
      if (s != nullptr)
        out.append(int(s->ind));
    }

    return out;
  }

  // ── Indoor: the cast IS the set. First appearance, deduplicated, until the VRAM runs out. ──
  if (npcs == nullptr)
    return out;

  int walking = 0;
  int still = 0;

  for (int slot = 1; slot < npcs->spriteCount(); slot++) {
    SpriteData* s = npcs->spriteAt(slot);
    if (s == nullptr || s->pictureID == 0)
      continue;

    const int pic = s->pictureID;

    if (out.contains(pic))
      continue;   // .checkIfAlreadyLoadedLoop -- it shares the earlier slot's tiles

    // Out of room. The console does not refuse; it just has nowhere to put the tiles, and what the
    // sprite draws is whatever is already in that slot. Either way: not this character.
    if (pic >= FirstStillSprite) {
      if (still >= StillVramSlots)
        continue;
      still++;
    }
    else {
      if (walking >= WalkingVramSlots)
        continue;
      walking++;
    }

    out.append(pic);
  }

  return out;
}

bool MapModel::pictureWouldRender(int picture) const
{
  if (picture <= 0)
    return false;

  // SPRITE_RED is always in VRAM -- LoadSpriteSetData writes it into the player's own slot before
  // the set goes in, and indoors the player's picture is copied in with everybody else's.
  if (picture == SpritePlayerPicture)
    return true;

  return vramPictures().contains(picture);
}

bool MapModel::pictureWouldRenderIfAdded(int picture) const
{
  if (picture <= 0)
    return false;

  if (pictureWouldRender(picture))
    return true;   // already in video memory -- a second copy costs nothing (.alreadyLoaded)

  // Outdoors, that is the end of it: the VRAM is the map's ROM sprite set and nothing you put on the
  // map can change what is in it.
  if (!mapIsIndoors())
    return false;

  // Indoors, the cast IS the set -- so the only question is whether there is a slot left of the
  // right KIND. A Pokéball cannot borrow a walking sprite's slot: the still sprites live in their
  // own two four-tile slots.
  const QVector<int> vram = vramPictures();

  int walking = 0;
  int still = 0;

  for (int p : vram) {
    if (p >= FirstStillSprite)
      still++;
    else
      walking++;
  }

  return (picture >= FirstStillSprite) ? (still < StillVramSlots)
                                       : (walking < WalkingVramSlots);
}

QVariantList MapModel::spriteList() const
{
  QVariantList out;

  QVariantMap none;
  none["ind"] = 0;
  none["name"] = QObject::tr("Empty");
  out.append(none);

  for (auto* el : SpritesDB::inst()->getStore()) {
    QVariantMap m;
    m["ind"] = el->ind;
    m["name"] = el->name;
    out.append(m);
  }

  return out;
}

QVariantList MapModel::spriteSetList() const
{
  QVariantList out;

  QVariantMap none;
  none["ind"] = 0;
  none["name"] = spriteSetNameOf(0);
  out.append(none);

  // The ten real sets, in id order. The twelve split ids are deliberately NOT offered: the console
  // resolves them before it stores anything, so a save can never legitimately hold one.
  for (int id = 1; id <= 10; id++) {
    auto* el = setAt(id);
    if (el == nullptr)
      continue;

    QVariantMap m;
    m["ind"] = id;
    m["name"] = spriteSetNameOf(id);
    out.append(m);
  }

  return out;
}

QVariantList MapModel::cachedSprites() const
{
  QVariantList out;
  if (sprites == nullptr)
    return out;

  for (int i = 0; i < sprites->lSpriteCount(); i++) {
    const int ind = sprites->lSpriteAt(i);

    QString name = QObject::tr("Empty");
    for (auto* el : SpritesDB::inst()->getStore())
      if (el->ind == ind)
        name = el->name;

    QVariantMap m;
    m["slot"] = i;
    m["ind"] = ind;
    m["name"] = (ind == 0) ? QObject::tr("Empty") : name;

    // The game keeps NINE walking sprites and then TWO still ones -- the last two slots are a
    // different kind of thing, and the panel says so.
    m["still"] = (i >= 9);

    out.append(m);
  }

  return out;
}

void MapModel::setCachedSprite(int slot, int picture)
{
  if (sprites == nullptr)
    return;

  sprites->lSpriteSet(slot, picture);   // one byte, and only that byte
  changed();
}

void MapModel::applySpriteSet(int setId)
{
  if (sprites == nullptr)
    return;

  if (setId == 0) {
    sprites->reset();       // all eleven slots + the id
    changed();
    return;
  }

  auto* el = setAt(setId);
  if (el == nullptr)
    return;

  sprites->loadSpriteSet(el, playerX(), playerY());
  changed();
}

void MapModel::applyMapSpriteSet()
{
  applySpriteSet(mapSpriteSetId());
}

int MapModel::borderBlock() const
{
  // `wMapBackgroundTile` (save 0x2659). The console reads THIS, not the map's shipped border -- so
  // the renderer must too, or editing the edge of the world does nothing on screen (which it didn't,
  // until 2026-07-13).
  return map->outOfBoundsBlock;
}

MapEngine::Buffer MapModel::mapBuffer() const
{
  // Every question about the map's blocks -- what is at this pixel, does this layer apply, is the
  // selection still inside -- goes through here, so they all agree with what is DRAWN.
  return MapEngine::buildOverworldMap(mapInd(), borderBlock());
}

// ── The map, the tileset, the blockset ────────────────────────────────────────

void MapModel::setMapInd(int ind)
{
  if (ind < 0 || ind > 255 || map->curMap == ind)
    return;

  // ONE byte. The map's size, its data pointers and its warps are all separate bytes elsewhere in
  // the save, and rewriting them because someone picked another map from a list is precisely the
  // "silently normalise the save" behaviour this editor exists not to do. The screen SHOWS when the
  // stored size no longer matches (@ref headerMatches) and offers to fix it (@ref fixMapHeader).
  map->curMap = ind;
  map->curMapChanged();

  changed();
}

void MapModel::setTilesetInd(int ind)
{
  if (ind < 0 || tileset->current == ind)
    return;

  auto* el = canonAt(ind);
  if (el == nullptr)
    return;

  // The tileset id AND the four pointers that make it real -- because a tileset id with another
  // tileset's pointers is not "tileset N", it is a mess, and the game reads the POINTERS. The grass
  // tile, the counters and the animation byte are deliberately left alone: those are the user's.
  tileset->current = ind;
  tileset->bank = el->bank;
  tileset->blockPtr = el->blockPtr;
  tileset->gfxPtr = el->gfxPtr;
  tileset->collPtr = el->collPtr;

  tileset->currentChanged();
  tileset->bankChanged();
  tileset->blockPtrChanged();
  tileset->gfxPtrChanged();
  tileset->collPtrChanged();

  changed();
}

int MapModel::blocksetInd() const
{
  // Which tileset's blocks the save's `blockPtr` actually names. Normally the loaded tileset's own;
  // a save may point somewhere else entirely, and the console would draw exactly that.
  for (auto* el : TilesetDB::inst()->getStore())
    if (el->blockPtr == tileset->blockPtr)
      return el->ind;

  return -1;   // a pointer that is no tileset's blockset -- shown, never "corrected"
}

void MapModel::setBlocksetInd(int ind)
{
  auto* el = canonAt(ind);
  if (el == nullptr || tileset->blockPtr == el->blockPtr)
    return;

  // Just the blocks pointer. NOT the graphics pointer -- picking a different blockset is the whole
  // point of this being its own control.
  tileset->blockPtr = el->blockPtr;
  tileset->blockPtrChanged();

  changed();
}

bool MapModel::blocksetIsTileset() const
{
  return blocksetInd() == tilesetInd();
}

QString MapModel::blocksetName() const
{
  auto* el = canonAt(blocksetInd());
  return (el == nullptr) ? QObject::tr("Custom pointer") : el->name;
}

bool MapModel::headerMatches() const
{
  auto* entry = MapEngine::sourceMap(mapInd());
  if (entry == nullptr)
    return true;   // nothing to compare against -- don't cry wolf

  return map->width == entry->getWidth() && map->height == entry->getHeight();
}

void MapModel::fixMapHeader()
{
  auto* entry = MapEngine::sourceMap(mapInd());
  if (entry == nullptr)
    return;

  // Exactly the size the loaded map really is, and nothing else. (The 2x2 fields are the same size
  // counted the way the game counts it for its bigger steps.)
  map->width = entry->getWidth();
  map->height = entry->getHeight();
  map->width2x2 = entry->getWidth() * 2;
  map->height2x2 = entry->getHeight() * 2;

  map->widthChanged();
  map->heightChanged();
  map->width2x2Changed();
  map->height2x2Changed();

  changed();
}

int MapModel::contrastPercent() const
{
  // 0 is a normal screen, 9 is black -- so as a brightness it reads backwards. The picker shows the
  // thing a person means: 100% is what you see when nothing is wrong.
  const int c = qBound(0, contrast(), contrastMax());
  return qRound((1.0 - (c / static_cast<double>(contrastMax()))) * 100.0);
}

QVariantList MapModel::mapList() const
{
  QVariantList out;

  // DB entry fields are protected -- always the getters, never the members (a standing rule; see
  // CLAUDE.md).
  //
  // GROUPED, like the music list (Twilight, 2026-07-13). 248 names in one flat list is a wall. The
  // group is the map's own TILESET -- which is real data out of maps.json, not a category we made up:
  // "Overworld" gathers the towns and routes, "Cave" the caves, "Pokecenter" every Poké Center, and
  // so on. The unfinished copies get their own group, because that is what they are.
  QString lastGroup;

  QVector<MapDBEntry*> sorted;
  for (auto* el : MapsDB::inst()->getStore())
    sorted.append(el);

  std::stable_sort(sorted.begin(), sorted.end(), [](MapDBEntry* a, MapDBEntry* b) {
    auto groupOf = [](MapDBEntry* e) {
      auto* src = MapEngine::sourceMap(e->getInd());
      const bool copy = (src != nullptr && src != e);
      // The copies sort last -- they are curiosities, not places.
      return copy ? QStringLiteral("zzz") + e->getTileset() : e->getTileset();
    };

    const QString ga = groupOf(a);
    const QString gb = groupOf(b);
    return (ga == gb) ? (a->getInd() < b->getInd()) : (ga < gb);
  });

  for (auto* el : sorted) {
    auto* src = MapEngine::sourceMap(el->getInd());
    const bool copy = (src != nullptr && src != el);

    const QString group = copy ? QObject::tr("Unfinished copies")
                               : (el->getTileset().isEmpty() ? QObject::tr("Other")
                                                             : el->getTileset());

    // The heading rides on the first entry of each group, so QML can draw it without a second model.
    QVariantMap m;
    m["ind"] = el->getInd();
    m["name"] = el->getName();
    m["group"] = (group != lastGroup) ? group : QString();
    m["isCopy"] = copy;
    m["copyOf"] = copy ? src->getName() : QString();

    lastGroup = group;
    out.append(m);
  }

  return out;
}

int MapModel::frame() const
{
  return animFrame;
}

void MapModel::setFrame(int frame)
{
  // The water's rotation is an 8-step ping-pong and the flower's three tiles are chosen from the
  // same counter, so the whole animation closes on 8. Keeping the number small keeps the image
  // provider's cache small -- the URL is the cache key.
  const int wrapped = ((frame % 8) + 8) % 8;

  if (animFrame == wrapped)
    return;

  animFrame = wrapped;
  emit frameChanged();
  emit sourceChanged();   // the frame is IN the URL -- the image must be re-fetched
}

int MapModel::contrast() const
{
  return general->contrast;
}

void MapModel::setContrast(int contrast)
{
  if (general->contrast == contrast)
    return;

  // This writes a real save byte (wMapPalOffset, 0x2609) -- and only that byte.
  general->contrast = contrast;
  general->contrastChanged();
}

bool MapModel::contrastIsGlitch() const
{
  return MapEngine::isGlitchPalette(contrast());
}

QString MapModel::contrastName() const
{
  return MapEngine::contrastName(contrast());
}

int MapModel::contrastMax() const
{
  return MapEngine::contrastMax;
}

QString MapModel::mapName() const
{
  auto* entry = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  return (entry == nullptr) ? QString() : entry->bestName();
}

bool MapModel::isCopy() const
{
  auto* source = MapEngine::sourceMap(mapInd());
  return source != nullptr && source->getInd() != mapInd();
}

QString MapModel::copyOfName() const
{
  auto* source = MapEngine::sourceMap(mapInd());

  if (source == nullptr || source->getInd() == mapInd())
    return QString();

  return source->bestName();
}

QString MapModel::tilesetName() const
{
  for (auto* el : TilesetDB::inst()->getStore())
    if (el->ind == tilesetInd())
      return el->name;

  return QString();
}

// The size comes from whichever map actually supplies the data -- a glitch id carries
// no dimensions of its own, so it takes the ones from the map it is a copy of.

int MapModel::blocksWide() const
{
  auto* entry = MapEngine::sourceMap(mapInd());
  return (entry == nullptr) ? 0 : entry->getWidth();
}

int MapModel::blocksHigh() const
{
  auto* entry = MapEngine::sourceMap(mapInd());
  return (entry == nullptr) ? 0 : entry->getHeight();
}

int MapModel::blockSize() const { return MapEngine::blockPx; }

int MapModel::imageWidth() const
{
  return (blocksWide() + 2 * MapEngine::mapBorder) * MapEngine::blockPx;
}

int MapModel::imageHeight() const
{
  return (blocksHigh() + 2 * MapEngine::mapBorder) * MapEngine::blockPx;
}

int MapModel::mapX() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).x(); }
int MapModel::mapY() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).y(); }
int MapModel::mapW() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).width(); }
int MapModel::mapH() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).height(); }

int MapModel::scratchX() const { return MapEngine::scratchRect(playerX(), playerY()).x(); }
int MapModel::scratchY() const { return MapEngine::scratchRect(playerX(), playerY()).y(); }
int MapModel::scratchW() const { return MapEngine::scratchRect(playerX(), playerY()).width(); }
int MapModel::scratchH() const { return MapEngine::scratchRect(playerX(), playerY()).height(); }

int MapModel::screenX() const { return MapEngine::screenRect(playerX(), playerY()).x(); }
int MapModel::screenY() const { return MapEngine::screenRect(playerX(), playerY()).y(); }
int MapModel::screenW() const { return MapEngine::screenRect(playerX(), playerY()).width(); }
int MapModel::screenH() const { return MapEngine::screenRect(playerX(), playerY()).height(); }

QVariantList MapModel::contrastShades(int contrast) const
{
  QVariantList out;

  // ⚠️ THE REAL PALETTE, not a decoration. The contrast segments used to be painted in the app's
  // accent blue (and the glitch ones in yellow) -- which told you *that* a value was unusual and
  // nothing whatever about what it would do to the map. Twilight: *"coloured segments matching the
  // current colours."* So each segment now wears the four shades that value actually renders in, and
  // sliding along the strip shows you the map going dark before the map does.
  //
  // `backgroundPalette` is the genuine rBGP byte the console would be holding -- glitch reads across
  // the fade table's seam included. We are not approximating; we are asking the engine.
  const int bgp = MapEngine::backgroundPalette(contrast);

  if (bgp < 0)
    return out;   // a value we have no palette for. The caller draws nothing rather than inventing.

  // Bits 1-0 are colour 0, bits 7-6 are colour 3 -- the hardware's own layout.
  static const int grey[4] = { 255, 170, 85, 0 };

  for (int i = 0; i < 4; i++) {
    const int shade = (bgp >> (2 * i)) & 3;
    const int g = grey[shade];
    out.append(QColor(g, g, g));
  }

  return out;
}

// ── The output palette (Game Boy Color / custom colour filter) ───────────────────────────────────
//
// ⚠️ Every one of these is a VIEW setting. Not a byte of the save moves. They live on MapEngine as a
// global; MapModel is the QML face and the thing that re-renders the map when they change.

int MapModel::colourMode() const { return MapEngine::colourMode(); }

void MapModel::setColourMode(int mode)
{
  if (mode == MapEngine::colourMode())
    return;

  MapEngine::setColourMode(mode);
  emit colourModeChanged();
  emit sourceChanged();   // the palette generation is in the URL; a new one re-renders the map
}

QVariantList MapModel::colourPresets() const { return MapEngine::colourPresets(); }

QVariantList MapModel::customColours() const
{
  QVariantList out;
  for (int i = 0; i < 4; i++)
    out.append(QColor(MapEngine::customColour(i)));

  return out;
}

void MapModel::setCustomColour(int shade, const QColor& colour)
{
  MapEngine::setCustomColour(shade, colour.rgb());

  // Editing a swatch means you want to SEE it -- so it also switches into Custom mode.
  if (MapEngine::colourMode() != MapEngine::Custom)
    MapEngine::setColourMode(MapEngine::Custom);

  emit colourModeChanged();
  emit sourceChanged();
}

QVariantMap MapModel::viewBoxesAt(int x, int y) const
{
  // ⚠️ The same MapEngine routines the bound properties use, just asked about a position that is not
  // (yet) the player's. That is what lets the two boxes follow him **while you are dragging him**
  // rather than snapping into place on release (Twilight, 2026-07-13: "Can the camera box and draw
  // area box update live as player is moved around" -- they can, and now they do).
  //
  // One source of truth: if these boxes are ever wrong, they are wrong in MapEngine, together.
  const QRect screen = MapEngine::screenRect(x, y);
  const QRect draw   = MapEngine::scratchRect(x, y);

  QVariantMap m;
  m["screenX"] = screen.x();
  m["screenY"] = screen.y();
  m["screenW"] = screen.width();
  m["screenH"] = screen.height();
  m["drawX"]   = draw.x();
  m["drawY"]   = draw.y();
  m["drawW"]   = draw.width();
  m["drawH"]   = draw.height();
  return m;
}

// ── The player ────────────────────────────────────────────────────────────────

int MapModel::playerFacing() const
{
  return MapEngine::facingFromPlayerDir(player->playerCurDir);
}

QString MapModel::playerSource() const
{
  // The contrast rides along because the player is drawn through the OBJECT palette -- and
  // that is the one the "harmless" glitch palettes actually damage.
  return "image://player/" + QString::number(playerFacing())
       + "/" + QString::number(contrast());
}

int MapModel::playerRectX() const { return MapEngine::playerRect(playerX(), playerY()).x(); }
int MapModel::playerRectY() const { return MapEngine::playerRect(playerX(), playerY()).y(); }
int MapModel::playerRectW() const { return MapEngine::playerRect(playerX(), playerY()).width(); }
int MapModel::playerRectH() const { return MapEngine::playerRect(playerX(), playerY()).height(); }

// ── Everybody else: the other fifteen sprite slots ────────────────────────────

QVariantList MapModel::npcList() const
{
  QVariantList ret;

  if (npcs == nullptr)
    return ret;

  // ⚠️ What the CONSOLE would have in video memory after it loaded this save -- computed the way
  // the console computes it (@ref vramPictures), NOT read out of the save's cached sprite set,
  // which the game overwrites before it ever looks at it. Outdoors that is the map's ROM sprite
  // set; **indoors there is no sprite set at all** and the cast IS the set.
  //
  // A sprite whose picture is not in there is what the console draws as garbage -- the lookup runs
  // off the end of `wSpriteSet` with no bounds check and lands on an arbitrary VRAM slot. So we say
  // so, rather than drawing it correctly here and letting the user find out on a cartridge.
  const QVector<int> loaded = vramPictures();

  // Slot 0 is the player and has his own layer. Everyone else is here.
  for (int i = 1; i < npcs->spriteCount(); i++) {
    SpriteData* s = npcs->spriteAt(i);
    if (s == nullptr)
      continue;

    // Picture id 0 means the slot is unused (ram/wram.asm). Draw nothing.
    if (s->pictureID == 0)
      continue;

    // mapX/mapY carry the game's +4 bias ("the topmost 2x2 tile has value 4").
    const int x = s->mapX - 4;
    const int y = s->mapY - 4;

    const QRect r = MapEngine::playerRect(x, y);   // same geometry, 4px lift and all

    SpriteDBEntry* entry = s->toSprite();

    QVariantMap m;
    m["slot"]    = i;
    m["picture"] = s->pictureID;
    m["name"]    = (entry != nullptr) ? entry->name : tr("Sprite %1").arg(s->pictureID);
    m["x"]       = x;
    m["y"]       = y;
    m["facing"]  = s->faceDir;
    m["rectX"]   = r.x();
    m["rectY"]   = r.y();
    m["rectW"]   = r.width();
    m["rectH"]   = r.height();
    // ── THE WALK, as the console draws it ──────────────────────────────────────────────────
    //
    // ⚠️ Sprites TELEPORTED between tiles, and this is why. `TryWalking` moves mapX/mapY to the
    // DESTINATION immediately and then slides the sprite 1 pixel a frame for 16 frames -- so the
    // tile coordinate is where it is GOING, not where it IS. Drawing straight from mapX/mapY
    // skipped the entire step. (Twilight: "when they walk they don't slide or move or animate
    // properly, it still looks bad.")
    //
    // The offset is exact, and it needs no reconstruction of the console's screen-pixel fields:
    //
    //     offset = −walkAnimationCounter × stepVector      (16 -> the source tile, 0 -> arrived)
    //
    const int step = (s->movementStatus == 3) ? s->walkAnimationCounter : 0;
    const int sx = (s->getXStepVector() & 0x80) ? (s->getXStepVector() - 256) : s->getXStepVector();
    const int sy = (s->getYStepVector() & 0x80) ? (s->getYStepVector() - 256) : s->getYStepVector();

    m["offX"] = -step * sx;
    m["offY"] = -step * sy;

    // animFrameCounter (0-3) + facing walks SpriteFacingAndAnimationTable. Frames 1 and 3 are the
    // SAME picture -- 3 is mirrored, and that mirror is the other leg. @see MapEngine::npcSprite
    m["source"]  = "image://player/npc/" + QString::number(s->pictureID)
                 + "/" + QString::number(s->faceDir)
                 + "/" + QString::number(contrast())
                 + "/" + QString::number(s->animFrameCounter & 3);

    // The two things about a sprite that a person cannot see by looking at it.
    m["inSpriteSet"] = (s->pictureID == SpritePlayerPicture) || loaded.contains(s->pictureID);
    m["missable"]    = s->getMissableIndex();

    ret.append(m);
  }

  return ret;
}

// ── Editing the cast ──────────────────────────────────────────────────────────

void MapModel::moveNpc(int slot, int x, int y)
{
  if (npcs == nullptr)
    return;

  // Clamp to the map. The border ring is not a place a sprite can usefully be -- the game never
  // draws one there -- so a drag that overshoots stops at the edge rather than parking somebody
  // in the trees.
  const int w = blocksWide() * 2;
  const int h = blocksHigh() * 2;

  x = std::max(0, std::min(w - 1, x));
  y = std::max(0, std::min(h - 1, y));

  npcs->spriteMove(slot, x, y);   // exactly two bytes
  castEdited = true;
  changed();
}

int MapModel::addNpc(int pictureID, int x, int y)
{
  if (npcs == nullptr)
    return -1;

  const int w = blocksWide() * 2;
  const int h = blocksHigh() * 2;

  x = std::max(0, std::min(w - 1, x));
  y = std::max(0, std::min(h - 1, y));

  const int slot = npcs->spriteAdd(pictureID, x, y);
  castEdited = true;
  changed();
  return slot;
}

void MapModel::removeNpc(int slot)
{
  if (npcs == nullptr)
    return;

  npcs->spriteRemove(slot);   // the rest slide up -- the game packs its slots and so do we
  castEdited = true;
  changed();
}

int MapModel::addRandomLoadedNpc(int x, int y)
{
  if (npcs == nullptr || npcRoomLeft() <= 0)
    return -1;

  // ⚠️ Only what the console would actually have in video memory here. Picking out of all 72 would
  // land you on an amber "this map hasn't loaded that picture" sprite roughly five times in six --
  // i.e. the one-click tool would mostly produce the broken case. @see vramPictures.
  QVector<int> loaded = vramPictures();

  // The player's own picture is loaded on every map, ahead of the set -- but a second Red walking
  // about is not what anybody means by "a random character".
  loaded.removeAll(SpritePlayerPicture);
  loaded.removeAll(0);

  if (loaded.isEmpty())
    return -1;

  const int picture = loaded.at(Random::inst()->rangeExclusive(0, loaded.size()));

  return addNpc(picture, x, y);
}

void MapModel::movePlayer(int x, int y)
{
  if (player == nullptr)
    return;

  // ⚠️ His coordinates are NOT in the sprite table -- they are his own two bytes, and the game's
  // view pointer is computed from them. Moving him therefore invalidates the pointer the save is
  // carrying, which the Details panel notices and offers to fix. (We never rewrite it behind you.)
  x = std::max(0, std::min(blocksWide() * 2 - 1, x));
  y = std::max(0, std::min(blocksHigh() * 2 - 1, y));

  player->xCoord = x;
  player->xCoordChanged();

  player->yCoord = y;
  player->yCoordChanged();

  changed();
}

int MapModel::npcRoomLeft() const
{
  if (npcs == nullptr)
    return 0;

  // 16 slots, and slot 0 is always the player.
  return std::max(0, npcs->spriteMax() - npcs->spriteCount());
}

QVariantList MapModel::spriteCatalog() const
{
  QVariantList ret;

  // The shelves, in the order they read.
  const QStringList order = { "Story", "Trainers", "Townsfolk", "Pokemon", "Objects" };

  for (const QString& group : order) {
    for (int i = 0; i < SpritesDB::inst()->getStoreSize(); i++) {
      SpriteDBEntry* e = SpritesDB::inst()->getStoreAt(i);
      if (e == nullptr || e->group != group)
        continue;

      QVariantMap m;
      m["ind"]   = int(e->ind);
      m["name"]  = e->name;
      m["group"] = e->group;

      // Facing down -- that is what a character looks like when you are choosing one.
      m["source"] = "image://player/npc/" + QString::number(e->ind)
                  + "/" + QString::number(MapEngine::FacingDown)
                  + "/" + QString::number(contrast());

      // ⚠️ "Would the CONSOLE draw this, if I dropped it here?" -- asked of the machine, not of the
      // save's cached sprite set (which the game overwrites before it ever reads it).
      //
      // Outdoors that means "is it in the map's ROM sprite set". **Indoors it means "is there a
      // video-memory slot left"** -- because indoors there IS no sprite set: the game loads each
      // NPC's own artwork. Which is why the marks move around as you fill a building up.
      const bool ok = pictureWouldRenderIfAdded(int(e->ind));
      m["inSpriteSet"] = ok;

      // ⚠️ SHORT. Twilight: *"the yellow exclamation point needs a shorter tooltip -- why not 'This
      // sprite is not expected on this map so it would come off garbled'."* One sentence. The long
      // version is what the panel's "?" is for.
      if (!ok) {
        m["why"] = mapIsIndoors()
          ? tr("This map is out of room for characters, so this one would come out garbled.")
          : tr("This character isn't expected on this map, so it would come out garbled.");
      }

      // The one character that needs a word of its own: dropping this out gives you a SECOND Red
      // standing on the map. Perfectly legal, perfectly drawable -- just worth saying out loud.
      if (int(e->ind) == SpritePlayerPicture) {
        m["note"] = tr("This is the player's own picture — drop one out and you get a second Red "
                       "standing on the map, drawn properly. The real player is always on the map "
                       "anyway; he is the one you can't delete.");
      }

      ret.append(m);
    }
  }

  return ret;
}

QVariantMap MapModel::npcAt(int slot) const
{
  const QVariantList all = npcList();
  for (const QVariant& v : all) {
    const QVariantMap m = v.toMap();
    if (m.value("slot").toInt() == slot)
      return m;
  }
  return QVariantMap();
}

bool MapModel::npcsEdited() const
{
  return castEdited;
}

// ── "Zoom to…" ────────────────────────────────────────────────────────────────
//
// Everything here answers in BUFFER pixels, because that is the one coordinate system the canvas
// speaks. The border ring is included, exactly as it is in every other rectangle this model
// publishes -- so a warp at map tile (5, 5) is at buffer pixel (3*32 + 5*16, ...).

namespace {

/// A point worth looking at.
QVariantMap target(bool ok, int x = 0, int y = 0, const QString& label = QString())
{
  QVariantMap m;
  m["ok"]    = ok;
  m["x"]     = x;
  m["y"]     = y;
  m["label"] = label;
  return m;
}

} // namespace

QVariantList MapModel::warpPoints() const
{
  QVariantList out;
  if (warps == nullptr)
    return out;

  for (int i = 0; i < warps->warpCount(); i++) {
    WarpData* w = warps->warpAt(i);
    if (w == nullptr)
      continue;

    QVariantMap m;
    // A warp's x/y are MAP tiles (half-blocks), like the player's.
    m["x"] = MapEngine::mapBorder * MapEngine::blockPx + w->x * 16 + 8;
    m["y"] = MapEngine::mapBorder * MapEngine::blockPx + w->y * 16 + 8;
    m["ind"] = i;
    out.append(m);
  }

  return out;
}

// ══ THE DOORS ═════════════════════════════════════════════════════════════════════════════════
//
// notes/reference/warps.md is the write-up, and it is verified against the cartridge. Nothing in
// here is guessed, and several of the names v1 used were wrong.

namespace {

/// A map's display name, or a plain "Map N" if the id names nothing (a glitch id still renders).
QString mapNameOf(int ind)
{
  auto* entry = MapsDB::inst()->getIndAt(QString::number(ind));
  return (entry == nullptr) ? QObject::tr("Map %1").arg(ind) : entry->bestName();
}

/// `LAST_MAP` -- the destination that means "put me back outside", i.e. on `wLastMap`.
constexpr int kReturnMap = 0xFF;

/// How many **arrival points** map @p ind has (its ROM `warps_to` list). This -- NOT its warp count
/// -- is what a door's `destWarp` indexes, and `LoadDestinationWarpPosition` does not bounds-check
/// it: past the end, the console copies four arbitrary ROM bytes into the view pointer and the
/// player's coordinates.
int arrivalCountOf(int ind)
{
  auto* entry = MapsDB::inst()->getIndAt(QString::number(ind));
  return (entry == nullptr) ? 0 : entry->getWarpIn().size();
}

} // namespace

int MapModel::lastMap() const
{
  return (world == nullptr) ? 0 : world->lastMap;
}

void MapModel::setLastMap(int ind)
{
  if (world == nullptr || world->lastMap == ind)
    return;

  world->lastMap = ind & 0xFF;   // ONE byte. Nothing else moves.
  world->lastMapChanged();
}

QString MapModel::lastMapName() const
{
  return mapNameOf(lastMap());
}

int MapModel::lastBlackoutMap() const
{
  return (world == nullptr) ? 0 : world->lastBlackoutMap;
}

void MapModel::setLastBlackoutMap(int ind)
{
  if (world == nullptr || world->lastBlackoutMap == ind)
    return;

  world->lastBlackoutMap = ind & 0xFF;
  world->lastBlackoutMapChanged();
}

QString MapModel::lastBlackoutMapName() const
{
  return mapNameOf(lastBlackoutMap());
}

QString MapModel::warpDestLabel(int destMap, int destWarp) const
{
  // The $FF door. It does not name a map at all -- it means "back outside", and WHERE outside is
  // lives in `wLastMap`. So it resolves live: change "Outside is…" and every one of these re-reads.
  const int target = (destMap == kReturnMap) ? lastMap() : destMap;

  const int arrivals = arrivalCountOf(target);
  if (destWarp < 0 || destWarp >= arrivals)
    return QString();   // the caller says "no such arrival point" -- it is not our job to soften it

  auto* entry = MapsDB::inst()->getIndAt(QString::number(target));
  if (entry == nullptr)
    return QString();

  MapDBEntryWarpIn* in = entry->getWarpIn().at(destWarp);
  if (in == nullptr)
    return QString();

  return tr("→ %1, arrival point %2 (%3, %4)")
      .arg(entry->bestName())
      .arg(destWarp)
      .arg(in->getX())
      .arg(in->getY());
}

QVariantList MapModel::warpList() const
{
  QVariantList out;
  if (warps == nullptr)
    return out;

  for (int i = 0; i < warps->warpCount(); i++) {
    WarpData* w = warps->warpAt(i);
    if (w == nullptr)
      continue;

    const bool isReturn = (w->destMap == kReturnMap);
    const int target    = isReturn ? lastMap() : w->destMap;
    const int arrivals  = arrivalCountOf(target);

    // A door sits on a TILE, and a tile is a half-block: 16 buffer pixels. The ring is 3 blocks.
    const int px = MapEngine::mapBorder * MapEngine::blockPx + w->x * 16;
    const int py = MapEngine::mapBorder * MapEngine::blockPx + w->y * 16;

    QVariantMap m;
    m["ind"]      = i;
    m["x"]        = w->x;
    m["y"]        = w->y;
    m["destMap"]  = w->destMap;
    m["destWarp"] = w->destWarp;

    m["rectX"] = px;
    m["rectY"] = py;
    m["rectW"] = 16;
    m["rectH"] = 16;

    m["isReturn"] = isReturn;

    // What it SAYS on the chip and in the status bar.
    m["destName"] = isReturn ? tr("back outside (%1)").arg(mapNameOf(lastMap()))
                             : mapNameOf(w->destMap);
    m["destLabel"]    = warpDestLabel(w->destMap, w->destWarp);
    m["arrivalCount"] = arrivals;

    // 🔫 The console does not bounds-check this. Shown and flagged; never refused, never rewritten.
    m["destValid"] = (w->destWarp >= 0 && w->destWarp < arrivals);

    out.append(m);
  }

  return out;
}

QVariantMap MapModel::warpAt(int ind) const
{
  const QVariantList all = warpList();
  for (const QVariant& v : all) {
    const QVariantMap m = v.toMap();
    if (m.value("ind").toInt() == ind)
      return m;
  }
  return QVariantMap();
}

void MapModel::moveWarp(int ind, int x, int y)
{
  if (warps == nullptr || ind < 0 || ind >= warps->warpCount())
    return;

  WarpData* w = warps->warpAt(ind);
  if (w == nullptr)
    return;

  // Clamp to the map. A door out in the 3-block border ring is a door the player can never reach,
  // so an overshooting drag stops at the edge rather than parking it in the trees.
  const int mw = blocksWide() * 2;
  const int mh = blocksHigh() * 2;

  x = std::max(0, std::min(mw - 1, x));
  y = std::max(0, std::min(mh - 1, y));

  if (w->x == x && w->y == y)
    return;

  // ⚠️ EXACTLY TWO BYTES. tst_warps byte-diffs the whole 32 KB save across this call and demands
  // that `x` and `y` are the only things in it that moved.
  w->x = x;
  w->xChanged();

  w->y = y;
  w->yChanged();

  warpsWereEdited = true;
  warps->warpsChanged();
}

int MapModel::addWarp(int x, int y)
{
  if (warps == nullptr || warpRoomLeft() <= 0)
    return -1;

  const int mw = blocksWide() * 2;
  const int mh = blocksHigh() * 2;

  x = std::max(0, std::min(mw - 1, x));
  y = std::max(0, std::min(mh - 1, y));

  warps->warpNew();

  const int ind = warps->warpCount() - 1;
  WarpData* w = warps->warpAt(ind);
  if (w == nullptr)
    return -1;

  w->x = x;
  w->xChanged();
  w->y = y;
  w->yChanged();

  // The sane default, and it is not arbitrary: a door is nearly always a way OUT. `$FF` = "back
  // outside" -- and it is the one destination that is always valid, because it resolves through
  // `wLastMap` rather than naming a map that may have no arrival points.
  w->destMap = kReturnMap;
  w->destMapChanged();
  w->destWarp = 0;
  w->destWarpChanged();

  warpsWereEdited = true;
  warps->warpsChanged();
  changed();   // the room left moved, and a layer may want lighting -- this one really does change things

  return ind;
}

void MapModel::removeWarp(int ind)
{
  if (warps == nullptr || ind < 0 || ind >= warps->warpCount())
    return;

  warps->warpRemove(ind);   // the rest slide up -- the game packs its warp list and so do we
  warpsWereEdited = true;
  changed();
}

int MapModel::warpRoomLeft() const
{
  if (warps == nullptr)
    return 0;

  return std::max(0, warps->warpMax() - warps->warpCount());
}

bool MapModel::warpsEdited() const
{
  return warpsWereEdited;
}

// ── The signs (placards) ─────────────────────────────────────────────────────────────────────────
//
// The doors' quieter sibling. Everything below is the warp implementation with `warp` swapped for
// `sign` -- the same canvas machinery, the same field kit -- except the one thing that IS different:
// a sign has WORDS, resolved from the map's text table (imported into maps.json by
// scripts/import_sign_text.py, notes/reference/signs.md). See MapModel::signTextList.

/// One line of a sign's text, for a chip or a combo row: the game prints it across several lines, and
/// a placard on the map has room for one. Newlines/box-breaks become " / "; long text is elided.
static QString signOneLine(const QString& text)
{
  QString s = text;
  s.replace("\n", " / ");
  s = s.simplified();
  constexpr int kMax = 48;
  if (s.size() > kMax)
    s = s.left(kMax - 1).trimmed() + QStringLiteral("…");
  return s;
}

/// The map's text-table entry for a 1-based @p textId, or nullptr if the id points past the table
/// (or the map is unknown). The DB carries the words; the save carries only the id.
static const MapDBEntryText* textEntryFor(int mapInd, int textId)
{
  MapDBEntry* m = MapsDB::inst()->getStoreAt(mapInd);
  if (m == nullptr || textId < 1 || textId > m->getTextEntriesSize())
    return nullptr;

  return m->getTextEntriesAt(textId - 1);
}

QString MapModel::signTextPreview(int textId) const
{
  const MapDBEntryText* e = textEntryFor(mapInd(), textId);
  if (e == nullptr)
    return QString();                            // points past the table -- the caller says so

  if (e->getScripted())
    return tr("(scripted text)");

  return signOneLine(e->getText());
}

QVariantList MapModel::signList() const
{
  QVariantList out;
  if (signsData == nullptr)
    return out;

  for (int i = 0; i < signsData->signCount(); i++) {
    SignData* s = signsData->signAt(i);
    if (s == nullptr)
      continue;

    // A sign sits on a TILE -- a half-block, 16 buffer pixels. The ring is 3 blocks. (No 4-px lift:
    // that is an OAM fact about sprites; a sign is a map coordinate and belongs on its tile.)
    const int px = MapEngine::mapBorder * MapEngine::blockPx + s->x * 16;
    const int py = MapEngine::mapBorder * MapEngine::blockPx + s->y * 16;

    const MapDBEntryText* e = textEntryFor(mapInd(), s->txtId);

    QVariantMap m;
    m["ind"]    = i;
    m["x"]      = s->x;
    m["y"]      = s->y;
    m["textId"] = s->txtId;

    m["rectX"] = px;
    m["rectY"] = py;
    m["rectW"] = 16;
    m["rectH"] = 16;

    // What it SAYS, on the chip and in the status bar. Empty when the id points nowhere on this map.
    m["preview"]   = signTextPreview(s->txtId);
    m["category"]  = (e == nullptr) ? QStringLiteral("") : e->getCategory();
    m["scripted"]  = (e != nullptr) && e->getScripted();
    m["textValid"] = (e != nullptr);

    out.append(m);
  }

  return out;
}

QVariantMap MapModel::signAt(int ind) const
{
  const QVariantList all = signList();
  for (const QVariant& v : all) {
    const QVariantMap m = v.toMap();
    if (m.value("ind").toInt() == ind)
      return m;
  }
  return QVariantMap();
}

void MapModel::moveSign(int ind, int x, int y)
{
  if (signsData == nullptr || ind < 0 || ind >= signsData->signCount())
    return;

  SignData* s = signsData->signAt(ind);
  if (s == nullptr)
    return;

  // Clamp to the map -- a sign out in the border ring is one the player can never read.
  const int mw = blocksWide() * 2;
  const int mh = blocksHigh() * 2;

  x = std::max(0, std::min(mw - 1, x));
  y = std::max(0, std::min(mh - 1, y));

  if (s->x == x && s->y == y)
    return;

  // ⚠️ EXACTLY TWO BYTES. tst_signs byte-diffs the whole 32 KB save across this and demands that the
  // sign's Y and X (in the coord array) are the only things in it that moved.
  s->x = x;
  s->xChanged();

  s->y = y;
  s->yChanged();

  signsWereEdited = true;
  signsData->signsChanged();
}

int MapModel::addSign(int x, int y)
{
  if (signsData == nullptr || signRoomLeft() <= 0)
    return -1;

  const int mw = blocksWide() * 2;
  const int mh = blocksHigh() * 2;

  x = std::max(0, std::min(mw - 1, x));
  y = std::max(0, std::min(mh - 1, y));

  signsData->signNew();

  const int ind = signsData->signCount() - 1;
  SignData* s = signsData->signAt(ind);
  if (s == nullptr)
    return -1;

  s->x = x;
  s->xChanged();
  s->y = y;
  s->yChanged();

  // A fresh placard should say something real. Default to this map's first SIGN-category text id, so
  // a new sign reads like a sign; fall back to id 1 if the map has no sign text at all.
  int defaultText = 1;
  MapDBEntry* m = MapsDB::inst()->getStoreAt(mapInd());
  if (m != nullptr) {
    for (int i = 0; i < m->getTextEntriesSize(); i++) {
      const MapDBEntryText* e = m->getTextEntriesAt(i);
      if (e != nullptr && e->getCategory() == QStringLiteral("sign")) {
        defaultText = e->getId();
        break;
      }
    }
  }

  s->txtId = defaultText;
  s->txtIdChanged();

  signsWereEdited = true;
  signsData->signsChanged();
  changed();   // the room left moved, and the Signs layer may want lighting -- this really changes things

  return ind;
}

void MapModel::removeSign(int ind)
{
  if (signsData == nullptr || ind < 0 || ind >= signsData->signCount())
    return;

  signsData->signRemove(ind);   // the rest slide up -- the game packs its sign list and so do we
  signsWereEdited = true;
  changed();
}

int MapModel::signRoomLeft() const
{
  if (signsData == nullptr)
    return 0;

  return std::max(0, signsData->signMax() - signsData->signCount());
}

QVariantList MapModel::signFields(int ind) const
{
  QVariantList ret;

  if (signsData == nullptr || ind < 0 || ind >= signsData->signCount())
    return ret;

  SignData* s = signsData->signAt(ind);
  if (s == nullptr)
    return ret;

  const QString where = tr("The sign");

  // X and Y are one fact, so they are one control -- the same `coords` kind the door/sprite panels use.
  ret.append(field(where, "xy", tr("Standing on"),
                   tr("Which tile of this map the sign is on, counting from the top-left."),
                   (s->x & 0xFF) | ((s->y & 0xFF) << 8), 0, 0, "coords"));

  // The words: a grouped picker of the map's real text. `enum` draws the combo and drops to a raw box
  // for any value no option names -- so a hack id past the map's table is reachable, and shown.
  ret.append(field(tr("What it says"), "textId", tr("Says…"),
                   tr("Which line of this map's text the sign prints when you read it.\n\n"
                      "The list is every text this map has — its own signs first, then the people's "
                      "dialogue, then the rest. A sign can point at any of them.\n\n"
                      "⚠️ An id past this map's text simply reads whatever text comes next in the "
                      "cartridge — shown, never refused."),
                   s->txtId, 0, 255, "enum", signTextList()));

  return ret;
}

void MapModel::setSignField(int ind, const QString& key, int value)
{
  if (signsData == nullptr || ind < 0 || ind >= signsData->signCount())
    return;

  SignData* s = signsData->signAt(ind);
  if (s == nullptr)
    return;

  if (key == "xy") {
    // Packed x | (y << 8), the same shape the door/sprite `coords` control speaks.
    moveSign(ind, value & 0xFF, (value >> 8) & 0xFF);
    return;
  }

  if (key == "textId") {
    s->txtId = value & 0xFF;
    s->txtIdChanged();
  } else {
    return;
  }

  signsWereEdited = true;
  signsData->signsChanged();
}

QVariantList MapModel::signTextList() const
{
  MapDBEntry* m = MapsDB::inst()->getStoreAt(mapInd());
  if (m == nullptr)
    return {};

  // Three sections, in the order Twilight asked for: the map's own SIGN text first, then the PEOPLE's
  // dialogue, then everything else. Each row shows the real words. The header rides on the first row
  // of its section (the same mechanism the item picker's `sectioned()` uses), so QML groups on it.
  QVariantList signs, people, other;

  for (int i = 0; i < m->getTextEntriesSize(); i++) {
    const MapDBEntryText* e = m->getTextEntriesAt(i);
    if (e == nullptr)
      continue;

    const QString words = e->getScripted() ? tr("(scripted text)") : signOneLine(e->getText());
    QVariantMap row = option(e->getId(), tr("%1 — %2").arg(e->getId()).arg(words));
    row["category"] = e->getCategory();

    if (e->getCategory() == QStringLiteral("sign"))
      signs.append(row);
    else if (e->getCategory() == QStringLiteral("person"))
      people.append(row);
    else
      other.append(row);
  }

  auto headed = [](QVariantList& list, const QString& heading) {
    if (list.isEmpty())
      return;
    QVariantMap first = list.first().toMap();
    first["header"] = heading;
    list[0] = first;
  };

  headed(signs, tr("Signs"));
  headed(people, tr("People"));
  headed(other, tr("Other text"));

  return signs + people + other;
}

bool MapModel::signsEdited() const
{
  return signsWereEdited;
}

QVariantList MapModel::flyWarpMapList() const
{
  QVariantList out;
  for (int ind : AreaWarps::legalFlyMaps())
    out.append(option(ind, mapNameOf(ind)));

  return out;
}

QVariantList MapModel::dungeonWarpMapList() const
{
  QVariantList out;
  QVector<int> seen;

  // Same as the hole list: 0 is the ordinary "nothing to fall onto" resting value, not a hack.
  out.append(option(0, tr("Nowhere — not falling")));

  for (const auto& pair : AreaWarps::legalDungeonWarps()) {
    if (seen.contains(pair.first))
      continue;

    seen.append(pair.first);
    out.append(option(pair.first, mapNameOf(pair.first)));
  }

  return out;
}

int MapModel::warpDungeonMap() const
{
  return (warps == nullptr) ? 0 : warps->dungeonWarpDestMap;
}

QVariantList MapModel::dungeonHoleList(int map) const
{
  QVariantList out;

  // ⚠️ **0 IS NOT A HACK VALUE. It is the resting state of the world.**
  //
  // `IsPlayerOnDungeonWarp` writes 0 here as its *first instruction*, every time you are not standing
  // on a hole -- so essentially every save ever made carries a 0, and offering only 1..3 would leave
  // the picker showing nothing on a perfectly ordinary file. It gets a name, like everything else.
  out.append(option(0, tr("Not falling")));

  for (const auto& pair : AreaWarps::legalDungeonWarps()) {
    if (pair.first != map)
      continue;

    // ⚠️ 1-BASED. `IsPlayerOnDungeonWarp` writes `wCoordIndex`, which starts at 1 -- and Victory
    // Road 2F has a hole **2** and no hole 1, which is why this is a per-map question and not a
    // range.
    out.append(option(pair.second, tr("Hole %1").arg(pair.second)));
  }

  return out;
}

QVariantList MapModel::warpFields(int ind) const
{
  QVariantList ret;

  if (warps == nullptr || ind < 0 || ind >= warps->warpCount())
    return ret;

  WarpData* w = warps->warpAt(ind);
  if (w == nullptr)
    return ret;

  const QString where = tr("The warp");

  // X and Y are one fact, so they are one control -- the same `coords` kind the sprite panel uses.
  ret.append(field(where, "xy", tr("Standing on"),
                   tr("Which tile of this map is the warp, counting from the top-left."),
                   (w->x & 0xFF) | ((w->y & 0xFF) << 8), 0, 0, "coords"));

  const QString leads = tr("Where it leads");

  // The destination map is a REAL map picker, all 248, glitch ids labelled -- plus the one value
  // that is not a map at all.
  ret.append(field(leads, "destMap", tr("Takes you to"),
                   tr("Which map the warp opens onto.\n\n\"Back outside\" (255) is how every "
                      "building's exit works: it doesn't name a map, it sends you to whatever map "
                      "you last stood on outdoors — which you can see and change in the toolbar."),
                   w->destMap, 0, 255, "map"));

  ret.append(field(leads, "destWarp", tr("Arriving at warp #"),
                   tr("Which arrival point of that map you land on.\n\n⚠️ These are NOT the other "
                      "map's warps — the game keeps a separate list of landing spots, and this "
                      "counts into that. The console does not check it: point it past the end and "
                      "it reads whatever cartridge bytes come next and drops you somewhere undefined."),
                   w->destWarp, 0, 255, "byte"));

  return ret;
}

void MapModel::setWarpField(int ind, const QString& key, int value)
{
  if (warps == nullptr || ind < 0 || ind >= warps->warpCount())
    return;

  WarpData* w = warps->warpAt(ind);
  if (w == nullptr)
    return;

  if (key == "xy") {
    // Packed x | (y << 8), the same shape the sprite panel's `coords` control speaks.
    moveWarp(ind, value & 0xFF, (value >> 8) & 0xFF);
    return;
  }

  if (key == "destMap") {
    w->destMap = value & 0xFF;
    w->destMapChanged();
  } else if (key == "destWarp") {
    w->destWarp = value & 0xFF;
    w->destWarpChanged();
  } else {
    return;
  }

  warpsWereEdited = true;
  warps->warpsChanged();
}

QVariantList MapModel::warpStateFields() const
{
  QVariantList ret;

  if (warps == nullptr)
    return ret;

  // ⚠️ Same filter, same switch, same reason as the sprite panel's: a field the console rewrites on
  // load is not hidden because it is unimportant, it is hidden because it is CLUTTER above the
  // fields that do something. On, it appears wearing its mark. Filtered HERE, in the model, so no
  // view can leak one and a test can prove they are gone.
  auto add = [&](QVariantMap f) {
    if (!showScratchFields && (f.value("scratch").toBool() || f.value("dead").toBool()))
      return;

    ret.append(f);
  };

  /// Mark a field as ⚠️ REWRITTEN ON LOAD. Console-verified: `wStatusFlags3` shares an address with
  /// `wCableClubDestinationMap`, and `SpecialEnterMap` -- which is on the Continue path -- zeroes
  /// it. The WHOLE byte. Wrote `$FF`, read back `$00`.
  auto wiped = [](QVariantMap f) {
    f["scratch"] = true;
    f["mark"]    = QStringLiteral("wiped");
    return f;
  };

  /// Mark a field as 💀 DEAD. It survives the save perfectly -- and nothing in the entire game
  /// ever reads it. Two writes, zero reads, across the whole disassembly.
  ///
  /// ⚠️ A wiped byte and an unread byte are NOT the same fact, and lumping them together under one
  /// grey "unused" would be the kind of hand-wave this project does not do.
  auto dead = [](QVariantMap f) {
    f["dead"] = true;
    f["mark"] = QStringLiteral("dead");
    return f;
  };

  /**
   * Mark a field as 🔫 a value the console can be hurt by.
   *
   * ⚠️ **`legal` is a fact; `armed` is whether anything will READ it.** The red "!" fires only on
   * `!legal && armed`, and that distinction is not pedantry -- it is the difference between a useful
   * warning and noise, and noise is a bug.
   *
   * Here is the trap it exists to avoid, and the screenshot review walked straight into it: the
   * fixture save holds `dungeonWarpDestMap = 194` (Victory Road 2F) and `whichDungeonWarp = 0`. That
   * pair is not in `DungeonWarpList`, so a naive check screams. But **0 is the resting value** --
   * `IsPlayerOnDungeonWarp` writes 0 there as its very first instruction whenever you are *not*
   * standing on a hole -- so essentially **every save anyone has ever made** carries it, and
   * `BIT_DUNGEON_WARP` is off, so the console will never look at either byte.
   *
   * Flagging that is crying wolf on every save ever opened. It is exactly the mistake the sprite
   * "your cast has changed" warning made in its first cut (reference/sprites.md), and it gets the
   * same answer: **say the true thing, not the alarming one.**
   */
  auto gun = [](QVariantMap f, bool legal, bool armed) {
    f["gun"]   = true;
    f["legal"] = legal;
    f["armed"] = armed;
    return f;
  };

  // Will the console actually READ these bytes as things stand?
  //
  //   `PrepareForSpecialWarp` only runs at all when BIT_FLY_OR_DUNGEON_WARP is set -- and only then
  //   does it reach either lookup table. Which of the two it reaches is BIT_DUNGEON_WARP.
  const bool specialArmed = warps->flyOrDungeonWarp;
  const bool dungeonArmed = warps->flyOrDungeonWarp && warps->dungeonWarp;

  // ── Where the special warps go ─────────────────────────────────────────────────────────────
  const QString goes = tr("Where the special warps go");

  ret.append(field(goes, "lastBlackoutMap", tr("Wake up at"),
                   tr("Blacking out, digging out with DIG, and using an ESCAPE ROPE all bring you "
                      "here."),
                   lastBlackoutMap(), 0, 255, "map"));

  ret.append(gun(field(goes, "specialWarpDestMap", tr("Fly sends you to"),
                       tr("Where the last FLY (or other special warp) was headed.\n\n⚠️ Only 13 maps "
                          "are legal here — the game looks this up in a table that has no end marker "
                          "and no bounds check. Any other map and the console reads whatever ROM "
                          "bytes follow the table and drops you somewhere undefined."),
                       warps->specialWarpDestMap, 0, 255, "flyMap"),
                 AreaWarps::isLegalFlyMap(warps->specialWarpDestMap),
                 specialArmed));

  // ⚠️ The MAP and the HOLE are judged SEPARATELY, and the first cut got this wrong too: it failed
  // both fields whenever the *pair* was wrong, so a perfectly good map (Victory Road 2F) came up
  // flagged because the hole beside it was 0. Two fields, two questions.
  //
  //   the map  -> is it a map that has holes at all?
  //   the hole -> is (this map, this hole) actually in the table?
  //
  // And **0 means "not falling"** on both. It is the value the game itself writes when you are not
  // standing on a hole, so it is not a hack value -- it is the ordinary state of the world.
  const bool mapResting  = (warps->dungeonWarpDestMap == 0);
  const bool holeResting = (warps->whichDungeonWarp == 0);

  bool mapHasHoles = false;
  for (const auto& p : AreaWarps::legalDungeonWarps())
    if (p.first == warps->dungeonWarpDestMap)
      mapHasHoles = true;

  ret.append(gun(field(goes, "dungeonWarpDestMap", tr("Falling drops you onto"),
                       tr("The floor below, when you fall down a hole.\n\nOnly 7 maps in the game "
                          "have holes. ⚠️ Name any other and the console reads whatever cartridge "
                          "bytes follow its table — it never checks."),
                       warps->dungeonWarpDestMap, 0, 255, "dungeonMap"),
                 mapResting || mapHasHoles,
                 dungeonArmed));

  ret.append(gun(field(goes, "whichDungeonWarp", tr("…through hole #"),
                       tr("Which hole on the floor above you fell through.\n\nThe game counts these "
                          "from 1, not 0 — and they have to MATCH the floor: Seafoam B1F has holes 1 "
                          "and 2, but Victory Road 2F has a hole 2 and no hole 1.\n\n0 means “not "
                          "falling”, which is what it says on almost every save."),
                       warps->whichDungeonWarp, 0, 255, "dungeonHole"),
                 holeResting
                   || AreaWarps::isLegalDungeonWarp(warps->dungeonWarpDestMap,
                                                    warps->whichDungeonWarp),
                 dungeonArmed));

  ret.append(field(goes, "warpDest", tr("Arriving at warp #"),
                   tr("Which arrival point of the map you are entering you will land on.\n\n255 "
                      "means \"don't move me\" — every special warp sets that, because it has "
                      "already placed you itself."),
                   warps->warpDest, 0, 255, "byte"));

  // ── What kind of warp is happening ─────────────────────────────────────────────────────────
  const QString kind = tr("What kind of warp is happening");

  ret.append(field(kind, "flyOrDungeonWarp", tr("A special warp is in progress"),
                   tr("The switch the whole fly/hole/Dig path hangs off. With it off, the game "
                      "thinks you are starting a new game and warps you to Red's bedroom."),
                   warps->flyOrDungeonWarp ? 1 : 0, 0, 1, "flag"));

  ret.append(field(kind, "flyWarp", tr("Arrive with the drop-in animation"),
                   tr("How you land off a warp pad or a FLY — you drop in from above rather than "
                      "just appearing."),
                   warps->flyWarp ? 1 : 0, 0, 1, "flag"));

  ret.append(field(kind, "dungeonWarp", tr("You fell down a hole"),
                   tr("Sends the destination lookup to the hole table instead of the fly table."),
                   warps->dungeonWarp ? 1 : 0, 0, 1, "flag"));

  ret.append(field(kind, "escapeWarp", tr("Dig / Escape Rope / blacked out"),
                   tr("Sends you to \"Wake up at\", above.\n\n(The editor used to call this "
                      "\"blackout destination\" and treat it as a mystery flag. It is neither a "
                      "destination nor a mystery.)"),
                   warps->escapeWarp ? 1 : 0, 0, 1, "flag"));

  ret.append(field(kind, "forcedWarp", tr("Warps fire without walking into them"),
                   tr("Normally, stepping onto a warp does nothing unless you are actually holding "
                      "a direction — you have to walk INTO it. This removes that check, so touching "
                      "the tile is enough.\n\nIt is how the Seafoam Islands current sweeps you along."),
                   warps->forcedWarp ? 1 : 0, 0, 1, "flag"));

  // ── ⚠️💀 The ones that do nothing — behind the switch ────────────────────────────────────────
  //
  // TWO DIFFERENT KINDS OF NOTHING, and the panel says which is which.
  const QString nothing = tr("Fields that do nothing");

  add(wiped(field(nothing, "scriptedWarp", tr("A script is warping you right now"),
                  tr("Tells the game to warp you immediately, with no warp tile needed. Exactly one "
                     "script in the whole game uses it (Mr. Fuji carrying you out of Pokémon "
                     "Tower).\n\n⚠️ The console ZEROES this byte every single time it loads a save "
                     "— it shares an address with a cable-club field that gets cleared on the way "
                     "in. Set it, save, load: it is gone. Verified on a real cartridge."),
                  warps->scriptedWarp ? 1 : 0, 0, 1, "flag")));

  add(wiped(field(nothing, "isDungeonWarp", tr("Standing on a hole"),
                  tr("Stops wild Pokémon attacking you mid-fall.\n\n⚠️ Same byte as above, and so "
                     "the same fate: zeroed on every save load."),
                  warps->isDungeonWarp ? 1 : 0, 0, 1, "flag")));

  add(dead(field(nothing, "warpedFromWarp", tr("Came in through warp #"),
                 tr("The game writes this every time you use a warp — and then never reads it "
                    "again.\n\n💀 Nothing anywhere in the game looks at it. It survives being saved "
                    "perfectly; it simply does not do anything. Change it freely, and change nothing."),
                 warps->warpedFromWarp, 0, 255, "byte")));

  add(dead(field(nothing, "warpedfromMap", tr("Came from map"),
                 tr("Same story: written on every warp, read by nothing.\n\n💀 If you are looking "
                    "for the map a warp sends you back to, that is \"Outside is…\" in the toolbar "
                    "— a different byte entirely, and a real one."),
                 warps->warpedfromMap, 0, 255, "map")));

  return ret;
}

void MapModel::setWarpStateField(const QString& key, int value)
{
  if (warps == nullptr)
    return;

  if (key == "lastBlackoutMap") {
    setLastBlackoutMap(value);
    return;
  }

  // Every one of these writes its own byte (or its own bit) and nothing else. tst_warps byte-diffs
  // the save across each of them.
  if (key == "specialWarpDestMap") {
    warps->specialWarpDestMap = value & 0xFF;
    warps->specialWarpDestMapChanged();
  } else if (key == "dungeonWarpDestMap") {
    warps->dungeonWarpDestMap = value & 0xFF;
    warps->dungeonWarpDestMapChanged();
  } else if (key == "whichDungeonWarp") {
    warps->whichDungeonWarp = value & 0xFF;
    warps->whichDungeonWarpChanged();
  } else if (key == "warpDest") {
    warps->warpDest = value & 0xFF;
    warps->warpDestChanged();
  } else if (key == "flyOrDungeonWarp") {
    warps->flyOrDungeonWarp = (value != 0);
    warps->flyOrDungeonWarpChanged();
  } else if (key == "flyWarp") {
    warps->flyWarp = (value != 0);
    warps->flyWarpChanged();
  } else if (key == "dungeonWarp") {
    warps->dungeonWarp = (value != 0);
    warps->dungeonWarpChanged();
  } else if (key == "escapeWarp") {
    warps->escapeWarp = (value != 0);
    warps->escapeWarpChanged();
  } else if (key == "forcedWarp") {
    warps->forcedWarp = (value != 0);
    warps->forcedWarpChanged();
  } else if (key == "scriptedWarp") {
    warps->scriptedWarp = (value != 0);
    warps->scriptedWarpChanged();
  } else if (key == "isDungeonWarp") {
    warps->isDungeonWarp = (value != 0);
    warps->isDungeonWarpChanged();
  } else if (key == "warpedFromWarp") {
    warps->warpedFromWarp = value & 0xFF;
    warps->warpedFromWarpChanged();
  } else if (key == "warpedfromMap") {
    warps->warpedfromMap = value & 0xFF;
    warps->warpedfromMapChanged();
  } else {
    return;
  }

  emit warpsChanged();
}

// ── The player's 26-byte map-state block ─────────────────────────────────────────────────────────
//
// ⚠️ notes/reference/player-state.md, verified on the cartridge (scripts/emu/probe_player_state.py).
// The durable fields show in named groups; the ten the game rewrites on load and the three it never
// reads are gathered behind the same "Reloaded values" switch the sprite and warp panels use.
QVariantList MapModel::playerFields() const
{
  QVariantList ret;

  if (player == nullptr)
    return ret;

  // Same filter, same switch, same reason as the sprite and warp panels: a field the console
  // rewrites on load is CLUTTER above the ones that do something. Filtered HERE, in the model, so no
  // view can leak one and a test can prove they are gone.
  auto add = [&](QVariantMap f) {
    if (!showScratchFields && (f.value("scratch").toBool() || f.value("dead").toBool()))
      return;
    ret.append(f);
  };

  // ⚠️ REWRITTEN ON LOAD -- console-verified. Unlike the warp block (where one wipe of wStatusFlags3
  // does them all), the player bytes are rewritten by DIFFERENT routines -- forced, zeroed, reset,
  // cleared -- so each carries its OWN explanation rather than a shared one.
  auto reload = [](QVariantMap f, const QString& note) {
    f["scratch"] = true;
    f["mark"]    = QStringLiteral("reload");
    f["note"]    = note;
    return f;
  };

  // 💀 DEAD -- survives a save perfectly, and nothing in the game ever reads it.
  auto dead = [](QVariantMap f, const QString& note) {
    f["dead"] = true;
    f["mark"] = QStringLiteral("dead");
    f["note"] = note;
    return f;
  };

  // The facing directions store the game's RAW bit values (PLAYER_DIR_*), which is why they are not
  // 0..4. A value that is none of these stays fully editable -- the combo just steps aside for a raw
  // box (PlayerField.qml), the same doctrine as everywhere else.
  const QVariantList dirs = {
    option(0, tr("— not moving")),
    option(1, tr("Right")),
    option(2, tr("Left")),
    option(4, tr("Down")),
    option(8, tr("Up")),
  };

  // ── Facing & movement ──────────────────────────────────────────────────────────────────────
  const QString move = tr("Facing & movement");

  ret.append(field(move, "moveDir", tr("Moving"),
                   tr("Which way he is stepping right now. It is 0 on any save you could really "
                      "make — you can only save while standing still."),
                   player->playerMoveDir, 0, 255, "enum", dirs));

  ret.append(field(move, "lastStopDir", tr("Last stop"),
                   tr("The way he was facing before he last stopped walking."),
                   player->playerLastStopDir, 0, 255, "enum", dirs));

  ret.append(field(move, "walkBikeSurf", tr("Getting around by"),
                   tr("Walking, cycling or surfing. Some maps override it — the Cycling Road puts "
                      "you on the bike, the Seafoam currents put you in the water."),
                   player->walkBikeSurf, 0, 255, "enum",
                   QVariantList{ option(0, tr("Walking")), option(1, tr("Biking")),
                                 option(2, tr("Surfing")) }));

  // ── Fine position (the half-block) ──────────────────────────────────────────────────────────
  const QString fine = tr("Fine position");

  ret.append(field(fine, "xBlockCoord", tr("Half-block across"),
                   tr("Which half of the 2×2 block he stands in, left to right. 0 or 1."),
                   player->xBlockCoord, 0, 255, "byte"));

  ret.append(field(fine, "yBlockCoord", tr("Half-block down"),
                   tr("Which half of the 2×2 block he stands in, top to bottom. 0 or 1."),
                   player->yBlockCoord, 0, 255, "byte"));

  // ── What he can do here ─────────────────────────────────────────────────────────────────────
  const QString may = tr("What he can do here");

  ret.append(field(may, "surfingAllowed", tr("Can surf from here"),
                   tr("Set when the tile he faces is water he could surf onto. The game works it "
                      "out again as he walks."),
                   player->surfingAllowed ? 1 : 0, 0, 1, "flag"));

  ret.append(field(may, "arrivedByFly", tr("Arrived by Fly"),
                   tr("“He just landed by FLY — play the drop-in animation.”\n\n(v1 called this "
                      "“Using Fly”. It isn't that — it does not mean FLY is usable here.)"),
                   player->flyOutofBattle ? 1 : 0, 0, 1, "flag"));

  // ── Battle ──────────────────────────────────────────────────────────────────────────────────
  const QString battle = tr("Battle");

  ret.append(field(battle, "noBattles", tr("No wild battles"),
                   tr("Suppresses wild Pokémon entirely — a debug/script flag, and a real, durable "
                      "one that survives the save."),
                   player->noBattles ? 1 : 0, 0, 1, "flag"));

  // ── Standing on ─────────────────────────────────────────────────────────────────────────────
  const QString standing = tr("Standing on");

  ret.append(field(standing, "standingOnWarp", tr("A warp tile"),
                   tr("He is on a warp tile. The game recomputes this as he moves, so it survives "
                      "the load but not the first step."),
                   player->standingOnWarp ? 1 : 0, 0, 1, "flag"));

  ret.append(field(standing, "spinPlayer", tr("A spin tile"),
                   tr("The spinning-floor tiles in the Rocket hideout and the Viridian Gym."),
                   player->spinPlayer ? 1 : 0, 0, 1, "flag"));

  // ── ⚠️💀 Rewritten on load, or never read — behind the switch ────────────────────────────────
  //
  // Twilight, 2026-07-14: *"it would be wonderful to know which ones were regenerated or rewritten
  // on save load with little exclamation points grouped below and hidden behind a switch."* This is
  // that. Ten the game rewrites, three it never reads -- and they are DIFFERENT facts, said apart.
  const QString nothing = tr("Rewritten on load, or never read");

  add(reload(field(nothing, "curDir", tr("Current direction"),
                   tr("Which way he is facing."),
                   player->playerCurDir, 0, 255, "enum", dirs),
             tr("The game FORCES this to DOWN every single time it loads your save — it is the first "
                "thing Continue does. Set it, save, load: he faces down. Verified on a real "
                "cartridge.")));

  add(reload(field(nothing, "strengthOutsideBattle", tr("Using Strength"),
                   tr("STRENGTH is active out in the overworld, so boulders can be pushed."),
                   player->strengthOutsideBattle ? 1 : 0, 0, 1, "flag"),
             tr("The game clears this on every ordinary map load — unless “Battle just ended” is "
                "set, in which case it survives. Both paths verified on a real cartridge.")));

  add(reload(field(nothing, "isBattle", tr("Battle ongoing"),
                   tr("Its real name is “talked to a trainer” (BIT_TALKED_TO_TRAINER). v1 called it "
                      "“battle ongoing”, which it isn't."),
                   player->isBattle ? 1 : 0, 0, 1, "flag"),
             tr("Zeroed on every save load — this byte is shared with a cable-club field the game "
                "clears on the way in. Verified on a real cartridge (wrote 255, read back 0).")));

  add(reload(field(nothing, "isTrainerBattle", tr("Trainer battle"),
                   tr("Its real name is “print end-of-battle text” (BIT_PRINT_END_BATTLE_TEXT). "
                      "Another v1 misnomer."),
                   player->isTrainerBattle ? 1 : 0, 0, 1, "flag"),
             tr("The same byte as “Battle ongoing”, so the same fate: zeroed on every save load.")));

  add(reload(field(nothing, "battleEndedOrBlackout", tr("Battle just ended / blacked out"),
                   tr("Set the instant a battle finishes or you black out. It is what decides "
                      "whether STRENGTH survives the next map load."),
                   player->battleEndedOrBlackout ? 1 : 0, 0, 1, "flag"),
             tr("The game clears this every time it enters a map — including the one Continue loads. "
                "Console-verified.")));

  add(reload(field(nothing, "usingLinkCable", tr("Link cable connected"),
                   tr("A trade or battle link is live."),
                   player->usingLinkCable ? 1 : 0, 0, 1, "flag"),
             tr("Cleared on load — you are never in a link when you press Continue. "
                "Console-verified.")));

  add(reload(field(nothing, "standingOnDoor", tr("Standing on a door"),
                   tr("He is on a door tile."),
                   player->standingOnDoor ? 1 : 0, 0, 1, "flag"),
             tr("Cleared on load. Console-verified.")));

  add(reload(field(nothing, "movingThroughDoor", tr("Walking through a door"),
                   tr("He is mid door-transition."),
                   player->movingThroughDoor ? 1 : 0, 0, 1, "flag"),
             tr("Cleared on load. Console-verified.")));

  add(reload(field(nothing, "finalLedgeJumping", tr("Hopping a ledge / fishing"),
                   tr("Its real name is BIT_LEDGE_OR_FISHING — set during a ledge hop AND while "
                      "fishing. v1 narrowed it to “final ledge jump”."),
                   player->finalLedgeJumping ? 1 : 0, 0, 1, "flag"),
             tr("Cleared on load — the game runs its mid-jump handler once and switches it off. "
                "Console-verified.")));

  add(reload(field(nothing, "jumpingY", tr("Ledge-hop height"),
                   tr("How far into a ledge hop his sprite is — an animation index."),
                   player->playerJumpingYScrnCoords, 0, 255, "byte"),
             tr("Zeroed on load. Console-verified.")));

  add(dead(field(nothing, "usedCardKey", tr("Used the Card Key"),
                 tr("Its real name is BIT_UNUSED_CARD_KEY. The game sets it when you use a Card "
                    "Key — and the source annotates that very write “; never checked”."),
                 player->usedCardKey ? 1 : 0, 0, 1, "flag"),
           tr("The game sets this and never once reads it. It survives being saved perfectly; it "
              "simply does nothing.")));

  add(dead(field(nothing, "xOffsetSpecialWarp", tr("X offset since special warp"),
                 tr("Meant to track how far he has moved since the last special warp."),
                 player->xOffsetSinceLastSpecialWarp, 0, 255, "byte"),
           tr("The disassembly's own comment: “they don't seem to be used for anything.” Written "
              "by the game, read by nothing.")));

  add(dead(field(nothing, "yOffsetSpecialWarp", tr("Y offset since special warp"),
                 tr("The Y half of the same unused pair."),
                 player->yOffsetSinceLastSpecialWarp, 0, 255, "byte"),
           tr("Same story: written, never read.")));

  return ret;
}

void MapModel::setPlayerField(const QString& key, int value)
{
  if (player == nullptr)
    return;

  // ⚠️ Each key writes EXACTLY ONE member -- one byte, or (for the flags) one bool that
  // AreaPlayer::save flattens with a bit-preserving setBit, so the other bits of a shared status
  // byte are never touched. tst_player byte-diffs the whole 32 KB save across each of these.
  const int b = value & 0xFF;
  const bool on = (value != 0);

  if      (key == "moveDir")              { player->playerMoveDir = b;            player->playerMoveDirChanged(); }
  else if (key == "lastStopDir")          { player->playerLastStopDir = b;        player->playerLastStopDirChanged(); }
  else if (key == "curDir")               { player->playerCurDir = b;             player->playerCurDirChanged(); }
  else if (key == "walkBikeSurf")         { player->walkBikeSurf = b;             player->walkBikeSurfChanged(); }
  else if (key == "xBlockCoord")          { player->xBlockCoord = b;              player->xBlockCoordChanged(); }
  else if (key == "yBlockCoord")          { player->yBlockCoord = b;              player->yBlockCoordChanged(); }
  else if (key == "jumpingY")             { player->playerJumpingYScrnCoords = b; player->playerJumpingYScrnCoordsChanged(); }
  else if (key == "surfingAllowed")       { player->surfingAllowed = on;          player->surfingAllowedChanged(); }
  else if (key == "arrivedByFly")         { player->flyOutofBattle = on;          player->flyOutofBattleChanged(); }
  else if (key == "strengthOutsideBattle"){ player->strengthOutsideBattle = on;   player->strengthOutsideBattleChanged(); }
  else if (key == "noBattles")            { player->noBattles = on;               player->noBattlesChanged(); }
  else if (key == "isBattle")             { player->isBattle = on;                player->isBattleChanged(); }
  else if (key == "isTrainerBattle")      { player->isTrainerBattle = on;         player->isTrainerBattleChanged(); }
  else if (key == "battleEndedOrBlackout"){ player->battleEndedOrBlackout = on;   player->battleEndedOrBlackoutChanged(); }
  else if (key == "usingLinkCable")       { player->usingLinkCable = on;          player->usingLinkCableChanged(); }
  else if (key == "standingOnWarp")       { player->standingOnWarp = on;          player->standingOnWarpChanged(); }
  else if (key == "standingOnDoor")       { player->standingOnDoor = on;          player->standingOnDoorChanged(); }
  else if (key == "movingThroughDoor")    { player->movingThroughDoor = on;       player->movingThroughDoorChanged(); }
  else if (key == "spinPlayer")           { player->spinPlayer = on;              player->spinPlayerChanged(); }
  else if (key == "finalLedgeJumping")    { player->finalLedgeJumping = on;       player->finalLedgeJumpingChanged(); }
  else if (key == "usedCardKey")          { player->usedCardKey = on;             player->usedCardKeyChanged(); }
  else if (key == "xOffsetSpecialWarp")   { player->xOffsetSinceLastSpecialWarp = b; player->xOffsetSinceLastSpecialWarpChanged(); }
  else if (key == "yOffsetSpecialWarp")   { player->yOffsetSinceLastSpecialWarp = b; player->yOffsetSinceLastSpecialWarpChanged(); }
  else
    return;

  // He affects the render (his facing draws his sprite), and the Details panel refreshes off this.
  emit changed();
}

QVariantMap MapModel::zoomTarget(const QString& kind)
{
  const int border = MapEngine::mapBorder * MapEngine::blockPx;

  if (kind == "player")
    return target(true,
                  border + playerX() * 16 + 8,
                  border + playerY() * 16 + 8,
                  tr("the player"));

  if (kind == "sprite" || kind == "object") {
    // An "object" is a Pokéball, a boulder, a fossil -- a sprite whose ARTWORK is in the Objects
    // group. Everything else is somebody.
    QVariantList pool;
    for (const QVariant& v : npcList()) {
      const QVariantMap m = v.toMap();

      SpriteDBEntry* e = SpritesDB::inst()->getIndAt(QString::number(m.value("picture").toInt()));
      const bool isObject = (e != nullptr && e->group == QStringLiteral("Objects"));

      if (isObject == (kind == "object"))
        pool.append(m);
    }

    if (pool.isEmpty())
      return target(false);

    const QVariantMap pick = pool.at(Random::inst()->rangeExclusive(0, pool.size())).toMap();
    return target(true,
                  pick.value("rectX").toInt() + 8,
                  pick.value("rectY").toInt() + 8,
                  pick.value("name").toString());
  }

  if (kind == "warp") {
    const QVariantList pts = warpPoints();
    if (pts.isEmpty())
      return target(false);

    const QVariantMap pick = pts.at(Random::inst()->rangeExclusive(0, pts.size())).toMap();
    return target(true, pick.value("x").toInt(), pick.value("y").toInt(),
                  tr("warp %1").arg(pick.value("ind").toInt() + 1));
  }

  if (kind == "door") {
    const MapEngine::Buffer buf = MapEngine::buildOverworldMap(mapInd(), borderBlock());
    const QVector<QPoint> doors =
        MapEngine::tilesInLayer(buf, tilesetInd(), MapEngine::LayerDoors, saveTilesOf(tileset));

    if (doors.isEmpty())
      return target(false);

    const QPoint p = doors.at(Random::inst()->rangeExclusive(0, doors.size()));
    return target(true, p.x() + 4, p.y() + 4, tr("a door"));
  }

  if (kind == "connection") {
    const auto strips = MapEngine::connectionStrips(mapInd());
    if (strips.isEmpty())
      return target(false);

    const MapEngine::Strip s = strips.at(Random::inst()->rangeExclusive(0, strips.size()));
    return target(true,
                  (s.bx + s.cols / 2.0) * MapEngine::blockPx,
                  (s.by + s.rows / 2.0) * MapEngine::blockPx,
                  s.name);
  }

  if (kind == "xy") {
    // The centre of a random BLOCK -- not a tile and not a pixel (Twilight). A block is the unit a
    // map is actually built out of, so it is the unit that is worth landing on.
    if (blocksWide() <= 0 || blocksHigh() <= 0)
      return target(false);

    const int bx = Random::inst()->rangeExclusive(0, blocksWide());
    const int by = Random::inst()->rangeExclusive(0, blocksHigh());

    return target(true,
                  border + bx * MapEngine::blockPx + MapEngine::blockPx / 2,
                  border + by * MapEngine::blockPx + MapEngine::blockPx / 2,
                  tr("block %1, %2").arg(bx).arg(by));
  }

  return target(false);
}

QVariantMap MapModel::zoomTargetsAvailable() const
{
  QVariantMap m;

  m["player"] = true;
  m["camera"] = true;
  m["map"]    = true;
  m["xy"]     = blocksWide() > 0 && blocksHigh() > 0;

  int people = 0, objects = 0;
  for (const QVariant& v : npcList()) {
    SpriteDBEntry* e = SpritesDB::inst()->getIndAt(
        QString::number(v.toMap().value("picture").toInt()));

    if (e != nullptr && e->group == QStringLiteral("Objects"))
      objects++;
    else
      people++;
  }

  m["sprite"] = people > 0;
  m["object"] = objects > 0;
  m["warp"]   = !warpPoints().isEmpty();
  m["connection"] = !MapEngine::connectionStrips(mapInd()).isEmpty();

  const MapEngine::Buffer buf = MapEngine::buildOverworldMap(mapInd(), borderBlock());
  m["door"] = MapEngine::layerApplies(buf, tilesetInd(), MapEngine::LayerDoors, saveTilesOf(tileset));

  return m;
}

namespace {

// ── The Details panel's field schema ─────────────────────────────────────────────────────────────
//
// ⚠️ REWRITTEN 2026-07-13. The first version emitted one row per byte, each a number box with a
// paragraph next to it, under headings called "Who", "Where" and "When". Twilight, verbatim:
//
//   > *"the Who When Where is really really dumb, don't do it. The fields are all just raw values —
//   > exactly what I said not to do. I don't know what most of those numbers mean on sprite details,
//   > it's cryptic as crap. It should be intuitive without having to read text for something like
//   > this. Don't show boxes if it's unnecessary — some of these raw values have a combo box next to
//   > them and I bet that combo box value is going to determine if the textbox raw value is even
//   > needed to be there or not."*
//
// She is right, and that last sentence is the design. So a field now declares a KIND, and the kind
// decides the control:
//
//   picture      -- a grid of the actual ARTWORK. You pick a character by looking at them.
//   coords       -- X and Y together, in ONE control. They are one fact.
//   enum         -- a combo. The raw byte box appears ONLY if the value is not one of the named ones
//                   (i.e. a hack value) -- because then, and only then, is there anything to type.
//   frames       -- a countdown in FRAMES. Shown as what it is: a duration. Not a number.
//   kind         -- plain NPC / trainer / item ball. It is bits 6-7 of the text byte, and it decides
//                   which of the three fields below even EXIST.
//   text         -- the map's REAL scripts, by name, out of the map's own ROM data.
//   item         -- the real item list.
//   trainerClass -- the real trainer classes;  trainerTeam -- which of that class's rosters.
//   byte         -- the last resort. If you are reaching for this, ask whether the field has a kind.
//
// `scratch` marks a byte the console recomputes when it loads the save. It gets a yellow "!".
// Not hidden, not refused, not silently normalised -- just labelled, so nobody spends an afternoon
// setting a value the game throws away. (Twilight: "animation scratch ... needs to be explained --
// a yellow exclamation point next to it, when moused over, would say it's reloaded on game load.")

// (The default arguments live on the forward declaration at the top of the file -- the warp fields
//  sit above this point and share the kit, and a default may only be given once.)
QVariantMap field(const QString& group, const QString& key, const QString& label,
                  const QString& blurb, int value, int min, int max,
                  const QString& kind,
                  const QVariantList& options,
                  bool scratch)
{
  QVariantMap m;
  m["group"]   = group;
  m["key"]     = key;
  m["label"]   = label;
  m["blurb"]   = blurb;
  m["value"]   = value;
  m["min"]     = min;
  m["max"]     = max;
  m["kind"]    = kind;
  m["options"] = options;
  m["scratch"] = scratch;
  return m;
}

/// One named value of an enum field. `hack` marks a value no real game would hold -- shown and
/// selectable, flagged in words, never refused.
QVariantMap option(int value, const QString& name, bool hack)
{
  QVariantMap m;
  m["value"] = value;
  m["name"]  = name;
  m["hack"]  = hack;
  return m;
}

/// Sort an option list into TWO SECTIONS -- the clean values first, the flagged ones after -- and put
/// a heading on the first row of each.
///
/// ⚠️ Twilight, 2026-07-13: *"If it has an exclamation on it, it probably needs a group above it that
/// has those without. I don't want duplicates above — I think there's so many of them they get lost.
/// This needs to be organised better."*
///
/// She is right: the item list is 100+ entries with the glitch items scattered through it, and a "!"
/// on row 84 is a "!" nobody sees. Two sections and the mark means something again. **Order is
/// otherwise preserved** -- these lists are in the game's own order and that is worth keeping.
QVariantList sectioned(const QVariantList& options,
                       const QString& cleanHeading, const QString& flaggedHeading)
{
  QVariantList clean;
  QVariantList flagged;

  for (const QVariant& v : options) {
    const QVariantMap m = v.toMap();
    (m.value("hack").toBool() ? flagged : clean).append(m);
  }

  // The heading rides on the FIRST row of its section -- one model, one list, nothing to drift.
  if (!clean.isEmpty()) {
    QVariantMap first = clean.first().toMap();
    first["header"] = cleanHeading;
    clean[0] = first;
  }

  if (!flagged.isEmpty()) {
    QVariantMap first = flagged.first().toMap();
    first["header"] = flaggedHeading;
    flagged[0] = first;
  }

  return clean + flagged;
}

// ── The text byte: three facts in one byte ───────────────────────────────────────────────────────
//
//     const_def 6
//     const BIT_TRAINER   ; 6
//     const BIT_ITEM      ; 7
//     DEF TRAINER EQU 1 << BIT_TRAINER
//     DEF ITEM    EQU 1 << BIT_ITEM      -- constants/map_object_constants.asm
//
// and the map macro that writes it:
//
//     IF _NARG > 7  : db TRAINER | \6 : db \7 : db \8    ; class, roster
//     ELIF _NARG > 6: db ITEM    | \6 : db \7            ; item id
//     ELSE          : db \6                              ; just talks
//                                                        -- macros/scripts/maps.asm
//
// So the TOP TWO BITS say what this character IS, and the bottom six are the script id. That is why
// the Details panel can hide the item picker from an NPC and the trainer roster from a Pokéball: the
// save itself already knows which of them is meaningless.
constexpr int TextBitTrainer = 0x40;
constexpr int TextBitItem    = 0x80;

/// What a sprite IS, out of the top two bits of its text byte.
enum SpriteKind { KindPlain = 0, KindTrainer = 1, KindItem = 2, KindBoth = 3 };

int kindOf(int textByte)
{
  const bool t = (textByte & TextBitTrainer) != 0;
  const bool i = (textByte & TextBitItem) != 0;

  if (t && i) return KindBoth;    // ⚠️ a real game never writes this. It is reachable, so it is shown.
  if (t)      return KindTrainer;
  if (i)      return KindItem;
  return KindPlain;
}

} // namespace

// ── Where the panel's real-world values come from ────────────────────────────────────────────────

QVariantList MapModel::mapTextList() const
{
  QVariantList ret;

  MapDBEntry* m = MapsDB::inst()->getStoreAt(mapInd());
  if (m == nullptr)
    return ret;

  // The map's OWN scripts, out of the cartridge -- who each one belongs to. A text id is an index
  // into this map's text-pointer table, so "Text 3" means nothing on its own; "Text 3 — Fisher 2"
  // is a thing you can actually choose. (Twilight: "Text id needs to reference whatever it's
  // supposed to... it needs to show real data.")
  QMap<int, QString> named;

  for (int i = 0; i < m->getSpritesSize(); i++) {
    const MapDBEntrySprite* s = m->getSpritesAt(i);
    if (s == nullptr)
      continue;

    const int id = s->getText();
    if (id <= 0)
      continue;

    // Two characters can share a script (the game does it). Say so rather than picking a winner.
    named[id] = named.contains(id) ? tr("%1, %2").arg(named[id], s->getSprite())
                                   : s->getSprite();
  }

  for (int i = 0; i < m->getSignsSize(); i++) {
    const MapDBEntrySign* s = m->getSignsAt(i);
    if (s == nullptr)
      continue;

    const int id = s->getTextID();
    if (id <= 0)
      continue;

    named[id] = named.contains(id) ? tr("%1, a sign").arg(named[id]) : tr("a sign");
  }

  // ⚠️ ONLY THE SCRIPTS THIS MAP REALLY HAS.
  //
  // It used to offer all 64 ids, with the 50-odd unused ones listed as "this map has no script 37".
  // That is fifty rows of nothing, and they buried the handful that mean something. Twilight:
  // *"Empty sign script slots need to rely on Something else."* -- and they do: the raw box is one
  // click away and reaches every one of the 64, so nothing is lost except the noise.
  ret.append(option(0, tr("Nothing to say")));

  for (auto it = named.constBegin(); it != named.constEnd(); ++it)
    ret.append(option(it.key(), tr("%1 — %2").arg(it.key()).arg(it.value())));

  return ret;
}

QVariantList MapModel::itemList() const
{
  QVariantList ret;

  for (int i = 0; i < ItemsDB::inst()->getStoreSize(); i++) {
    ItemDBEntry* e = ItemsDB::inst()->getStoreAt(i);
    if (e == nullptr)
      continue;

    // The glitch items are real bytes an item ball can hold, so they are offered -- and flagged.
    ret.append(option(e->getInd(), e->getReadable(), e->getGlitch()));
  }

  return ret;
}

QVariantList MapModel::trainerClassList() const
{
  QVariantList ret;

  for (int i = 0; i < TrainersDB::inst()->getStoreSize(); i++) {
    TrainerDBEntry* e = TrainersDB::inst()->getStoreAt(i);
    if (e == nullptr)
      continue;

    ret.append(option(int(e->ind), e->name, e->unused));
  }

  return ret;
}

QVariantList MapModel::npcFields(int slot) const
{
  QVariantList ret;

  if (npcs == nullptr || slot < 1 || slot >= npcs->spriteCount())
    return ret;

  SpriteData* s = npcs->spriteAt(slot);
  if (s == nullptr)
    return ret;

  // ⚠️ THE SCRATCH FIELDS ARE **ABSENT**, NOT GREYED, WHEN THE SWITCH IS OFF.
  //
  // Roughly a third of a sprite is bytes the console works out again the moment it loads the save --
  // the walk state, the on-screen pixels, the VRAM slot. Every one is real and every one is editable,
  // and the toolbar's "Reloaded values" switch turns them on. It is **off by default** (Twilight):
  //
  //   *"When it's off, the fields that relate to things there's no point in changing will not be
  //    present and add clutter."*
  //
  // So they do not get built at all. It is filtered HERE, in the model, and not in the panel -- so
  // no view can accidentally leak one, and a test can prove they are gone.
  auto add = [&](const QVariantMap& f) {
    if (!showScratchFields && f.value("scratch").toBool())
      return;

    ret.append(f);
  };

  // ── Character ──────────────────────────────────────────────────────────────────────────────
  //
  // A PICKER, with the artwork in it. You choose a character by looking at them, not by knowing
  // that 37 is a Fisherman. (Twilight: "If you need to select a picture, have a menu to select
  // from pictures.")
  const QString character = tr("Character");

  add(field(character, "pictureID", tr("Picture"), QString(),
                   s->pictureID, 0, 255, "picture"));

  // ── Where ──────────────────────────────────────────────────────────────────────────────────
  //
  // X and Y are ONE fact, so they are one control (Twilight: "x and y can probably be grouped into
  // 1 box"). The +4 bias comes off here and goes back on in setNpcField -- one conversion, one place.
  add(field(tr("Where"), "mapXY", tr("Standing at"),
                   tr("Where on the map, in tiles, counting from the top-left. (The save keeps "
                      "these with the game's +4 bias on them; we take it off, so this is what a "
                      "player would count.)"),
                   ((s->mapX - 4) & 0xFF) | (((s->mapY - 4) & 0xFF) << 8), 0, 0, "coords"));

  // ── Movement ───────────────────────────────────────────────────────────────────────────────
  //
  // Two different bytes in two different tables, and telling them apart is the whole of the
  // 2026-07-13 research. See notes/reference/npc-movement.md.
  const QString movement = tr("Movement");

  const QVariantList mobility = {
    option(0xFF, tr("Stand still")),
    option(0xFE, tr("Wander")),
    option(0x00, tr("Walk through walls"), true),
  };
  add(field(movement, "movementByte", tr("What they do"),
                   tr("The game has exactly two words for this: WALK and STAY. Somebody who STAYS "
                      "is not frozen, though — they still TURN to face you. Any other value lets "
                      "them move with no collision detection at all."),
                   s->movementByte, 0, 255, "enum", mobility));

  const QVariantList movement2 = {
    option(0x00, tr("Anywhere")),
    option(0x01, tr("Up and down only")),
    option(0x02, tr("Left and right only")),
    option(0x10, tr("A boulder — Strength pushes them")),
    option(0xD0, tr("Facing down")),
    option(0xD1, tr("Facing up")),
    option(0xD2, tr("Facing left")),
    option(0xD3, tr("Facing right")),
    option(0xFF, tr("Nowhere at all")),
  };
  // ⚠️ *"Isn't wander supposed to let you pick how far they can walk?"* -- Twilight. The honest answer
  // is **no**, and it is worth saying out loud rather than leaving her to wonder.
  //
  // This byte picks an AXIS and nothing else: anywhere / up-and-down / left-and-right. There IS a
  // distance limit in the game -- `yDisp` and `xDisp` -- but it is not map data, it is live state
  // (it sits under "Right now", behind the reloaded-values switch), the game always initialises it
  // to **8**, and **the mechanism is bugged**: it only ever checks one end of the range, so a sprite
  // can drift off the other way forever. We keep the bug, because it is the game.
  add(field(movement, "rangeDirByte", tr("Where they may go"),
                   tr("Which way they are allowed to wander — and that is ALL this picks. There is "
                      "no \"how far\": the game's distance limit is a separate value, it is always 8, "
                      "and it is bugged (it only checks one side, so they can drift off the other "
                      "way forever).\n\nFor somebody who doesn't walk, this same byte fixes which way "
                      "they stand."),
                   s->getRangeDirByte(), 0, 255, "enum", movement2));

  const QVariantList facings = {
    option(0x0, tr("Down")),
    option(0x4, tr("Up")),
    option(0x8, tr("Left")),
    option(0xC, tr("Right")),
  };
  add(field(movement, "faceDir", tr("Facing"),
                   tr("Which way they are drawn right now. There is no right-facing artwork in the "
                      "game for anybody — facing right is facing LEFT, mirrored."),
                   s->faceDir, 0, 255, "enum", facings));

  add(field(movement, "origFacingDir", tr("Were facing, before they turned to you"),
                   tr("The game backs the facing up here when somebody turns to talk, and puts it "
                      "back when the text box closes."),
                   s->origFacingDir, 0, 255, "enum", facings, true));

  const QVariantList grass = {
    option(0x00, tr("On open ground")),
    option(0x80, tr("In tall grass — it draws over their legs")),
  };
  add(field(movement, "grassPriority", tr("Standing in"),
                   tr("Makes the game draw the grass OVER their lower half, so they look like they "
                      "are standing in it rather than on it."),
                   s->grassPriority, 0, 255, "enum", grass));

  // ── Talking to it ──────────────────────────────────────────────────────────────────────────
  //
  // ⚠️ THE KIND IS THE TEXT BYTE'S TOP TWO BITS, and it decides which of the fields below exist at
  // all. A Pokéball has no trainer roster; a Bug Catcher has no item. Neither of them should be
  // looking at a box for one. THIS is what Twilight meant by "the combo box value is going to
  // determine if the raw textbox is even needed to be there or not".
  const QString talk = tr("Talking to it");
  const int textByte = s->getTextID();
  const int kind = kindOf(textByte);

  // ⚠️ "Both at once" -- which Twilight quite reasonably read as *"both of WHAT? there are three
  // other options."* It means both bits set at the same time, which is a thing no real game writes
  // and the console does something confused with. So it says what it is.
  const QVariantList kinds = {
    option(KindPlain,   tr("Somebody to talk to")),
    option(KindTrainer, tr("A trainer — they will battle you")),
    option(KindItem,    tr("An item ball — pick it up")),
    option(KindBoth,    tr("A trainer AND an item ball"), true),
  };
  add(field(talk, "spriteKind", tr("What they are"),
                   tr("The top two bits of the script byte. The game reads them to decide whether "
                      "walking into them starts a battle, hands you an item, or just talks."),
                   kind, 0, 3, "enum", kinds));

  add(field(talk, "textID", tr("Their script"),
                   tr("Which of THIS MAP's scripts runs when you talk to them. The names come from "
                      "the cartridge — they are what the map really uses each script for. A script "
                      "this map doesn't have is still reachable, under \"Something else\"."),
                   textByte & 0x3F, 0, 0x3F, "enum", mapTextList()));

  // The two that only exist for the kinds that have them.
  if (kind == KindItem || kind == KindBoth) {
    add(field(talk, "trainerClassOrItemID", tr("The item"),
                     tr("What is in the ball."),
                     s->getTrainerClassOrItemID(), 0, 255, "enum",
                     sectioned(itemList(), tr("Items"), tr("Glitch items"))));
  }

  if (kind == KindTrainer || kind == KindBoth) {
    add(field(talk, "trainerClassOrItemID", tr("Their class"),
                     tr("Which kind of trainer — a Bug Catcher, a Lass, a Gym Leader."),
                     s->getTrainerClassOrItemID(), 0, 255, "enum",
                     sectioned(trainerClassList(), tr("Trainer classes"), tr("Never used in-game"))));

    add(field(talk, "trainerSetID", tr("Which of their teams"),
                     tr("A trainer class has several rosters — Bug Catcher #3 brings a different "
                        "team from Bug Catcher #1. This picks which one."),
                     s->getTrainerSetID(), 0, 255, "team"));
  }

  // ── Right now ──────────────────────────────────────────────────────────────────────────────
  //
  // The live state of the walk. These are exactly the numbers the simulation ticks -- and the game
  // rebuilds them from the map when it loads the save, so every one of them wears the yellow "!".
  const QString live = tr("Right now");

  const QVariantList statuses = {
    option(0, tr("Not started yet")),
    option(1, tr("Ready to move")),
    option(2, tr("Waiting out a delay")),
    option(3, tr("Mid-step")),
  };
  add(field(live, "movementStatus", tr("Doing"),
                   tr("Where this sprite is in its walk cycle. A save at rest is almost always "
                      "\"ready to move\"."),
                   s->movementStatus, 0, 255, "enum", statuses, true));

  // Not a number and not a sentence explaining what a number means. A DURATION, drawn as one.
  // (Twilight: "What is 'delay until next move'? What does that mean, how is it measured? Don't
  // tell them with text — tell them with a beautiful, polished, clean UI/UX.")
  add(field(live, "movementDelay", tr("Then waits"),
                   tr("How long before it may move again. The game counts this down one per frame, "
                      "at just under 60 frames a second, and sets a fresh random one every time a "
                      "sprite finishes a step."),
                   s->movementDelay, 0, 255, "frames", {}, true));

  add(field(live, "yDisp", tr("How far it has wandered, up/down"),
                   tr("Meant to stop a sprite drifting away from where it started. It doesn't: the "
                      "game only ever checks one end of the range, so a sprite can walk off in the "
                      "other direction forever. The bug is in the cartridge and we keep it."),
                   s->yDisp, 0, 255, "byte", {}, true));

  add(field(live, "xDisp", tr("How far it has wandered, left/right"),
                   tr("As above, and just as bugged."),
                   s->xDisp, 0, 255, "byte", {}, true));

  // ── The drawing ────────────────────────────────────────────────────────────────────────────
  //
  // Was "Animation scratch", which named the bytes without saying anything about them. These are
  // what the console is using to PUT THIS SPRITE ON THE SCREEN this frame -- and it works all of
  // them out again the moment it loads the save. Every one gets the "!".
  const QString draw = tr("The drawing");

  // ⚠️ `pictureIDCopy` IS NOT A SECOND COPY OF THE PICTURE ID. It was briefly modelled as one, and
  // the panel grew a "the game's second copy disagrees" row that fired on **every sprite in every
  // save** -- the same "noise is a bug" mistake that the ROM-cast warning made. The cartridge:
  //
  //     ; the pictures IDs stored at [x#SPRITESTATEDATA2_PICTUREID] are no longer needed,
  //     ; so zero them
  //     .zeroStoredPictureIDLoop
  //         xor a
  //         ld [hl], a          ; [x#SPRITESTATEDATA2_PICTUREID]
  //                               -- engine/overworld/map_sprites.asm, LoadMapSpriteTilePatterns
  //
  // It is a SCRATCH byte the game borrows while it is loading tile patterns and then **wipes**. At
  // rest it is 0 for all sixteen slots, in every save ever written by a console. So it lives here,
  // with the rest of the scratch, wearing the "!" -- not up in Character pretending to be a fact.
  add(field(draw, "pictureIDCopy", tr("Picture, mid-load"),
                   tr("A scratch byte the game borrows while it loads this map's artwork into video "
                      "memory, and then wipes. In a real save it is always zero — including this "
                      "one, probably."),
                   s->pictureIDCopy, 0, 255, "byte", {}, true));

  add(field(draw, "imageIndex", tr("Which frame is showing"),
                   tr("Which picture of this character the console is drawing. $FF means \"off "
                      "screen — don't draw at all\"."),
                   s->imageIndex, 0, 255, "byte", {}, true));

  add(field(draw, "imageBaseOffset", tr("Where its pictures live"),
                   tr("Which of the eleven sprite-picture slots in the Game Boy's video memory this "
                      "character's artwork was loaded into. The player is always slot 1."),
                   s->imageBaseOffset, 0, 255, "byte", {}, true));

  add(field(draw, "walkAnimationCounter", tr("Frames left in this step"),
                   tr("A step takes 16 frames. This counts them down."),
                   s->walkAnimationCounter, 0, 255, "frames", {}, true));

  add(field(draw, "animFrameCounter", tr("Where in the walk cycle"),
                   tr("Four states, which is what makes the four-frame walk. (The game only has "
                      "TWO walking pictures — it mirrors one of them to fake the other leg.)"),
                   s->animFrameCounter, 0, 255, "byte", {}, true));

  add(field(draw, "intraAnimationFrameCounter", tr("Frames until the next one"),
                   tr("Counts to 4 between animation frames, so the legs don't flicker."),
                   s->intraAnimationFrameCounter, 0, 255, "frames", {}, true));

  const QVariantList steps = {
    option(0x00, tr("Still")),
    option(0x01, tr("+1 pixel")),
    option(0xFF, tr("−1 pixel")),
  };
  add(field(draw, "yStepVector", tr("Moving, up/down"),
                   tr("How far it slides each frame while it is mid-step."),
                   s->getYStepVector(), 0, 255, "enum", steps, true));

  add(field(draw, "xStepVector", tr("Moving, left/right"),
                   tr("As above."),
                   s->getXStepVector(), 0, 255, "enum", steps, true));

  add(field(draw, "screenXY", tr("On screen, in pixels"),
                   tr("Where the console is actually drawing this sprite, in screen pixels. It "
                      "works this out from the map coordinates and the player's, every single time "
                      "it loads a map — so it is a read-out, not a setting."),
                   (s->xPixels & 0xFF) | ((s->yPixels & 0xFF) << 8), 0, 0, "pixels", {}, true));

  add(field(draw, "gridXY", tr("Snapped to the grid"),
                   tr("The same position, rounded to whole tiles. It is what the collision code "
                      "reads, and the game recomputes it."),
                   (s->xAdjusted & 0xFF) | ((s->yAdjusted & 0xFF) << 8), 0, 0, "pixels", {}, true));

  add(field(draw, "collisionData", tr("What it last bumped into"),
                   tr("A bit per direction: which ways this sprite found blocked the last time it "
                      "tried to walk."),
                   s->collisionData, 0, 255, "byte", {}, true));

  return ret;
}

void MapModel::setNpcField(int slot, const QString& key, int value)
{
  if (npcs == nullptr || slot < 1 || slot >= npcs->spriteCount())
    return;

  SpriteData* s = npcs->spriteAt(slot);
  if (s == nullptr)
    return;

  // Every one of these is a byte, and every byte takes its full range. A value no real game would
  // hold is FLAGGED in the panel, never refused and never quietly corrected.
  const int b = value & 0xFF;

  if      (key == "pictureID")      { s->pictureID = b;      s->pictureIDChanged(); }
  else if (key == "pictureIDCopy")  { s->pictureIDCopy = b;  s->pictureIDCopyChanged(); }

  // ── The composite fields ─────────────────────────────────────────────────────────────────
  //
  // X+Y and the two pixel pairs are ONE control each in the panel (they are one fact), so they
  // arrive as one packed value: low byte = X, high byte = Y. Unpacked here, and only here.
  //
  // X and Y are shown WITHOUT the game's +4 bias and stored WITH it -- one conversion, in one
  // place, so the panel and the canvas can never disagree about where somebody is.
  else if (key == "mapXY") {
    s->mapX = ((value & 0xFF) + 4) & 0xFF;
    s->mapY = (((value >> 8) & 0xFF) + 4) & 0xFF;
    s->mapXChanged();
    s->mapYChanged();
  }
  else if (key == "screenXY") {
    s->xPixels = value & 0xFF;
    s->yPixels = (value >> 8) & 0xFF;
    s->xPixelsChanged();
    s->yPixelsChanged();
  }
  else if (key == "gridXY") {
    s->xAdjusted = value & 0xFF;
    s->yAdjusted = (value >> 8) & 0xFF;
    s->xAdjustedChanged();
    s->yAdjustedChanged();
  }

  // ⚠️ THE KIND IS NOT A BYTE OF ITS OWN. It is bits 6 and 7 of the text byte, so setting it
  // REWRITES THOSE TWO BITS AND NOTHING ELSE -- the script id in the bottom six is left exactly as
  // it was. (Byte fidelity: we were told to change what it IS, not what it says.)
  else if (key == "spriteKind") {
    const int keep = s->getTextID() & 0x3F;
    int bits = 0;
    if (value == 1 || value == 3) bits |= 0x40;   // TRAINER
    if (value == 2 || value == 3) bits |= 0x80;   // ITEM
    s->setTextID(keep | bits);
  }

  // ...and the script id is the OTHER six bits, with the kind bits left alone. Same rule, both ways.
  else if (key == "textID") {
    const int keep = s->getTextID() & 0xC0;
    s->setTextID(keep | (value & 0x3F));
  }

  else if (key == "movementByte")   { s->movementByte = b;   s->movementByteChanged(); }
  else if (key == "rangeDirByte")   { s->setRangeDirByte(b); }
  else if (key == "faceDir")        { s->faceDir = b;        s->faceDirChanged(); }
  else if (key == "origFacingDir")  { s->origFacingDir = b;  s->origFacingDirChanged(); }
  else if (key == "grassPriority")  { s->grassPriority = b;  s->grassPriorityChanged(); }
  else if (key == "movementStatus") { s->movementStatus = b; s->movementStatusChanged(); }
  else if (key == "movementDelay")  { s->movementDelay = b;  s->movementDelayChanged(); }
  else if (key == "yDisp")          { s->yDisp = b;          s->yDispChanged(); }
  else if (key == "xDisp")          { s->xDisp = b;          s->xDispChanged(); }
  else if (key == "trainerClassOrItemID") { s->setTrainerClassOrItemID(b); }
  else if (key == "trainerSetID")   { s->setTrainerSetID(b); }
  else if (key == "imageIndex")     { s->imageIndex = b;     s->imageIndexChanged(); }
  else if (key == "imageBaseOffset"){ s->imageBaseOffset = b; s->imageBaseOffsetChanged(); }
  else if (key == "walkAnimationCounter") { s->walkAnimationCounter = b; s->walkAnimationCounterChanged(); }
  else if (key == "animFrameCounter") { s->animFrameCounter = b; s->animFrameCounterChanged(); }
  else if (key == "intraAnimationFrameCounter") { s->intraAnimationFrameCounter = b; s->intraAnimationFrameCounterChanged(); }
  else if (key == "yStepVector")    { s->setYStepVector(b); }
  else if (key == "xStepVector")    { s->setXStepVector(b); }
  else if (key == "collisionData")  { s->collisionData = b;  s->collisionDataChanged(); }
  else
    return;   // an unknown key writes NOTHING -- we do not guess at bytes

  castEdited = true;
  changed();
}

// ── Which tiles animate (the save's `type` byte, 0x3522 = sTileAnimations) ─────

int MapModel::tileAnim() const
{
  return tileset->type;
}

void MapModel::setTileAnim(int anim)
{
  // Three values and only three -- 0, 1, 2 -- because that is all the game's own byte can
  // meaningfully be. (Unlike the music bank, an out-of-range value here isn't dangerous, it
  // simply isn't reachable through this control; a save holding one is still SHOWN.)
  if (anim < 0 || anim > 2 || tileset->type == anim)
    return;

  // Writes exactly one save byte, and nothing else.
  tileset->type = anim;
  tileset->typeChanged();

  changed();
  overlayChanged();
}

QString MapModel::tileAnimName() const
{
  const QString name = MapEngine::tileAnimName(tileAnim());

  // A save can hold something that isn't 0/1/2. Say so rather than pretend it's "Indoor".
  return name.isEmpty()
       ? QObject::tr("Unknown (%1)").arg(tileAnim())
       : name;
}

QString MapModel::tileAnimDoes() const
{
  switch (tileAnim()) {
    case 0:  return QObject::tr("Nothing animates.");
    case 1:  return QObject::tr("Water animates. Flowers don't.");
    case 2:  return QObject::tr("Water and flowers animate.");
    default: return QObject::tr("Not a value the game uses — it will read past its own table.");
  }
}

int MapModel::tileAnimDefault() const
{
  for (auto* el : TilesetDB::inst()->getStore())
    if (el->ind == tilesetInd())
      return static_cast<int>(el->typeAsEnum());

  return -1;
}

bool MapModel::tileAnimIsDefault() const
{
  const int def = tileAnimDefault();
  return def < 0 || def == tileAnim();
}

// ── The semantic overlay ──────────────────────────────────────────────────────

int MapModel::layers() const { return shownLayers; }

void MapModel::setLayers(int layers)
{
  if (shownLayers == layers)
    return;

  shownLayers = layers;
  overlayChanged();
}

void MapModel::toggleLayer(int layer)
{
  setLayers(shownLayers ^ layer);
}

bool MapModel::layerOn(int layer) const
{
  return (shownLayers & layer) != 0;
}

namespace {
/// The save's grass + counter tiles, which the overlay and the inspector both need.
MapEngine::SaveTiles saveTilesOf(AreaTileset* tileset)
{
  MapEngine::SaveTiles out;
  out.grassTile = tileset->grassTile;
  for (int i = 0; i < maxTalkingOverTiles; i++)
    out.counters.append(tileset->talkingOverTiles[i]);
  return out;
}
} // namespace

QString MapModel::overlaySource() const
{
  if (!valid() || shownLayers == 0)
    return QString();

  const MapEngine::SaveTiles save = saveTilesOf(tileset);

  QString id = "image://map/overlay/" + QString::number(mapInd())
             + "/" + QString::number(tilesetInd())
             + "/" + QString::number(static_cast<uint>(shownLayers))
             + "/" + QString::number(save.grassTile);

  // Always THREE counter slots, even when the save has fewer -- the border block follows them in the
  // id, and a variable-length list would shift its position and quietly turn a counter into a border.
  for (int i = 0; i < 3; i++)
    id += "/" + QString::number(i < save.counters.size() ? save.counters.at(i) : 0xFF);

  // The block the ring is filled with (the save's own). The BORDER layer paints over the ring, so it
  // has to be built from the same block the map is.
  id += "/" + QString::number(borderBlock());

  return id;
}

QVariantList MapModel::layerList() const
{
  // Everything a chip needs to draw ITSELF as its own legend -- the colour it paints in, the
  // name, and the sentence explaining what the thing actually is. No separate legend row,
  // and no chance of the two drifting apart.
  static const QVector<MapEngine::Layer> order = {
    MapEngine::LayerWalls, MapEngine::LayerGrass, MapEngine::LayerWater,
    MapEngine::LayerWarps, MapEngine::LayerDoors, MapEngine::LayerLedges,
    MapEngine::LayerCounters, MapEngine::LayerElevation, MapEngine::LayerCutTrees,
    MapEngine::LayerBorder,
  };

  QVariantList out;
  if (!valid())
    return out;

  const auto buffer = mapBuffer();
  const MapEngine::SaveTiles save = saveTilesOf(tileset);

  for (const MapEngine::Layer layer : order) {
    QVariantMap m;
    m["layer"] = static_cast<int>(layer);
    m["name"] = MapEngine::layerName(layer);
    m["description"] = MapEngine::layerDescription(layer);
    m["color"] = MapEngine::layerColor(layer);
    m["on"] = layerOn(static_cast<int>(layer));

    // A chip for something this map hasn't got should say so plainly, rather than switch on
    // an empty overlay and leave you wondering whether it's broken.
    m["applies"] = MapEngine::layerApplies(buffer, tilesetInd(), layer, save);

    out.append(m);
  }

  return out;
}

// ── The selected block ────────────────────────────────────────────────────────

bool MapModel::hasSelection() const  { return selX >= 0 && selY >= 0 && valid(); }
int MapModel::selectedBlockX() const { return selX; }
int MapModel::selectedBlockY() const { return selY; }

int MapModel::selectedBlock() const
{
  if (!hasSelection())
    return -1;

  return MapEngine::blockAt(mapBuffer(), selX, selY);
}

bool MapModel::selectedIsBorder() const
{
  if (!hasSelection())
    return false;

  return selX < MapEngine::mapBorder || selY < MapEngine::mapBorder
      || selX >= blocksWide() + MapEngine::mapBorder
      || selY >= blocksHigh() + MapEngine::mapBorder;
}

// The user thinks in MAP coordinates; the engine thinks in BUFFER coordinates (which include
// the 3-block ring). Translate, and say -1 out in the ring rather than hand back a negative
// that looks like a real coordinate.
int MapModel::selectedMapX() const
{
  return (!hasSelection() || selectedIsBorder()) ? -1 : selX - MapEngine::mapBorder;
}

int MapModel::selectedMapY() const
{
  return (!hasSelection() || selectedIsBorder()) ? -1 : selY - MapEngine::mapBorder;
}

void MapModel::selectAtPixel(int px, int py)
{
  if (!valid())
    return;

  const int bx = px / MapEngine::blockPx;
  const int by = py / MapEngine::blockPx;

  const auto buffer = mapBuffer();
  if (MapEngine::blockAt(buffer, bx, by) < 0)
    return;  // clicked outside the buffer -- keep whatever was selected

  // Clicking the selected block again deselects it. The map is the thing; a selection you
  // can't get rid of is a selection that's in the way.
  if (selX == bx && selY == by) {
    clearSelection();
    return;
  }

  selX = bx;
  selY = by;
  selectionChanged();
}

void MapModel::moveSelection(int dx, int dy)
{
  if (!hasSelection())
    return;

  const auto buffer = mapBuffer();
  const int bx = qBound(0, selX + dx, buffer.stride - 1);
  const int by = qBound(0, selY + dy, buffer.rows - 1);

  if (bx == selX && by == selY)
    return;

  selX = bx;
  selY = by;
  selectionChanged();
}

void MapModel::clearSelection()
{
  if (selX < 0 && selY < 0)
    return;

  selX = -1;
  selY = -1;
  selectionChanged();
}

void MapModel::revalidateSelection()
{
  // The map changed under the selection. A block index that was inside the old buffer may be
  // outside the new one, so drop it rather than point at nothing.
  if (selX < 0 && selY < 0)
    return;

  const auto buffer = mapBuffer();
  if (MapEngine::blockAt(buffer, selX, selY) < 0)
    clearSelection();
  else
    selectionChanged();
}

namespace {
/// The one-line human summary of a tile. This is the entire point of the inspector: nobody
/// should have to know that $52 is grass, or that "counter" means the shop desk.
QString tileLabel(TileTraitsDB::Traits t, const QString& ledgeFacing)
{
  QStringList parts;

  if (t.testFlag(TileTraitsDB::Door))      parts << QObject::tr("Door");
  else if (t.testFlag(TileTraitsDB::Warp)) parts << QObject::tr("Warp");
  if (t.testFlag(TileTraitsDB::WarpPad))   parts << QObject::tr("Warp pad");
  if (t.testFlag(TileTraitsDB::Hole))      parts << QObject::tr("Hole");
  if (t.testFlag(TileTraitsDB::Grass))     parts << QObject::tr("Grass — wild Pokémon");
  if (t.testFlag(TileTraitsDB::Water))     parts << QObject::tr("Water — needs Surf");
  if (t.testFlag(TileTraitsDB::Counter))   parts << QObject::tr("Counter — talk across it");
  if (t.testFlag(TileTraitsDB::Bookshelf)) parts << QObject::tr("Reads back");
  if (t.testFlag(TileTraitsDB::Elevation)) parts << QObject::tr("Elevation edge");

  if (t.testFlag(TileTraitsDB::Ledge)) {
    parts << QObject::tr("Ledge — jump %1").arg(ledgeFacing.isEmpty()
                                                ? QObject::tr("off") : ledgeFacing);
  }

  // Wall vs floor is the fallback, not the headline: if a tile is a door, "door" is the
  // useful thing to say, and "you can't walk on it" is implied by it being a door.
  if (parts.isEmpty())
    parts << (t.testFlag(TileTraitsDB::Wall) ? QObject::tr("Wall")
                                             : QObject::tr("Floor — you can walk here"));

  return parts.join(QObject::tr(", "));
}
} // namespace

QVariantMap MapModel::describeAt(int px, int py) const
{
  // What is under the cursor, in the two coordinate systems and in words -- everything the status
  // bar says. One call per mouse-move, so it does no rendering: it reads the block buffer (cheap,
  // and MapEngine caches it) and one tile's traits.
  QVariantMap m;
  m["valid"] = false;

  if (!valid() || px < 0 || py < 0 || px >= imageWidth() || py >= imageHeight())
    return m;

  const int bx = px / MapEngine::blockPx;
  const int by = py / MapEngine::blockPx;

  const auto buffer = mapBuffer();
  const int block = MapEngine::blockAt(buffer, bx, by);
  if (block < 0)
    return m;

  // Which of the block's 16 tiles the cursor is actually over (4x4 of 8px tiles).
  const int tx = (px % MapEngine::blockPx) / MapEngine::tilePx;
  const int ty = (py % MapEngine::blockPx) / MapEngine::tilePx;

  const QByteArray tiles = MapEngine::blockTileIds(tilesetInd(), block);
  const int idx = ty * MapEngine::blockTiles + tx;
  const int tile = (idx >= 0 && idx < tiles.size())
                   ? static_cast<quint8>(tiles.at(idx)) : -1;

  const bool border = bx < MapEngine::mapBorder || by < MapEngine::mapBorder
                   || bx >= blocksWide() + MapEngine::mapBorder
                   || by >= blocksHigh() + MapEngine::mapBorder;

  m["valid"] = true;
  m["blockX"] = bx;                 // buffer coords (the ring included)
  m["blockY"] = by;
  m["block"] = block;
  m["border"] = border;

  // Map coords -- what a player would call the place. -1 out in the ring, rather than a negative
  // number that looks like a real coordinate.
  m["mapBlockX"] = border ? -1 : bx - MapEngine::mapBorder;
  m["mapBlockY"] = border ? -1 : by - MapEngine::mapBorder;
  m["mapTileX"] = border ? -1 : (bx - MapEngine::mapBorder) * 2 + (tx / 2);
  m["mapTileY"] = border ? -1 : (by - MapEngine::mapBorder) * 2 + (ty / 2);

  m["tile"] = tile;
  m["label"] = (tile >= 0) ? tileInfo(tile).value("label").toString() : QString();

  return m;
}

QVariantMap MapModel::tileInfo(int tile) const
{
  const MapEngine::SaveTiles save = saveTilesOf(tileset);

  QVector<var8> counters;
  for (const int c : save.counters)
    counters.append(static_cast<var8>(c));

  auto* traits = TileTraitsDB::inst();
  const TileTraitsDB::Traits t = traits->traitsOf(
      tilesetInd(), static_cast<var8>(tile),
      static_cast<var8>(save.grassTile), counters);

  const QString facing = traits->ledgeFacing(static_cast<var8>(tile));

  QVariantMap m;
  m["tile"] = tile;
  m["wall"] = t.testFlag(TileTraitsDB::Wall);
  m["passable"] = t.testFlag(TileTraitsDB::Passable);
  m["grass"] = t.testFlag(TileTraitsDB::Grass);
  m["water"] = t.testFlag(TileTraitsDB::Water);
  m["warp"] = t.testFlag(TileTraitsDB::Warp);
  m["door"] = t.testFlag(TileTraitsDB::Door);
  m["ledge"] = t.testFlag(TileTraitsDB::Ledge);
  m["ledgeFacing"] = facing;
  m["counter"] = t.testFlag(TileTraitsDB::Counter);
  m["bookshelf"] = t.testFlag(TileTraitsDB::Bookshelf);
  m["warpPad"] = t.testFlag(TileTraitsDB::WarpPad);
  m["hole"] = t.testFlag(TileTraitsDB::Hole);
  m["elevation"] = t.testFlag(TileTraitsDB::Elevation);
  m["label"] = tileLabel(t, facing);

  return m;
}

QVariantList MapModel::blockTileIds(int block) const
{
  QVariantList out;

  const QByteArray tiles = MapEngine::blockTileIds(tilesetInd(), block);
  for (int i = 0; i < tiles.size(); i++)
    out.append(static_cast<int>(static_cast<quint8>(tiles.at(i))));

  return out;
}

QString MapModel::tileAnimStrFor(int anim) const
{
  switch (anim) {
    case 1:  return QStringLiteral("cave");
    case 2:  return QStringLiteral("outdoor");
    default: return QStringLiteral("indoor");
  }
}

int MapModel::blockCount() const
{
  return BlocksDB::inst()->tilesetBlockCount(tilesetInd());
}

namespace {
/// The DB entry for a tileset id. (Stored in id order, but never assume it -- verify, then scan.)
TilesetDBEntry* canonAt(int tilesetInd)
{
  for (auto* el : TilesetDB::inst()->getStore())
    if (el->ind == tilesetInd)
      return el;

  return nullptr;
}
} // namespace

QVariantList MapModel::tilesetList() const
{
  QVariantList out;

  for (auto* el : TilesetDB::inst()->getStore()) {
    QVariantMap m;
    m["ind"] = el->ind;
    m["name"] = el->name;
    m["tileAnim"] = static_cast<int>(el->typeAsEnum());
    m["typeName"] = el->type;   // "Indoor" / "Cave" / "Outdoor"
    out.append(m);
  }

  return out;
}

QVariantMap MapModel::canonicalTileset() const
{
  QVariantMap m;

  auto* el = canonAt(tilesetInd());
  if (el == nullptr)
    return m;

  m["ind"] = el->ind;
  m["name"] = el->name;
  m["bank"] = el->bank;
  m["blockPtr"] = el->blockPtr;
  m["gfxPtr"] = el->gfxPtr;
  m["collPtr"] = el->collPtr;
  m["tileAnim"] = static_cast<int>(el->typeAsEnum());
  m["grassTile"] = el->grass;

  return m;
}

void MapModel::restoreTilesetPointers()
{
  auto* el = canonAt(tilesetInd());
  if (el == nullptr)
    return;

  // Exactly the four bytes that were wrong, and no others. In particular this does NOT touch
  // the grass tile, the counters or the animation byte -- the user may have meant those.
  tileset->bank = el->bank;
  tileset->blockPtr = el->blockPtr;
  tileset->gfxPtr = el->gfxPtr;
  tileset->collPtr = el->collPtr;

  tileset->bankChanged();
  tileset->blockPtrChanged();
  tileset->gfxPtrChanged();
  tileset->collPtrChanged();

  changed();
}

QVariantList MapModel::selectedTiles() const
{
  QVariantList out;
  if (!hasSelection())
    return out;

  const QByteArray tiles = MapEngine::blockTileIds(tilesetInd(), selectedBlock());
  if (tiles.isEmpty())
    return out;

  for (int i = 0; i < tiles.size(); i++) {
    const int tile = static_cast<quint8>(tiles.at(i));

    QVariantMap m = tileInfo(tile);
    m["index"] = i;
    m["x"] = i % MapEngine::blockTiles;   // 4x4, row-major -- the block's own layout
    m["y"] = i / MapEngine::blockTiles;

    out.append(m);
  }

  return out;
}
