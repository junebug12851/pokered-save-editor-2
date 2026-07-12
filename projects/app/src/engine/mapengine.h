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

#include <QByteArray>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>

struct MapDBEntry;

/**
 * @brief Rebuilds the Game Boy's overworld map exactly as the game does, and draws it.
 *
 * Not a "map viewer" -- a re-implementation of the real thing. Every number below is
 * lifted from the disassembly, and the layout it produces is the same one that lived
 * in the Game Boy's RAM.
 *
 * ## The three levels
 *
 * A **tile** is 8x8 px. A **block** is 4x4 tiles (32x32 px) and is what a map is
 * actually made of; the tileset's blockset says which 16 tiles a block is. One
 * walking step is 2x2 tiles (16 px), so a block is 2x2 steps -- which is why the
 * player's x/y coordinates count in half-blocks.
 *
 * ## The buffer (`wOverworldMap`, `LoadTileBlockMap`)
 *
 * The game does not keep the map on its own. It keeps the map surrounded by a
 * **3-block border** on every side (`MAP_BORDER`), pre-filled with the map's border
 * block (`wMapBackgroundTile` -- the first byte of the map's object data, which is
 * MapDBEntry::getBorder()), into which connected maps later bleed their edge strips.
 * So the buffer is `(width + 6) x (height + 6)` blocks, with the map itself at
 * block (3, 3). @ref buildOverworldMap reproduces it.
 *
 * ## The view (`LoadCurrentMapView`)
 *
 * From that buffer the game copies a **6x5-block scratch area** (`SCREEN_BLOCK_WIDTH`
 * x `SCREEN_BLOCK_HEIGHT` = 24x20 tiles -- `wSurroundingTiles`), and then copies the
 * **20x18-tile screen** (the actual 160x144 px Game Boy screen) out of *that*, offset
 * by `(2 * xBlockCoord, 2 * yBlockCoord)` tiles -- i.e. by 0 or 16 px on each axis,
 * depending on which half of its block the player stands in. The scratch area is
 * always block-aligned; the screen slides around inside it. That is the entire trick:
 * the map scrolls smoothly while the block grid it is built from never moves.
 *
 * The scratch area's top-left block is always the player's block **minus 2**, on both
 * axes, which puts the player at screen tile (8, 8) -- and which is exactly why the
 * border is 3 blocks wide: it makes `(width + 6) x (height + 6)` the smallest buffer
 * that can hold every view the player can produce, including at the map's corners.
 *
 * @ref viewPointer computes the pointer the game itself stores in
 * `wCurrentTileBlockMapViewPointer`; feeding it a real save's coordinates reproduces
 * the pointer that save has on disk, byte for byte (pinned by `tst_map_engine`).
 *
 * @see BlocksDB (the block data), TilesetEngine (the tiles), MapModel (the QML face),
 *      MapProvider (the image provider), `notes/reference/gen1-knowledge.md`.
 */
class MapEngine
{
public:
  // ── The game's geometry (constants/gfx_constants.asm, map_data_constants.asm) ──

  static constexpr int mapBorder = 3;          ///< `MAP_BORDER` -- border blocks on each side.
  static constexpr int blockTiles = 4;         ///< `BLOCK_WIDTH` / `BLOCK_HEIGHT` -- tiles per block side.
  static constexpr int tilePx = 8;             ///< Pixels per tile side.
  static constexpr int blockPx = blockTiles * tilePx; ///< Pixels per block side (32).
  static constexpr int screenBlocksW = 6;      ///< `SCREEN_BLOCK_WIDTH` -- scratch area width, blocks.
  static constexpr int screenBlocksH = 5;      ///< `SCREEN_BLOCK_HEIGHT` -- scratch area height, blocks.
  static constexpr int screenTilesW = 20;      ///< `SCREEN_WIDTH` -- visible screen width, tiles.
  static constexpr int screenTilesH = 18;      ///< `SCREEN_HEIGHT` -- visible screen height, tiles.
  static constexpr int tilesPerBlock = blockTiles * blockTiles; ///< Bytes per block in a blockset (16).

  /// The block the scratch area starts at, relative to the player's block (`LoadCurrentMapView`).
  static constexpr int viewBlockOffset = 2;

  /// WRAM address of `wOverworldMap` -- the base of the pointer the save stores.
  static constexpr int overworldMapAddr = 0xC6E8;

  /**
   * @brief The overworld block buffer: the map, ringed by its border block.
   *
   * `blocks` is @ref stride x @ref rows block ids, row-major, with the map's own
   * blocks at (mapBorder, mapBorder). @ref valid is false when the map has no block
   * data (the glitch ids past the end of the real maps).
   */
  struct Buffer {
    int mapInd = -1;       ///< Map id asked for.
    int sourceInd = -1;    ///< Map id the data actually came from (see @ref sourceMap).
    QString sourceName;    ///< That map's name.
    bool isCopy = false;   ///< Is this id a glitch/half-baked copy of another map?
    int width = 0;         ///< Map width, blocks.
    int height = 0;        ///< Map height, blocks.
    int stride = 0;        ///< Buffer width, blocks (`width + 2 * mapBorder`).
    int rows = 0;          ///< Buffer height, blocks (`height + 2 * mapBorder`).
    int border = 0;        ///< The border block id the ring is filled with.
    QByteArray blocks;     ///< `stride * rows` block ids.
    bool valid = false;    ///< Was there block data to build from?
  };

  /**
   * @brief The map whose data a given id actually draws.
   *
   * Usually itself. But the glitch and half-baked map ids are not empty -- they are
   * **copies**, and `maps.json` already says which map of (`incomplete`), in exact
   * agreement with the ROM: its header-pointer table sends "Unused Map 0B" at Saffron
   * City's header, the Lance's Room ids at Lance's Room, "Cinnabar Mart Copy" at
   * Cinnabar Mart. An aliased header means the two really are the same map.
   *
   * So rather than invent dimensions for an id that carries none, we follow that link
   * and draw the map it is a copy of -- which is precisely what a Game Boy loading that
   * id would put on screen. Returns nullptr only when there is genuinely nothing (no
   * size, no blocks, and nothing to fall back to -- e.g. "Last Map").
   */
  static MapDBEntry* sourceMap(int mapInd);

  /// Build @p mapInd's overworld buffer -- the map placed inside its 3-block border ring.
  static Buffer buildOverworldMap(int mapInd);

  /// Render @p buffer with tileset @p tilesetInd at animation @p frame. 32 px per block.
  static QImage render(const Buffer& buffer, int tilesetInd, int frame = 0);

  /**
   * @brief The 6x5-block scratch area as TILE ids -- the game's `wSurroundingTiles`.
   *
   * 24x20 = 480 tile ids, row-major, exactly what `LoadCurrentMapView` leaves in RAM at
   * `$C508`. Not needed to draw anything (@ref render goes straight to pixels) -- it
   * exists so the emulator harness can compare our expansion of blocks into tiles
   * against the real console's, byte for byte. See `reference/emulator-verification.md`.
   */
  static QByteArray surroundingTiles(const Buffer& buffer, int tilesetInd, int x, int y);

  /**
   * @brief The visible screen as TILE ids -- the game's `wTileMap`.
   *
   * 20x18 = 360 tile ids, row-major: `$C3A0` on the console. This is the whole view
   * pipeline in one array -- blocks, the block->tile expansion, the scratch area, and
   * the half-block screen offset -- so comparing it against real RAM verifies all of it
   * at once, with no sprites and no palettes in the way.
   */
  static QByteArray screenTiles(const Buffer& buffer, int tilesetInd, int x, int y);

  /// The tileset id a map uses per its own header (its `getToTileset` equivalent, without deep links).
  static int tilesetOf(int mapInd);

  // ── The game's view maths ────────────────────────────────────────────────────

  /// The scratch area's top-left block in buffer coordinates, for a player at @p x, @p y.
  static QPoint viewBlock(int x, int y);

  /// The pointer the game stores in `wCurrentTileBlockMapViewPointer` (a WRAM address).
  static int viewPointer(int x, int y, int mapWidth);

  /// The map itself inside the buffer, in buffer pixels.
  static QRect mapRect(int width, int height);

  /// The 6x5-block scratch area (`wSurroundingTiles`), in buffer pixels.
  static QRect scratchRect(int x, int y);

  /// The 20x18-tile visible screen, in buffer pixels. Sits inside @ref scratchRect.
  static QRect screenRect(int x, int y);

private:
  /// The tileset image id TilesetEngine wants: `<tileset>/<type>/<font>/<frame>`.
  static QString tilesetId(int tilesetInd, int frame);
};
