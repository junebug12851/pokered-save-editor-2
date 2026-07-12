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

#include <QPainter>
#include <QPixmap>
#include <QVector>

#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentryconnect.h>

#include "./mapengine.h"
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

MapEngine::Buffer MapEngine::buildOverworldMap(int mapInd)
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

  if (out.width <= 0 || out.height <= 0 || blocks.size() != out.width * out.height)
    return out;

  // The border block is wMapBackgroundTile -- the first byte of the map's object
  // data, which is what maps.json records as the map's border.
  out.border = qMax(0, map->getBorder());

  out.stride = out.width + 2 * mapBorder;
  out.rows   = out.height + 2 * mapBorder;

  // LoadTileBlockMap: fill the whole buffer with the border block, then drop the
  // map in past the border. (Connected maps' edge strips overwrite parts of the
  // ring afterwards -- not yet reproduced; see notes.)
  out.blocks = QByteArray(out.stride * out.rows, static_cast<char>(out.border));

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

QString MapEngine::tilesetId(int tilesetInd, int frame)
{
  auto* tileset = tilesetAt(tilesetInd);
  if (tileset == nullptr)
    return QString();

  // <tileset>/<type>/<font>/<frame>. Never "font": the font overlay paints over
  // tiles 0x49-0x5F, which belong to the tileset -- and no map uses a tile id past
  // the tileset's own 0x00-0x5F anyway (verified across all 248 maps, and pinned by
  // tst_map_blocks), so there is nothing in the font/text VRAM region to draw.
  return tileset->name + "/" + tileset->type + "/nofont/" + QString::number(frame);
}

QImage MapEngine::render(const Buffer& buffer, int tilesetInd, int frame)
{
  if (!buffer.valid)
    return QImage();

  const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(tilesetInd);
  const QString id = tilesetId(tilesetInd, frame);

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
  return img;
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

  QByteArray out(width * height, '\0');
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

  QByteArray out(screenTilesW * screenTilesH, '\0');

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
