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
 * @file mapengine.cpp
 * @brief Implementation of MapEngine -- the overworld buffer + the renderer.
 *        See mapengine.h for the geometry this reproduces.
 */

#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QVector>

#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/tiletraitsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentryconnect.h>

#include "./mapengine.h"
#include "./spriteart.h"
#include "./tilesetengine.h"

namespace {

/// The maps DB is keyed by the map id as a string (same as AreaMap::toCurMap()).
MapDBEntry* mapAt(int mapInd)
{
  return MapsDB::inst()->getIndAt(QString::number(mapInd));
}

/// How many `incomplete` hops to follow before deciding the data is looping.
constexpr int maxIncompleteHops = 4;

} // namespace

MapDBEntry* MapEngine::sourceMap(int mapInd)
{
  auto* entry = mapAt(mapInd);

  // The glitch and half-baked maps are not empty -- they are *copies*. maps.json says
  // so itself: `incomplete` names the map they are an unfinished duplicate of, and it
  // agrees exactly with what the ROM does (its header-pointer table sends "Unused Map
  // 0B" at Saffron City's header, the Lance's Room ids at Lance's Room, and so on). So
  // when an entry doesn't carry its own size or tileset, we don't invent one -- we walk
  // to the map it is a copy of and use that map's real data, which is what a Game Boy
  // loading that id would draw.
  //
  // The reachable "*_Copy" maps (Trashed House Copy, Cinnabar Mart Copy, ...) have their
  // own dimensions but no tileset string; they hop for the tileset alone. The unused ids
  // have neither and hop for everything.
  for (int hop = 0; entry != nullptr && hop < maxIncompleteHops; hop++) {
    // "Drawable" means it has everything the renderer needs: a size, blocks, and a
    // tileset it can actually name. A copy that has its own size but no tileset (the
    // reachable "*_Copy" maps) is still not drawable, so it keeps hopping -- and it
    // lands on the very map the ROM would have loaded for it anyway, since an aliased
    // header means the two are literally the same map.
    const bool sized = entry->getWidth() > 0 && entry->getHeight() > 0;
    const bool hasBlocks = BlocksDB::inst()->hasMap(entry->getInd());
    const bool hasTileset = TilesetDB::inst()->getIndAt(entry->getTileset()) != nullptr;

    if (sized && hasBlocks && hasTileset)
      return entry;

    if (entry->getIncomplete().isEmpty())
      return nullptr; // nothing to fall back to -- there is genuinely no map here

    entry = MapsDB::inst()->getIndAt(entry->getIncomplete());
  }

  return nullptr;
}

namespace {

/// Tilesets are stored in id order, but never assume it -- verify, then scan.
TilesetDBEntry* tilesetAt(int tilesetInd)
{
  auto* entry = TilesetDB::inst()->getStoreAt(tilesetInd);
  if (entry != nullptr && entry->ind == tilesetInd)
    return entry;

  for (auto* el : TilesetDB::inst()->getStore())
    if (el->ind == tilesetInd)
      return el;

  return nullptr;
}

} // namespace

MapEngine::Buffer MapEngine::buildOverworldMap(int mapInd, int borderBlock)
{
  Buffer out;
  out.mapInd = mapInd;

  // The map whose data this id actually draws -- itself, or, for a glitch/half-baked
  // copy, the map it is a copy of (see sourceMap()).
  auto* map = sourceMap(mapInd);

  // Nothing in the data and nothing to fall back to (e.g. "Last Map"). Drawing
  // something anyway would mean inventing a map, so we draw nothing and say so.
  if (map == nullptr)
    return out;

  out.sourceInd = map->getInd();
  out.sourceName = map->bestName();
  out.isCopy = (out.sourceInd != mapInd);

  const QByteArray blocks = BlocksDB::inst()->mapBlocks(out.sourceInd);

  out.width  = map->getWidth();
  out.height = map->getHeight();

  // qsizetype throughout: Route 17 is 512x2496 px, and a map's block count is compared against a
  // QByteArray size. Multiplying two ints and letting the result widen is how you get a silent
  // overflow on a big map -- so widen first, multiply second.
  if (out.width <= 0 || out.height <= 0
      || blocks.size() != static_cast<qsizetype>(out.width) * out.height)
    return out;

  // The border block is `wMapBackgroundTile` -- and the SAVE holds it (`AreaMap::outOfBoundsBlock`).
  // That byte is what the console reads, and it is allowed to disagree with what the map ships with;
  // change it and the edge of the world genuinely changes. So the caller's value wins, and the map's
  // own is only the fallback (what a fresh load would put there).
  //
  // ⚠️ Until 2026-07-13 this always used the map's byte, so editing the save's did nothing on screen.
  out.border = (borderBlock >= 0) ? borderBlock : qMax(0, map->getBorder());

  out.stride = out.width + 2 * mapBorder;
  out.rows   = out.height + 2 * mapBorder;

  // LoadTileBlockMap: fill the whole buffer with the border block, then drop the
  // map in past the border. (Connected maps' edge strips overwrite parts of the
  // ring afterwards -- not yet reproduced; see notes.)
  out.blocks = QByteArray(static_cast<qsizetype>(out.stride) * out.rows,
                          static_cast<char>(out.border));

  for (int y = 0; y < out.height; y++) {
    const int src = y * out.width;
    const int dst = (y + mapBorder) * out.stride + mapBorder;
    for (int x = 0; x < out.width; x++)
      out.blocks[dst + x] = blocks[src + x];
  }

  applyConnections(out, map);

  out.valid = true;
  return out;
}

MapEngine::Connection MapEngine::connectionOf(const MapDBEntry* from, int dir,
                                              const MapDBEntry* to, int offset)
{
  // The `connection` macro (macros/scripts/maps.asm), reproduced exactly. It is a pure
  // function of the two maps' sizes and the offset, and it is the ONLY thing that decides
  // where a strip comes from, where it lands, and how long it is.
  //
  // Verified byte-for-byte against the compiled structs in the real cartridge, for all 78
  // connections in the game (scripts/emu/verify_connections.py).
  //
  // We deliberately do NOT use MapDBEntryConnect::stripLocation()/stripSize(): stripSize()
  // branches on `fromWidth < toWidth` where the macro clamps on min(curW + 3 - offset, toW),
  // and maps.json's `flag` is a patch for that divergence. The real game has no flag.
  // See reference/map-connections.md.
  Connection c;
  c.valid = false;

  const int curW = from->getWidth();
  const int curH = from->getHeight();
  const int toW = to->getWidth();
  const int toH = to->getHeight();

  if (curW <= 0 || curH <= 0 || toW <= 0 || toH <= 0)
    return c;

  // THE CLAMP. Two numbers out of one: `tgt` is where the strip lands in OUR ring, `src`
  // is how far into the NEIGHBOUR it starts. A negative offset would push the destination
  // out of the buffer, so the game slides the SOURCE along and pins the destination to 0.
  int src = 0;
  int tgt = offset + 3;
  if (tgt < 2) {
    src = -tgt;
    tgt = 0;
  }

  int blk = 0;   // src, relative to the neighbour's block data
  int map = 0;   // dest, relative to wOverworldMap
  int len = 0;

  switch (dir) {
    case MapDBEntryConnect::ConnectDir::NORTH:
      blk = toW * (toH - 3) + src;               // the neighbour's LAST 3 rows
      map = tgt;
      len = qMin(curW + 3 - offset, toW);
      break;

    case MapDBEntryConnect::ConnectDir::SOUTH:
      blk = src;                                 // its FIRST row
      map = (curW + 6) * (curH + 3) + tgt;
      len = qMin(curW + 3 - offset, toW);
      break;

    case MapDBEntryConnect::ConnectDir::WEST:
      blk = toW * src + toW - 3;                 // its LAST 3 columns
      map = (curW + 6) * tgt;
      len = qMin(curH + 3 - offset, toH);
      break;

    case MapDBEntryConnect::ConnectDir::EAST:
      blk = toW * src;                           // its FIRST column
      map = (curW + 6) * tgt + curW + 3;
      len = qMin(curH + 3 - offset, toH);
      break;

    default:
      return c;
  }

  c.dir = dir;
  c.toInd = to->getInd();
  c.toBank = to->getBank();
  c.toWidth = toW;
  c.srcAddr = to->getDataPtr() + blk;   // a ROM POINTER, in the neighbour's bank
  c.destIndex = map;                    // an index into our buffer
  c.length = len - src;                 // N/S: a width. E/W: a ROW COUNT.
  c.valid = true;

  return c;
}

int MapEngine::connectionOffset(const MapDBEntryConnect* connect)
{
  // maps.json keeps the POST-clamp pair (stripMove == tgt - 3, stripOffset == src), not the
  // raw offset the header was written with. The clamp is invertible: a non-zero src can only
  // have come from tgt < 2, where src = -(offset + 3).
  //
  // Verified on all 78 connections (Route 4 south = -25 -> stored (-3, 22); Route 11 east
  // = -27 -> stored (-3, 24); Viridian north = +5 -> stored (5, 0)).
  const int stripOffset = connect->getStripOffset();
  const int stripMove = connect->getStripMove();

  return (stripOffset != 0) ? (-stripOffset - 3) : stripMove;
}

QVector<MapEngine::Strip> MapEngine::connectionStrips(int mapInd)
{
  // The same walk applyConnections() does, recording WHERE each strip lands instead of copying it.
  // One source of truth for the arithmetic (connectionOf), two consumers: the renderer, and the
  // Connections layer that lets you SEE the thing. (2026-07-13, Twilight: "connector maps need
  // outlines too -- they have a lot of calculations.")
  QVector<Strip> out;

  auto* map = sourceMap(mapInd);
  if (map == nullptr)
    return out;

  const int stride = map->getWidth() + 2 * mapBorder;
  if (map->getWidth() <= 0 || map->getHeight() <= 0)
    return out;

  const int dirs[] = {
    MapDBEntryConnect::ConnectDir::NORTH,
    MapDBEntryConnect::ConnectDir::SOUTH,
    MapDBEntryConnect::ConnectDir::WEST,
    MapDBEntryConnect::ConnectDir::EAST
  };

  for (int dir : dirs) {
    const auto* connect = map->getConnectAt(dir);
    if (connect == nullptr)
      continue;

    auto* to = MapsDB::inst()->getIndAt(connect->getMap());
    if (to == nullptr)
      continue;

    const Connection c = connectionOf(map, dir, to, connectionOffset(connect));
    if (!c.valid || c.length <= 0)
      continue;

    const bool northSouth = (dir == MapDBEntryConnect::ConnectDir::NORTH
                          || dir == MapDBEntryConnect::ConnectDir::SOUTH);

    Strip s;
    s.dir = dir;
    s.toInd = c.toInd;
    s.name = to->bestName();
    s.bx = c.destIndex % stride;
    s.by = c.destIndex / stride;
    s.rows = northSouth ? mapBorder : c.length;
    s.cols = northSouth ? c.length : mapBorder;

    out.append(s);
  }

  return out;
}

void MapEngine::applyConnections(Buffer& out, const MapDBEntry* map)
{
  // LoadTileBlockMap fills the ring with the border block and THEN bleeds the connected
  // maps' edges over the top of it. Pallet Town's ring is not a wall of trees -- it is the
  // bottom of Route 1 and the top of Route 21, which is why you can see into the next route
  // before you walk there, and why the walk across is seamless.
  const int dirs[] = {
    MapDBEntryConnect::ConnectDir::NORTH,
    MapDBEntryConnect::ConnectDir::SOUTH,
    MapDBEntryConnect::ConnectDir::WEST,
    MapDBEntryConnect::ConnectDir::EAST
  };

  for (int dir : dirs) {
    const auto* connect = map->getConnectAt(dir);
    if (connect == nullptr)
      continue;

    auto* to = MapsDB::inst()->getIndAt(connect->getMap());
    if (to == nullptr)
      continue;

    const Connection c = connectionOf(map, dir, to, connectionOffset(connect));
    if (!c.valid || c.length <= 0)
      continue;

    // The two loop shapes are NOT the same, and the length means different things in them:
    //   N/S -- 3 rows (MAP_BORDER), each `length` blocks wide
    //   E/W -- `length` rows, each 3 blocks wide
    // Both step the source by the NEIGHBOUR's width and the destination by OUR stride.
    const bool northSouth = (dir == MapDBEntryConnect::ConnectDir::NORTH
                          || dir == MapDBEntryConnect::ConnectDir::SOUTH);

    const int rows = northSouth ? mapBorder : c.length;
    const int cols = northSouth ? c.length : mapBorder;

    for (int row = 0; row < rows; row++) {
      for (int col = 0; col < cols; col++) {
        const int dst = c.destIndex + row * out.stride + col;
        if (dst < 0 || dst >= out.blocks.size())
          continue; // the game would scribble on neighbouring WRAM; we simply don't

        // Read it as the console does: a pointer, in the neighbour's bank. An address we
        // don't hold comes back -1 -- unknown, left as the border block rather than faked.
        const int block = BlocksDB::inst()->blockAt(c.toBank, c.srcAddr + row * c.toWidth + col);
        if (block < 0)
          continue;

        out.blocks[dst] = static_cast<char>(block);
      }
    }
  }
}

int MapEngine::tilesetOf(int mapInd)
{
  // Walk to whichever map actually supplies this id's data -- a half-baked copy has no
  // tileset string of its own, and the map it copies is where the real one lives.
  auto* map = sourceMap(mapInd);
  if (map == nullptr)
    return -1;

  // MapsDB is not deep-linked at boot, so resolve the tileset by name ourselves --
  // TilesetDB indexes by both name and nameAlias, and maps.json stores the alias.
  auto* tileset = TilesetDB::inst()->getIndAt(map->getTileset());
  return (tileset == nullptr) ? -1 : tileset->ind;
}

QString MapEngine::tileAnimName(int tileAnim)
{
  // The save's `type` byte (0x3522 = sTileAnimations) and tileset.json's friendly names are
  // the same three values -- verified 1:1 against the cartridge's header table for all 24
  // tilesets. So this is a rename, not a mapping. (notes/reference/tiles.md)
  switch (tileAnim) {
    case 0:  return QStringLiteral("Indoor");   // TILEANIM_NONE
    case 1:  return QStringLiteral("Cave");     // TILEANIM_WATER
    case 2:  return QStringLiteral("Outdoor");  // TILEANIM_WATER_FLOWER
    default: return QString();                  // not a value the game can produce
  }
}

QString MapEngine::tilesetId(int tilesetInd, int frame, int tileAnim)
{
  auto* tileset = tilesetAt(tilesetInd);
  if (tileset == nullptr)
    return QString();

  // Which tiles animate comes from the SAVE, not the tileset -- they are allowed to
  // disagree, and when they do the save is what the console would actually run. Only fall
  // back to the tileset's own value when the caller has nothing to say (tileAnim < 0).
  const QString anim = tileAnimName(tileAnim);
  const QString type = anim.isEmpty() ? tileset->type : anim;

  // <tileset>/<type>/<font>/<frame>. Never "font": the font overlay paints over
  // tiles 0x49-0x5F, which belong to the tileset -- and no map uses a tile id past
  // the tileset's own 0x00-0x5F anyway (verified across all 248 maps, and pinned by
  // tst_map_blocks), so there is nothing in the font/text VRAM region to draw.
  return tileset->name + "/" + type + "/nofont/" + QString::number(frame);
}

// ── Palettes ──────────────────────────────────────────────────────────────────
namespace {

/// `dc a,b,c,d` -- four 2-bit shades packed MSB-first, which IS the Game Boy's palette
/// register layout (bits 7-6 = colour 3 ... bits 1-0 = colour 0). So `dc 3,2,1,0` == 0xE4,
/// the identity palette.
constexpr int dc(int c3, int c2, int c1, int c0)
{
  return (c3 << 6) | (c2 << 4) | (c1 << 2) | c0;
}

/// FadePal1..FadePal8 (home/fade.asm), laid out contiguously exactly as they are in ROM --
/// three bytes each: rBGP, rOBP0, rOBP1. The whole point is that they are CONTIGUOUS: the
/// glitch palettes are what you get by reading across the seams.
constexpr int fadeTable[] = {
  dc(3,3,3,3), dc(3,3,3,3), dc(3,3,3,3),   // FadePal1  @ 0   -- black
  dc(3,3,3,2), dc(3,3,3,2), dc(3,3,2,0),   // FadePal2  @ 3   -- very dark (needs Flash)
  dc(3,3,2,1), dc(3,2,1,0), dc(3,2,1,0),   // FadePal3  @ 6   -- dark
  dc(3,2,1,0), dc(3,1,0,0), dc(3,2,0,0),   // FadePal4  @ 9   -- NORMAL, and the base
  dc(3,2,1,0), dc(3,1,0,0), dc(3,2,0,0),   // FadePal5  @ 12
  dc(2,1,0,0), dc(2,0,0,0), dc(2,1,0,0),   // FadePal6  @ 15
  dc(1,0,0,0), dc(1,0,0,0), dc(1,0,0,0),   // FadePal7  @ 18
  dc(0,0,0,0), dc(0,0,0,0), dc(0,0,0,0),   // FadePal8  @ 21  -- white
};

constexpr int fadePal4 = 9; ///< What LoadGBPal subtracts the contrast from.

/// The four shades the tileset art is drawn in -- and, under the identity palette the art
/// was captured with, a pixel's shade IS its colour index. That is what makes the remap
/// exact rather than approximate.
constexpr int shadeGrey[4] = { 255, 170, 85, 0 };

int shadeOf(int grey)
{
  for (int i = 0; i < 4; i++)
    if (grey == shadeGrey[i])
      return i;

  // Not one of the four -- shouldn't happen with the game's art, but never guess wildly:
  // fall back to the nearest shade.
  int best = 0;
  for (int i = 1; i < 4; i++)
    if (qAbs(grey - shadeGrey[i]) < qAbs(grey - shadeGrey[best]))
      best = i;

  return best;
}

} // namespace

int MapEngine::backgroundPalette(int contrast)
{
  // hl = FadePal4 - contrast, then read three bytes. rBGP is the first of them.
  const int at = fadePal4 - contrast;

  if (contrast < 0 || at < 0)
    return -1;  // past the table -- the game would read whatever ROM sits before it, and
                // we don't ship the ROM, so we say "unknown" rather than invent a palette

  return fadeTable[at];
}

int MapEngine::spritePalette(int contrast)
{
  // rOBP0 -- the SECOND of the three bytes LoadGBPal reads. The player's OAM attributes are
  // 0, so he is drawn with OBP0.
  //
  // This is the byte the "harmless-looking" glitch palettes actually damage: contrast 1 and
  // 2 leave rBGP at $E4 (the map looks perfectly normal) but shift rOBP0 by one and two
  // bytes into the table respectively. Nothing showed it until the player was drawn.
  const int at = fadePal4 - contrast + 1;

  if (contrast < 0 || (fadePal4 - contrast) < 0)
    return -1;  // past the table -- unknown, and we say so rather than invent one

  return fadeTable[at];
}

bool MapEngine::isGlitchPalette(int contrast)
{
  // A level is an ALIGNED read. Anything else straddles two entries in the fade table and
  // is a palette that exists nowhere in the game.
  return contrast >= 0 && contrast <= contrastMax && (contrast % 3) != 0;
}

QString MapEngine::contrastName(int contrast)
{
  if (contrast < 0 || contrast > contrastMax)
    return QObject::tr("Past the fade table — the game reads whatever ROM follows");

  if (isGlitchPalette(contrast))
    return QObject::tr("Glitch palette — a read straddling two entries");

  switch (contrast) {
    case 0: return QObject::tr("Normal");
    case 3: return QObject::tr("Dark");
    case 6: return QObject::tr("Very dark — the “needs FLASH” cave palette");
    case 9: return QObject::tr("Black");
    default: break;
  }

  return QString();
}

QImage MapEngine::render(const Buffer& buffer, int tilesetInd, int frame, int contrast,
                         int tileAnim, int blocksetInd)
{
  if (!buffer.valid)
    return QImage();

  // The BLOCKS and the TILES are two different pointers in the save (`blockPtr` and `gfxPtr`), so
  // they are two different choices here. Normally they name the same tileset; a save is entitled to
  // disagree, and a console draws exactly what the pointers say. -1 = "the same one".
  const int blocksFrom = (blocksetInd < 0) ? tilesetInd : blocksetInd;

  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(blocksFrom);
  const QString id = tilesetId(tilesetInd, frame, tileAnim);

  if (blockset.isEmpty() || id.isEmpty())
    return QImage();

  const int blockCount = blockset.size() / tilesPerBlock;
  const QVector<QPixmap> tiles = TilesetEngine::buildTileset(id);

  if (tiles.isEmpty())
    return QImage();

  QImage img(buffer.stride * blockPx, buffer.rows * blockPx, QImage::Format_ARGB32);
  img.fill(Qt::white);

  QPainter p(&img);

  for (int by = 0; by < buffer.rows; by++) {
    for (int bx = 0; bx < buffer.stride; bx++) {
      const int block = static_cast<quint8>(buffer.blocks[by * buffer.stride + bx]);

      // A block id the tileset doesn't have is not something the game can draw
      // either -- leave it blank rather than invent a tile for it.
      if (block >= blockCount)
        continue;

      const int base = block * tilesPerBlock;

      for (int ty = 0; ty < blockTiles; ty++) {
        for (int tx = 0; tx < blockTiles; tx++) {
          const int tile = static_cast<quint8>(blockset[base + ty * blockTiles + tx]);
          if (tile >= tiles.size())
            continue;

          p.drawPixmap(bx * blockPx + tx * tilePx,
                       by * blockPx + ty * tilePx,
                       tiles.at(tile));
        }
      }
    }
  }

  p.end();

  // Now push the whole thing through the Game Boy's background palette, exactly as the
  // hardware does: every pixel's shade is its colour index (the art is drawn under the
  // identity palette), and rBGP says what shade each index actually comes out as.
  //
  // This is what makes contrast real rather than a filter -- and it is why the glitch
  // palettes render *correctly as glitches*: we aren't simulating "broken", we are applying
  // the byte the console would genuinely be holding.
  const int bgp = backgroundPalette(contrast);

  if (bgp >= 0 && bgp != dc(3, 2, 1, 0)) {  // 0xE4 is identity -- nothing to do
    QRgb lut[4];
    for (int i = 0; i < 4; i++) {
      const int shade = (bgp >> (2 * i)) & 3;
      const int grey = shadeGrey[shade];
      lut[i] = qRgb(grey, grey, grey);
    }

    for (int y = 0; y < img.height(); y++) {
      QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(y));
      for (int x = 0; x < img.width(); x++)
        row[x] = lut[shadeOf(qRed(row[x]))] | 0xFF000000;
    }
  }

  return img;
}

// ── The semantic overlay ──────────────────────────────────────────────────────
//
// The map is four shades of grey, and it can be pushed through a glitch palette that makes
// it any four OTHER shades of grey. So an overlay cannot rely on a colour wash alone -- pick
// any tint and there is some palette it disappears into.
//
// So every layer gets BOTH a hue and a PATTERN. The pattern is what carries the meaning when
// several layers stack, or when the palette turns hostile; the hue is what makes it quick to
// read when it doesn't. And the whole thing is drawn at low alpha, because the map is still
// the point -- this is an annotation, not a repaint.

namespace {

/// One tile's worth of overlay: a tint, and a pattern drawn into it.
enum class Pattern {
  Hatch,      ///< Diagonal lines. Walls -- it reads as "solid", and it never looks walkable.
  Dots,       ///< Grass -- scattered, like the grass sprite itself.
  Waves,      ///< Water.
  Ring,       ///< Warps + doors -- a target you step onto.
  Arrow,      ///< Ledges -- and it points the way you jump.
  Bar,        ///< Counters -- a desk, seen from above.
  Wash,       ///< The border ring: no pattern, just a flat dim. It isn't a tile property.
  Corners,    ///< Cut trees (a BLOCK, so it brackets the whole 32x32).
  EdgeLine,   ///< Elevation edges.
};

void paintTile(QPainter& p, int px, int py, const QColor& colour, Pattern pattern,
               const QString& arrowDir = QString())
{
  const QRect r(px, py, MapEngine::tilePx, MapEngine::tilePx);

  // The tint. Deliberately weak -- you must still be able to see the map through it.
  QColor fill = colour;
  fill.setAlpha(64);
  p.fillRect(r, fill);

  QColor ink = colour;
  ink.setAlpha(205);
  QPen pen(ink);
  pen.setWidth(1);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);

  switch (pattern) {
    case Pattern::Hatch:
      // Two diagonals, corner to corner -- at 8 px this reads as a solid cross-hatch when
      // tiled, which is exactly the "you cannot go here" idiom.
      p.drawLine(px, py + 7, px + 7, py);
      p.drawLine(px, py + 3, px + 3, py);
      p.drawLine(px + 4, py + 7, px + 7, py + 4);
      break;

    case Pattern::Dots:
      p.drawPoint(px + 2, py + 2);
      p.drawPoint(px + 5, py + 4);
      p.drawPoint(px + 3, py + 6);
      p.drawPoint(px + 6, py + 1);
      break;

    case Pattern::Waves:
      p.drawLine(px + 1, py + 2, px + 3, py + 2);
      p.drawLine(px + 4, py + 3, px + 6, py + 3);
      p.drawLine(px + 1, py + 5, px + 3, py + 5);
      p.drawLine(px + 4, py + 6, px + 6, py + 6);
      break;

    case Pattern::Ring:
      p.drawRect(px + 1, py + 1, 5, 5);
      break;

    case Pattern::Arrow: {
      // A ledge is the one tile whose meaning has a DIRECTION, so the overlay has to carry
      // it -- "you may hop off here, that way". A colour alone would throw that away.
      const int cx = px + 3, cy = py + 3;
      if (arrowDir == "down") {
        p.drawLine(cx, cy - 2, cx, cy + 2);
        p.drawLine(cx - 2, cy, cx, cy + 2);
        p.drawLine(cx + 2, cy, cx, cy + 2);
      } else if (arrowDir == "up") {
        p.drawLine(cx, cy + 2, cx, cy - 2);
        p.drawLine(cx - 2, cy, cx, cy - 2);
        p.drawLine(cx + 2, cy, cx, cy - 2);
      } else if (arrowDir == "left") {
        p.drawLine(cx + 2, cy, cx - 2, cy);
        p.drawLine(cx, cy - 2, cx - 2, cy);
        p.drawLine(cx, cy + 2, cx - 2, cy);
      } else {  // right
        p.drawLine(cx - 2, cy, cx + 2, cy);
        p.drawLine(cx, cy - 2, cx + 2, cy);
        p.drawLine(cx, cy + 2, cx + 2, cy);
      }
      break;
    }

    case Pattern::Bar:
      p.drawLine(px + 1, py + 3, px + 6, py + 3);
      p.drawLine(px + 1, py + 4, px + 6, py + 4);
      break;

    case Pattern::EdgeLine:
      p.drawLine(px + 1, py + 6, px + 6, py + 6);
      break;

    case Pattern::Corners:
      p.drawLine(px, py, px + 2, py);
      p.drawLine(px, py, px, py + 2);
      break;

    case Pattern::Wash:
      break;  // the tint is the whole message
  }
}

} // namespace

QColor MapEngine::layerColor(Layer layer)
{
  // Hues chosen to stay apart from each other AND from four greys: no two adjacent on the
  // wheel, and none of them so pale that a bright palette swallows them.
  switch (layer) {
    case LayerWalls:     return QColor("#37474F"); // blue grey 800 -- solid, heavy, "stop"
    case LayerGrass:     return QColor("#2E7D32"); // green 800
    case LayerWater:     return QColor("#0277BD"); // light blue 800
    case LayerWarps:     return QColor("#6A1B9A"); // purple 800
    case LayerDoors:     return QColor("#EF6C00"); // orange 800
    case LayerLedges:    return QColor("#C62828"); // red 800
    case LayerCounters:  return QColor("#00838F"); // cyan 800
    case LayerBorder:    return QColor("#546E7A"); // blue grey 600
    case LayerCutTrees:  return QColor("#9E9D24"); // lime 800
    case LayerElevation: return QColor("#4527A0"); // deep purple 800
    default:             return QColor("#000000");
  }
}

QString MapEngine::layerName(Layer layer)
{
  switch (layer) {
    case LayerWalls:     return QObject::tr("Walls");
    case LayerGrass:     return QObject::tr("Grass");
    case LayerWater:     return QObject::tr("Water");
    case LayerWarps:     return QObject::tr("Warps");
    case LayerDoors:     return QObject::tr("Doors");
    case LayerLedges:    return QObject::tr("Ledges");
    case LayerCounters:  return QObject::tr("Counters");
    case LayerBorder:    return QObject::tr("Border");
    case LayerCutTrees:  return QObject::tr("Cut trees");
    case LayerElevation: return QObject::tr("Elevation");
    default:             return QString();
  }
}

QString MapEngine::layerDescription(Layer layer)
{
  // Say what the thing IS. Half of these are words Twilight said she didn't know the meaning
  // of -- so the app is where that gets answered, not a wiki.
  switch (layer) {
    case LayerWalls:
      return QObject::tr("Everything you can't walk onto. The game stores the list of tiles you CAN "
                "walk on; a tile that isn't on it is a wall.");
    case LayerGrass:
      return QObject::tr("The grass tile. Standing here is what triggers a wild Pokémon, and it's "
                "what draws over your feet.");
    case LayerWater:
      return QObject::tr("Water. You need Surf.");
    case LayerWarps:
      return QObject::tr("Step on it and you're somewhere else.");
    case LayerDoors:
      return QObject::tr("A door — a warp you walk into, with the door animation.");
    case LayerLedges:
      return QObject::tr("A ledge. The arrow is the only way you can jump it.");
    case LayerCounters:
      return QObject::tr("The tiles you can talk ACROSS — a shop counter. Without these the clerk is "
                "out of reach behind the desk.");
    case LayerBorder:
      return QObject::tr("The 3-block ring around every map, filled with its out-of-bounds block. "
                "Connected maps bleed their edges into it.");
    case LayerCutTrees:
      return QObject::tr("A tree Cut turns into something else. This one is a whole BLOCK, not a tile.");
    case LayerElevation:
      return QObject::tr("An edge you can't cross — the game fakes a difference in height by simply "
                "forbidding the step between two particular tiles.");
    default:
      return QString();
  }
}

namespace {
/// Every layer, in the order the chips show them and the order they paint (back to front).
const QVector<MapEngine::Layer>& allLayers()
{
  static const QVector<MapEngine::Layer> layers = {
    MapEngine::LayerBorder, MapEngine::LayerWalls, MapEngine::LayerElevation,
    MapEngine::LayerWater, MapEngine::LayerGrass, MapEngine::LayerCounters,
    MapEngine::LayerCutTrees, MapEngine::LayerWarps, MapEngine::LayerDoors,
    MapEngine::LayerLedges,
  };
  return layers;
}

Pattern patternFor(MapEngine::Layer layer)
{
  switch (layer) {
    case MapEngine::LayerWalls:     return Pattern::Hatch;
    case MapEngine::LayerGrass:     return Pattern::Dots;
    case MapEngine::LayerWater:     return Pattern::Waves;
    case MapEngine::LayerWarps:     return Pattern::Ring;
    case MapEngine::LayerDoors:     return Pattern::Ring;
    case MapEngine::LayerLedges:    return Pattern::Arrow;
    case MapEngine::LayerCounters:  return Pattern::Bar;
    case MapEngine::LayerBorder:    return Pattern::Wash;
    case MapEngine::LayerCutTrees:  return Pattern::Corners;
    case MapEngine::LayerElevation: return Pattern::EdgeLine;
    default:                        return Pattern::Wash;
  }
}

/// Does tile @p tile, in this tileset, belong to @p layer?
bool tileInLayer(MapEngine::Layer layer, TileTraitsDB::Traits t)
{
  switch (layer) {
    case MapEngine::LayerWalls:     return t.testFlag(TileTraitsDB::Wall);
    case MapEngine::LayerGrass:     return t.testFlag(TileTraitsDB::Grass);
    case MapEngine::LayerWater:     return t.testFlag(TileTraitsDB::Water);
    case MapEngine::LayerWarps:     return t.testFlag(TileTraitsDB::Warp)
                                        || t.testFlag(TileTraitsDB::WarpPad)
                                        || t.testFlag(TileTraitsDB::Hole);
    case MapEngine::LayerDoors:     return t.testFlag(TileTraitsDB::Door);
    case MapEngine::LayerLedges:    return t.testFlag(TileTraitsDB::Ledge);
    case MapEngine::LayerCounters:  return t.testFlag(TileTraitsDB::Counter);
    case MapEngine::LayerElevation: return t.testFlag(TileTraitsDB::Elevation);
    default:                        return false;  // Border + CutTrees are block-level
  }
}
} // namespace

QImage MapEngine::overlay(const Buffer& buffer, int tilesetInd, quint32 layers,
                          const SaveTiles& save)   // no default -- see the header
{
  if (!buffer.valid || layers == LayerNone)
    return QImage();

  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  if (blockset.isEmpty())
    return QImage();

  auto* traits = TileTraitsDB::inst();
  const int blockCount = blockset.size() / tilesPerBlock;

  QVector<var8> counters;
  for (const int c : save.counters)
    counters.append(static_cast<var8>(c));

  QImage img(buffer.stride * blockPx, buffer.rows * blockPx, QImage::Format_ARGB32);
  img.fill(Qt::transparent);   // transparent wherever we have nothing to say

  QPainter p(&img);

  for (int by = 0; by < buffer.rows; by++) {
    for (int bx = 0; bx < buffer.stride; bx++) {
      const int block = static_cast<quint8>(buffer.blocks[by * buffer.stride + bx]);
      const int bpx = bx * blockPx;
      const int bpy = by * blockPx;

      // ── The two BLOCK-level layers. Getting these right is the whole reason this file
      // keeps insisting on the difference: a cut tree is a block, and the border ring is
      // made of blocks. Painting them per-tile would be a lie about what they are.
      if ((layers & LayerBorder) != 0) {
        const bool inRing = bx < mapBorder || by < mapBorder
                         || bx >= buffer.stride - mapBorder
                         || by >= buffer.rows - mapBorder;
        if (inRing) {
          QColor c = layerColor(LayerBorder);
          c.setAlpha(72);
          p.fillRect(bpx, bpy, blockPx, blockPx, c);
        }
      }

      if ((layers & LayerCutTrees) != 0 && traits->isCutTreeBlock(static_cast<var8>(block))) {
        QColor c = layerColor(LayerCutTrees);
        c.setAlpha(60);
        p.fillRect(bpx, bpy, blockPx, blockPx, c);
        c.setAlpha(210);
        p.setPen(QPen(c, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(bpx, bpy, blockPx - 1, blockPx - 1);
      }

      if (block >= blockCount)
        continue;

      // ── The TILE-level layers.
      const int base = block * tilesPerBlock;

      for (int ty = 0; ty < blockTiles; ty++) {
        for (int tx = 0; tx < blockTiles; tx++) {
          const var8 tile = static_cast<var8>(blockset[base + ty * blockTiles + tx]);

          const TileTraitsDB::Traits t = traits->traitsOf(
              tilesetInd, tile, static_cast<var8>(save.grassTile), counters);

          const int px = bpx + tx * tilePx;
          const int py = bpy + ty * tilePx;

          for (const Layer layer : allLayers()) {
            if ((layers & layer) == 0 || !tileInLayer(layer, t))
              continue;

            paintTile(p, px, py, layerColor(layer), patternFor(layer),
                      layer == LayerLedges ? traits->ledgeFacing(tile) : QString());
          }
        }
      }
    }
  }

  p.end();
  return img;
}

bool MapEngine::layerApplies(const Buffer& buffer, int tilesetInd, Layer layer,
                             const SaveTiles& save)
{
  // A chip for a layer with nothing to show should say so, not switch on an empty overlay
  // and leave the user wondering whether it's broken.
  if (!buffer.valid)
    return false;

  if (layer == LayerBorder)
    return true;  // every map has a ring

  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  if (blockset.isEmpty())
    return false;

  auto* traits = TileTraitsDB::inst();
  const int blockCount = blockset.size() / tilesPerBlock;

  QVector<var8> counters;
  for (const int c : save.counters)
    counters.append(static_cast<var8>(c));

  // Only the blocks this map actually PLACES count -- a tileset having a door tile is not
  // the same as this map having a door.
  for (const char raw : buffer.blocks) {
    const int block = static_cast<quint8>(raw);

    if (layer == LayerCutTrees) {
      if (traits->isCutTreeBlock(static_cast<var8>(block)))
        return true;
      continue;
    }

    if (block >= blockCount)
      continue;

    const int base = block * tilesPerBlock;
    for (int i = 0; i < tilesPerBlock; i++) {
      const var8 tile = static_cast<var8>(blockset[base + i]);
      const TileTraitsDB::Traits t = traits->traitsOf(
          tilesetInd, tile, static_cast<var8>(save.grassTile), counters);
      if (tileInLayer(layer, t))
        return true;
    }
  }

  return false;
}

QByteArray MapEngine::blockTileIds(int tilesetInd, int block)
{
  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  const int base = block * tilesPerBlock;

  if (blockset.isEmpty() || block < 0 || base + tilesPerBlock > blockset.size())
    return QByteArray();

  return blockset.mid(base, tilesPerBlock);
}

int MapEngine::blockAt(const Buffer& buffer, int bx, int by)
{
  if (!buffer.valid || bx < 0 || by < 0 || bx >= buffer.stride || by >= buffer.rows)
    return -1;

  return static_cast<quint8>(buffer.blocks[by * buffer.stride + bx]);
}

// ── The player's sprite ───────────────────────────────────────────────────────

int MapEngine::facingFromPlayerDir(int playerCurDir)
{
  // The save stores `playerCurDir` as BIT FLAGS (PLAYER_DIR_*), which are not the sprite
  // facing values (SPRITE_FACING_*). Two different encodings of the same idea.
  switch (playerCurDir) {
    case 1: return FacingRight; // PLAYER_DIR_RIGHT -> $C
    case 2: return FacingLeft;  // PLAYER_DIR_LEFT  -> $8
    case 4: return FacingDown;  // PLAYER_DIR_DOWN  -> $0
    case 8: return FacingUp;    // PLAYER_DIR_UP    -> $4
    default: break;
  }

  return FacingDown; // the game's own default, and what a fresh save holds
}

QRect MapEngine::playerRect(int x, int y)
{
  // His 2x2-tile cell, lifted 4 px -- exactly where the console puts him. Cross-checked:
  // for the player at (5,6) this lands at buffer (176, 188), and the console's OAM says he
  // is at screen (64, 60) -- which is (176,188) minus the screen's origin (112,128). Same
  // pixel, arrived at two different ways.
  return QRect(mapBorder * blockPx + x * 16,
               mapBorder * blockPx + y * 16 - spriteLift,
               16, 16);
}

QImage MapEngine::playerSprite(int facing, int contrast)
{
  // gfx/sprites/red.png: six 16x16 frames -- stand down, stand up, stand LEFT, then the
  // three walking ones. There is NO "right" frame: the game draws facing-right as
  // facing-left, X-flipped (SpriteFacingAndAnimationTable -> .FlippedOAM). We do the same.
  static const QImage sheet = QImage(":/assets/sprites/red.png").convertToFormat(QImage::Format_ARGB32);

  if (sheet.isNull())
    return QImage();

  int frame = 0;
  bool mirror = false;

  switch (facing) {
    case FacingDown:  frame = 0; break;
    case FacingUp:    frame = 1; break;
    case FacingLeft:  frame = 2; break;
    case FacingRight: frame = 2; mirror = true; break;  // left, flipped -- as the game does
    default: break;
  }

  QImage sprite = sheet.copy(0, frame * 16, 16, 16);

  if (mirror)
    sprite = sprite.mirrored(true, false);

  // The OBJECT palette. Two things the hardware does that a naive tint would not:
  //   * colour 0 is ALWAYS transparent for an object -- that is the sprite's cut-out;
  //   * the other three go through rOBP0 (the player's OAM attributes are 0, so OBP0).
  //
  // This is also where the glitch palettes finally bite: contrast 1 and 2 leave rBGP alone
  // -- the map looks fine -- and wreck rOBP0/rOBP1. Until the player was drawn, they looked
  // like nothing was wrong. See reference/palettes.md and reference/sprites.md.
  const int obp0 = spritePalette(contrast);

  for (int y = 0; y < sprite.height(); y++) {
    QRgb* row = reinterpret_cast<QRgb*>(sprite.scanLine(y));

    for (int x = 0; x < sprite.width(); x++) {
      const int index = shadeOf(qRed(row[x]));

      if (index == 0) {
        row[x] = qRgba(0, 0, 0, 0);   // transparent, always
        continue;
      }

      const int shade = (obp0 >= 0) ? ((obp0 >> (2 * index)) & 3) : index;
      const int grey = shadeGrey[shade];
      row[x] = qRgba(grey, grey, grey, 255);
    }
  }

  return sprite;
}

QVector<QPoint> MapEngine::tilesInLayer(const Buffer& buffer, int tilesetInd, Layer layer,
                                        const SaveTiles& save)
{
  QVector<QPoint> out;

  if (!buffer.valid)
    return out;

  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  if (blockset.isEmpty())
    return out;

  auto* traits = TileTraitsDB::inst();
  const int blockCount = blockset.size() / tilesPerBlock;

  QVector<var8> counters;
  for (const int c : save.counters)
    counters.append(static_cast<var8>(c));

  // The same walk layerApplies does -- block by block, tile by tile -- so "is there a door here?"
  // and "where are the doors?" can never disagree.
  for (int b = 0; b < buffer.blocks.size(); b++) {
    const int block = static_cast<quint8>(buffer.blocks.at(b));

    const int blockX = b % buffer.stride;
    const int blockY = b / buffer.stride;

    if (layer == LayerCutTrees) {
      if (traits->isCutTreeBlock(static_cast<var8>(block)))
        out.append(QPoint(blockX * blockPx, blockY * blockPx));
      continue;
    }

    if (block >= blockCount)
      continue;

    const int base = block * tilesPerBlock;
    for (int i = 0; i < tilesPerBlock; i++) {
      const var8 tile = static_cast<var8>(blockset[base + i]);
      const TileTraitsDB::Traits t = traits->traitsOf(
          tilesetInd, tile, static_cast<var8>(save.grassTile), counters);

      if (!tileInLayer(layer, t))
        continue;

      // A block is 4x4 tiles of 8 px.
      const int tx = i % blockTiles;
      const int ty = i / blockTiles;

      out.append(QPoint(blockX * blockPx + tx * 8, blockY * blockPx + ty * 8));
    }
  }

  return out;
}

QImage MapEngine::npcSprite(int pictureID, int facing, int contrast)
{
  // The atlas: 72 cells of 16x96, cell (pictureID - 1). Imported verbatim from pret/pokered
  // by scripts/import_sprites.py, in the same greyscale the player's sheet is in.
  static const QImage atlas =
      QImage(":/assets/sprites/overworld.png").convertToFormat(QImage::Format_ARGB32);

  if (atlas.isNull())
    return QImage();

  // Picture id 0 means "this slot is unused" (ram/wram.asm). Draw nothing -- do not guess.
  if (pictureID < 1 || pictureID > spriteArtCount)
    return QImage();

  const int frames = spriteArtFrames[pictureID];
  if (frames <= 0)
    return QImage();

  // Which of the three standing frames, and whether to mirror. Right is LEFT, X-FLIPPED --
  // there is no right-facing art in the game, for anybody. (SpriteFacingAndAnimationTable
  // -> .FlippedOAM; see reference/sprites.md.)
  int frame = 0;
  bool mirror = false;

  switch (facing) {
    case FacingDown:  frame = 0; break;
    case FacingUp:    frame = 1; break;
    case FacingLeft:  frame = 2; break;
    case FacingRight: frame = 2; mirror = true; break;
    default: break;
  }

  // A one-frame sprite (a ball, a boulder, a fossil) has ONE quad and the game's facing table
  // sends every facing to it. Clamp rather than read past the art we have.
  if (frame >= frames)
    frame = 0;

  QImage sprite = atlas.copy((pictureID - 1) * 16, frame * 16, 16, 16);

  if (mirror)
    sprite = sprite.mirrored(true, false);

  // Exactly as playerSprite does: colour 0 is ALWAYS transparent for an object (the sprite's
  // cut-out), and the other three go through rOBP0 -- which is where the "harmless" glitch
  // palettes stop being harmless.
  const int obp0 = spritePalette(contrast);

  for (int y = 0; y < sprite.height(); y++) {
    QRgb* row = reinterpret_cast<QRgb*>(sprite.scanLine(y));

    for (int x = 0; x < sprite.width(); x++) {
      const int index = shadeOf(qRed(row[x]));

      if (index == 0) {
        row[x] = qRgba(0, 0, 0, 0);
        continue;
      }

      const int shade = (obp0 >= 0) ? ((obp0 >> (2 * index)) & 3) : index;
      const int grey = shadeGrey[shade];
      row[x] = qRgba(grey, grey, grey, 255);
    }
  }

  return sprite;
}

QByteArray MapEngine::surroundingTiles(const Buffer& buffer, int tilesetInd, int x, int y)
{
  // LoadCurrentMapView, in full: walk the 6x5 blocks starting at the view block, and
  // stamp each one's 4x4 tiles into a 24x20 tile grid.
  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  if (!buffer.valid || blockset.isEmpty())
    return QByteArray();

  const int blockCount = blockset.size() / tilesPerBlock;
  const int width = screenBlocksW * blockTiles;   // 24
  const int height = screenBlocksH * blockTiles;  // 20

  QByteArray out(static_cast<qsizetype>(width) * height, '\0');
  const QPoint view = viewBlock(x, y);

  for (int by = 0; by < screenBlocksH; by++) {
    for (int bx = 0; bx < screenBlocksW; bx++) {
      const int col = view.x() + bx;
      const int row = view.y() + by;

      if (col < 0 || col >= buffer.stride || row < 0 || row >= buffer.rows)
        continue;

      const int block = static_cast<quint8>(buffer.blocks[row * buffer.stride + col]);
      if (block >= blockCount)
        continue;

      const int base = block * tilesPerBlock;

      for (int ty = 0; ty < blockTiles; ty++) {
        for (int tx = 0; tx < blockTiles; tx++) {
          const int dst = (by * blockTiles + ty) * width + (bx * blockTiles + tx);
          out[dst] = blockset[base + ty * blockTiles + tx];
        }
      }
    }
  }

  return out;
}

QByteArray MapEngine::screenTiles(const Buffer& buffer, int tilesetInd, int x, int y)
{
  const QByteArray surrounding = surroundingTiles(buffer, tilesetInd, x, y);
  if (surrounding.isEmpty())
    return QByteArray();

  const int srcWidth = screenBlocksW * blockTiles; // 24

  // The screen is copied out of the scratch area, pushed over by which half of his block
  // the player stands in -- 2 tiles per half-block, on each axis.
  const int offsetX = (x % 2) * 2;
  const int offsetY = (y % 2) * 2;

  QByteArray out(static_cast<qsizetype>(screenTilesW) * screenTilesH, '\0');

  for (int row = 0; row < screenTilesH; row++)
    for (int col = 0; col < screenTilesW; col++)
      out[row * screenTilesW + col] = surrounding[(row + offsetY) * srcWidth + (col + offsetX)];

  return out;
}

// ── The game's view maths ──────────────────────────────────────────────────────
//
// x and y are the player's coordinates: they count in half-blocks (one walking step
// = 2x2 tiles), so the player's block is x / 2 and the half he stands in is x & 1.

QPoint MapEngine::viewBlock(int x, int y)
{
  // The scratch area starts 2 blocks up and left of the player's block, which is
  // what puts the player at screen tile (8, 8). +mapBorder converts a map block
  // into a buffer block.
  return QPoint(mapBorder + (x / 2) - viewBlockOffset,
                mapBorder + (y / 2) - viewBlockOffset);
}

int MapEngine::viewPointer(int x, int y, int mapWidth)
{
  // wCurrentTileBlockMapViewPointer -- the WRAM address of the scratch area's
  // top-left block inside wOverworldMap. The save stores this; recomputing it and
  // comparing is how we prove the maths is the game's (tst_map_engine).
  const QPoint block = viewBlock(x, y);
  const int stride = mapWidth + 2 * mapBorder;

  return overworldMapAddr + block.y() * stride + block.x();
}

QRect MapEngine::mapRect(int width, int height)
{
  return QRect(mapBorder * blockPx, mapBorder * blockPx,
               width * blockPx, height * blockPx);
}

QRect MapEngine::scratchRect(int x, int y)
{
  const QPoint block = viewBlock(x, y);

  return QRect(block.x() * blockPx, block.y() * blockPx,
               screenBlocksW * blockPx, screenBlocksH * blockPx);
}

QRect MapEngine::screenRect(int x, int y)
{
  const QRect scratch = scratchRect(x, y);

  // The screen sits inside the scratch area, pushed over by which half of his block
  // the player is standing in: 2 tiles (16 px) per half-block, on each axis.
  const int offsetX = (x % 2) * 2 * tilePx;
  const int offsetY = (y % 2) * 2 * tilePx;

  return QRect(scratch.x() + offsetX, scratch.y() + offsetY,
               screenTilesW * tilePx, screenTilesH * tilePx);
}
