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
#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>

#include "./db_autoport.h"

class QQmlEngine;

/**
 * @brief The raw block data every map is actually built out of.
 *
 * The other DBs hold what the game *says* about a map (size, warps, sprites);
 * this one holds what the map *is*: the grid of block ids, and the tile ids each
 * block is made of. Both are imported byte-for-byte from the pret/pokered
 * disassembly by `scripts/import_map_blocks.ps1` and compiled into `db.qrc`.
 *
 * Two levels, exactly as on the Game Boy:
 *
 * - **A map** is `width * height` **block ids**, row-major
 *   (`:/assets/blocks/maps/<mapInd>.blk`, one byte per block). The index is the id
 *   the save stores in `wCurMap` -- the same `ind` MapDBEntry uses.
 * - **A block** is a 4x4 grid of **tile ids**, row-major -- 16 bytes in the
 *   tileset's blockset (`:/assets/blocks/tilesets/<tilesetInd>.bst`), indexed by
 *   the id in `wCurMapTileset`. So one block is 32x32 px and one tile is 8x8 px.
 *
 * Tile ids are Game Boy BG tile ids under signed addressing: `0x00`-`0x5F` is the
 * tileset's own graphics (@ref mapTilesetSize tiles at `$9000`), `0x60`-`0x7F` the
 * text-box tiles and `0x80`-`0xFF` the font (`$8800`). A handful of *unused* blocks
 * in the reds_house/house/gate blocksets carry ids from those upper regions, but no
 * map ever places them -- every block any map actually uses stays inside
 * `0x00`-`0x5F`, which is why the renderer only ever needs the tileset graphics.
 * (Pinned by `tst_map_blocks`.)
 *
 * Standard DB-singleton: private constructor, `inst()`, and load() is called by
 * DB::loadAll() -- never from the constructor (Qt 6 static-init deadlock; see
 * `decisions/architecture.md`).
 *
 * @see MapsDB / MapDBEntry (the map's definition), TilesetDB (the tileset's),
 *      MapEngine (which turns all of this into pixels).
 */
class DB_AUTOPORT BlocksDB : public QObject
{
  Q_OBJECT

public:
  static BlocksDB* inst(); ///< The process-wide BlocksDB singleton.

  /// Blocks of border kept on every side of a map, for map connections (`MAP_BORDER`).
  static constexpr int mapBorder = 3;
  /// A block is this many tiles on a side (`BLOCK_WIDTH` / `BLOCK_HEIGHT`).
  static constexpr int blockTiles = 4;
  /// Bytes per block in a blockset -- a 4x4 grid of tile ids.
  static constexpr int tilesPerBlock = blockTiles * blockTiles;
  /// Tiles of graphics a tileset actually loads (`MAP_TILESET_SIZE`, `$60`).
  static constexpr int mapTilesetSize = 0x60;

  /// A map's block ids (`width * height` bytes, row-major). Empty if @p mapInd has no data.
  QByteArray mapBlocks(int mapInd) const;
  /// A tileset's blockset (16 bytes per block). Empty if @p tilesetInd has no data.
  QByteArray tilesetBlocks(int tilesetInd) const;

  bool hasMap(int mapInd) const;         ///< Does @p mapInd have block data?
  bool hasTileset(int tilesetInd) const; ///< Does @p tilesetInd have a blockset?

  /**
   * @brief One block byte, addressed the way the Game Boy addresses it: bank + ROM address.
   *
   * Map connections do not index a tidy per-map array -- they hold a **pointer** into the
   * neighbouring map's block data in ROM (`ConnectionStripSrc`), and the game reads through
   * it with `SwitchToMapRomBank` set to the *connected* map's bank. A strip can therefore
   * legitimately be asked to read past the end of that map's blocks, and the console simply
   * hands over whatever bytes follow.
   *
   * So we read the same way: a block is looked up by (@p bank, @p addr) against the block
   * regions we actually have -- every map's `.blk`, placed at its own `bank`/`dataPtr` from
   * maps.json. An address that lands in a region we hold gives the true byte, even if it
   * belongs to a *different* map than the one being read. An address in ROM we do not hold
   * (headers, scripts, text -- we don't ship the ROM) returns **-1**: unknown, and said so,
   * never guessed.
   *
   * No connection in the shipped game reads outside its neighbour's own blocks (verified for
   * all 78 -- `scripts/emu/verify_connections.py`). This exists so that a *hostile* offset --
   * an edited or glitched header -- overruns exactly where the console would, instead of
   * being quietly clamped into something that merely looks plausible.
   *
   * @return the block id, or -1 if that address is not block data we hold.
   */
  int blockAt(int bank, int addr) const;

  int tilesetBlockCount(int tilesetInd) const; ///< Number of blocks in @p tilesetInd's blockset.

  int mapCount() const;     ///< Number of maps with block data.
  int tilesetCount() const; ///< Number of tilesets with a blockset.

public slots:
  void load(); ///< Load every .blk / .bst from the resources. Called by DB::loadAll().
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  BlocksDB(); ///< Private -- use inst().

  /// One map's block data, placed where it really lives in ROM.
  struct Region {
    int addr = 0;      ///< Its `dataPtr` -- the address of the map's `_Blocks` label.
    QByteArray blocks; ///< The bytes there.
  };

  void indexRom(); ///< Build @ref rom from maps.json's bank/dataPtr. Called by load().

  QHash<int, QByteArray> maps;     ///< mapInd -> block ids.
  QHash<int, QByteArray> tilesets; ///< tilesetInd -> blockset.
  QHash<int, QVector<Region>> rom; ///< bank -> the block regions we hold in it.
};
