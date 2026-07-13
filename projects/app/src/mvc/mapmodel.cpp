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
#include <pse-db/entries/mapdbentrysprite.h>
#include <pse-db/itemsdb.h>
#include <pse-db/spriteSet.h>
#include <pse-db/sprites.h>
#include <pse-db/trainers.h>
#include <QMap>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/fragments/spritedata.h>
#include <pse-savefile/expanded/fragments/warpdata.h>
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
} // namespace

MapModel::MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset, AreaGeneral* general,
                   AreaLoadedSprites* sprites, AreaSprites* npcs, AreaWarps* warps)
  : sprites(sprites), npcs(npcs), warps(warps),
    map(map), player(player), tileset(tileset), general(general)
{
  // The map's cast changed -- somebody was placed, moved or deleted. Redraw.
  if (npcs != nullptr)
    connect(npcs, &AreaSprites::spritesChanged, this, &MapModel::changed);

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
  return "image://map/" + QString::number(mapInd())
       + "/" + QString::number(tilesetInd())
       + "/" + QString::number(frame())
       + "/" + QString::number(contrast())
       + "/" + QString::number(tileAnim())
       + "/" + QString::number(blocksetInd())    // whose BLOCKS -- the save's own second pointer
       + "/" + QString::number(borderBlock());   // what fills the ring -- the save's own byte
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

  // Which eleven pictures this map has actually LOADED into the Game Boy's sprite memory. A
  // sprite whose picture is not among them is what the console draws as garbage -- so we say
  // so rather than quietly drawing it correctly and letting the user find out in-game.
  QVector<int> loaded;
  if (sprites != nullptr) {
    for (int slot = 0; slot < 11; slot++)
      loaded.append(sprites->lSpriteAt(slot));
  }

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
    m["source"]  = "image://player/npc/" + QString::number(s->pictureID)
                 + "/" + QString::number(s->faceDir)
                 + "/" + QString::number(contrast());

    // The two things about a sprite that a person cannot see by looking at it.
    m["inSpriteSet"] = loaded.isEmpty() || loaded.contains(s->pictureID);
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

  // The eleven pictures this map has actually loaded -- so the bar can say, before you drag
  // anything, which characters this map can draw properly and which it cannot.
  QVector<int> loaded;
  if (sprites != nullptr) {
    for (int slot = 0; slot < 11; slot++)
      loaded.append(sprites->lSpriteAt(slot));

    // ⚠️ SPRITE_RED (picture 1) is ALWAYS loaded, on every map, and is NOT part of the sprite set.
    //
    //     ld hl, wSpritePlayerStateData2PictureID
    //     ld a, SPRITE_RED
    //     ld [hl], a                  ; ...and only THEN does the sprite set go in
    //                                   -- engine/overworld/map_sprites.asm
    //
    // Its tile patterns reach VRAM before the set's do, so an NPC given this picture draws as a
    // second, perfectly correct Red. Marking it "not loaded" would be a lie -- and, as Twilight put
    // it, saying the player's own picture isn't loaded is a nonsense sentence.
    loaded.append(SpritePlayerPicture);
  }

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

      m["inSpriteSet"] = loaded.isEmpty() || loaded.contains(int(e->ind));

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

QVariantMap field(const QString& group, const QString& key, const QString& label,
                  const QString& blurb, int value, int min, int max,
                  const QString& kind = QStringLiteral("byte"),
                  const QVariantList& options = {},
                  bool scratch = false)
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
QVariantMap option(int value, const QString& name, bool hack = false)
{
  QVariantMap m;
  m["value"] = value;
  m["name"]  = name;
  m["hack"]  = hack;
  return m;
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

  // Six bits. Every one of them is reachable, so every one of them is offered -- the ones the map
  // actually uses carry the name of what uses them, and the rest are honestly labelled as unused.
  for (int id = 0; id <= 0x3F; id++) {
    if (id == 0) {
      ret.append(option(0, tr("0 — nothing to say")));
      continue;
    }

    ret.append(named.contains(id)
                 ? option(id, tr("%1 — %2").arg(id).arg(named[id]))
                 : option(id, tr("%1 — this map has no script %1").arg(id), true));
  }

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

  // ── Character ──────────────────────────────────────────────────────────────────────────────
  //
  // A PICKER, with the artwork in it. You choose a character by looking at them, not by knowing
  // that 37 is a Fisherman. (Twilight: "If you need to select a picture, have a menu to select
  // from pictures.")
  const QString character = tr("Character");

  ret.append(field(character, "pictureID", tr("Picture"), QString(),
                   s->pictureID, 0, 255, "picture"));

  // ── Where ──────────────────────────────────────────────────────────────────────────────────
  //
  // X and Y are ONE fact, so they are one control (Twilight: "x and y can probably be grouped into
  // 1 box"). The +4 bias comes off here and goes back on in setNpcField -- one conversion, one place.
  ret.append(field(tr("Where"), "mapXY", tr("Standing at"),
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
    option(0xFF, tr("Stands still")),
    option(0xFE, tr("Wanders")),
    option(0x00, tr("Walks through walls"), true),
  };
  ret.append(field(movement, "movementByte", tr("What it does"),
                   tr("The game has exactly two words for this: WALK and STAY. A sprite that STAYS "
                      "is not frozen, though — it still TURNS to face you. Any other value lets it "
                      "move with no collision detection at all."),
                   s->movementByte, 0, 255, "enum", mobility));

  const QVariantList movement2 = {
    option(0x00, tr("Anywhere")),
    option(0x01, tr("Up and down only")),
    option(0x02, tr("Left and right only")),
    option(0x10, tr("A boulder — Strength pushes it")),
    option(0xD0, tr("Faces down")),
    option(0xD1, tr("Faces up")),
    option(0xD2, tr("Faces left")),
    option(0xD3, tr("Faces right")),
    option(0xFF, tr("Nothing at all")),
  };
  ret.append(field(movement, "rangeDirByte", tr("Where it may go"),
                   tr("ONE byte doing two jobs: a wander range for a sprite that walks, a fixed "
                      "facing for one that doesn't. It is NOT the facing below — that is a "
                      "different byte in a different table, and it is the one you see."),
                   s->getRangeDirByte(), 0, 255, "enum", movement2));

  const QVariantList facings = {
    option(0x0, tr("Down")),
    option(0x4, tr("Up")),
    option(0x8, tr("Left")),
    option(0xC, tr("Right")),
  };
  ret.append(field(movement, "faceDir", tr("Facing"),
                   tr("Which way it is drawn right now. There is no right-facing artwork in the "
                      "game for anybody — facing right is facing LEFT, mirrored."),
                   s->faceDir, 0, 255, "enum", facings));

  ret.append(field(movement, "origFacingDir", tr("Was facing, before it turned to you"),
                   tr("The game backs the facing up here when a sprite turns to talk, and puts it "
                      "back when the text box closes."),
                   s->origFacingDir, 0, 255, "enum", facings, true));

  const QVariantList grass = {
    option(0x00, tr("On open ground")),
    option(0x80, tr("In tall grass — it draws over their legs")),
  };
  ret.append(field(movement, "grassPriority", tr("Standing in"),
                   tr("Makes the game draw the grass OVER the sprite's lower half, so they look "
                      "like they are standing in it rather than on it."),
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

  const QVariantList kinds = {
    option(KindPlain,   tr("Somebody to talk to")),
    option(KindTrainer, tr("A trainer — they will battle you")),
    option(KindItem,    tr("An item ball — pick it up")),
    option(KindBoth,    tr("Both at once"), true),
  };
  ret.append(field(talk, "spriteKind", tr("What it is"),
                   tr("The top two bits of the script byte. The game reads them to decide whether "
                      "walking into this sprite starts a battle, hands you an item, or just talks."),
                   kind, 0, 3, "enum", kinds));

  ret.append(field(talk, "textID", tr("Its script"),
                   tr("Which of THIS MAP's scripts runs. The names come from the cartridge — they "
                      "are what the map really uses each script for."),
                   textByte & 0x3F, 0, 0x3F, "enum", mapTextList()));

  // The two that only exist for the kinds that have them.
  if (kind == KindItem || kind == KindBoth) {
    ret.append(field(talk, "trainerClassOrItemID", tr("The item"),
                     tr("What is in the ball."),
                     s->getTrainerClassOrItemID(), 0, 255, "enum", itemList()));
  }

  if (kind == KindTrainer || kind == KindBoth) {
    ret.append(field(talk, "trainerClassOrItemID", tr("Their class"),
                     tr("Which kind of trainer — a Bug Catcher, a Lass, a Gym Leader."),
                     s->getTrainerClassOrItemID(), 0, 255, "enum", trainerClassList()));

    ret.append(field(talk, "trainerSetID", tr("Which of their teams"),
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
  ret.append(field(live, "movementStatus", tr("Doing"),
                   tr("Where this sprite is in its walk cycle. A save at rest is almost always "
                      "\"ready to move\"."),
                   s->movementStatus, 0, 255, "enum", statuses, true));

  // Not a number and not a sentence explaining what a number means. A DURATION, drawn as one.
  // (Twilight: "What is 'delay until next move'? What does that mean, how is it measured? Don't
  // tell them with text — tell them with a beautiful, polished, clean UI/UX.")
  ret.append(field(live, "movementDelay", tr("Then waits"),
                   tr("How long before it may move again. The game counts this down one per frame, "
                      "at just under 60 frames a second, and sets a fresh random one every time a "
                      "sprite finishes a step."),
                   s->movementDelay, 0, 255, "frames", {}, true));

  ret.append(field(live, "yDisp", tr("How far it has wandered, up/down"),
                   tr("Meant to stop a sprite drifting away from where it started. It doesn't: the "
                      "game only ever checks one end of the range, so a sprite can walk off in the "
                      "other direction forever. The bug is in the cartridge and we keep it."),
                   s->yDisp, 0, 255, "byte", {}, true));

  ret.append(field(live, "xDisp", tr("How far it has wandered, left/right"),
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
  ret.append(field(draw, "pictureIDCopy", tr("Picture, mid-load"),
                   tr("A scratch byte the game borrows while it loads this map's artwork into video "
                      "memory, and then wipes. In a real save it is always zero — including this "
                      "one, probably."),
                   s->pictureIDCopy, 0, 255, "byte", {}, true));

  ret.append(field(draw, "imageIndex", tr("Which frame is showing"),
                   tr("Which picture of this character the console is drawing. $FF means \"off "
                      "screen — don't draw at all\"."),
                   s->imageIndex, 0, 255, "byte", {}, true));

  ret.append(field(draw, "imageBaseOffset", tr("Where its pictures live"),
                   tr("Which of the eleven sprite-picture slots in the Game Boy's video memory this "
                      "character's artwork was loaded into. The player is always slot 1."),
                   s->imageBaseOffset, 0, 255, "byte", {}, true));

  ret.append(field(draw, "walkAnimationCounter", tr("Frames left in this step"),
                   tr("A step takes 16 frames. This counts them down."),
                   s->walkAnimationCounter, 0, 255, "frames", {}, true));

  ret.append(field(draw, "animFrameCounter", tr("Where in the walk cycle"),
                   tr("Four states, which is what makes the four-frame walk. (The game only has "
                      "TWO walking pictures — it mirrors one of them to fake the other leg.)"),
                   s->animFrameCounter, 0, 255, "byte", {}, true));

  ret.append(field(draw, "intraAnimationFrameCounter", tr("Frames until the next one"),
                   tr("Counts to 4 between animation frames, so the legs don't flicker."),
                   s->intraAnimationFrameCounter, 0, 255, "frames", {}, true));

  const QVariantList steps = {
    option(0x00, tr("Still")),
    option(0x01, tr("+1 pixel")),
    option(0xFF, tr("−1 pixel")),
  };
  ret.append(field(draw, "yStepVector", tr("Moving, up/down"),
                   tr("How far it slides each frame while it is mid-step."),
                   s->getYStepVector(), 0, 255, "enum", steps, true));

  ret.append(field(draw, "xStepVector", tr("Moving, left/right"),
                   tr("As above."),
                   s->getXStepVector(), 0, 255, "enum", steps, true));

  ret.append(field(draw, "screenXY", tr("On screen, in pixels"),
                   tr("Where the console is actually drawing this sprite, in screen pixels. It "
                      "works this out from the map coordinates and the player's, every single time "
                      "it loads a map — so it is a read-out, not a setting."),
                   (s->xPixels & 0xFF) | ((s->yPixels & 0xFF) << 8), 0, 0, "pixels", {}, true));

  ret.append(field(draw, "gridXY", tr("Snapped to the grid"),
                   tr("The same position, rounded to whole tiles. It is what the collision code "
                      "reads, and the game recomputes it."),
                   (s->xAdjusted & 0xFF) | ((s->yAdjusted & 0xFF) << 8), 0, 0, "pixels", {}, true));

  ret.append(field(draw, "collisionData", tr("What it last bumped into"),
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
