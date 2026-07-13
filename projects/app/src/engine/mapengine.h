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
#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

struct MapDBEntry;
struct MapDBEntryConnect;

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

  /**
   * @brief One connection strip, resolved: where it reads, where it lands, how much.
   *
   * The `connection` macro's output, recomputed. @ref srcAddr is a **ROM pointer** into the
   * neighbour's block data, read in @ref toBank -- not an index into an array. @ref length
   * means a **width** for north/south and a **row count** for east/west; the same byte, two
   * meanings, which is one of the several ways this gets implemented wrong.
   *
   * See `reference/map-connections.md`.
   */
  struct Connection {
    int dir = -1;        ///< MapDBEntryConnect::ConnectDir.
    int toInd = -1;      ///< The neighbour.
    int toBank = -1;     ///< Its ROM bank -- what SwitchToMapRomBank selects.
    int toWidth = 0;     ///< Its width: the source row stride.
    int srcAddr = 0;     ///< `ConnectionStripSrc` -- a pointer into its blocks.
    int destIndex = 0;   ///< `ConnectionStripDest`, as an index into @ref Buffer::blocks.
    int length = 0;      ///< `ConnectionStripLength`. N/S: width. E/W: rows.
    bool valid = false;
  };

  /// The macro's arithmetic, for @p from's connection in @p dir to @p to at @p offset.
  static Connection connectionOf(const MapDBEntry* from, int dir,
                                 const MapDBEntry* to, int offset);

  /**
   * @brief Where each connected map's strip LANDS in the ring -- in buffer BLOCK coordinates.
   *
   * The border ring is not a wall of trees: the game bleeds the neighbouring maps' edges into it, so
   * Pallet Town's ring really is the bottom of Route 1 and the top of Route 21. That is a lot of
   * arithmetic (a clamp that turns one signed offset into two numbers, two different loop shapes),
   * and it is worth being able to SEE. This is what the Connections layer draws.
   *
   * `{ dir, toInd, name, bx, by, cols, rows }` -- block coords into Buffer::blocks, one per
   * connection this map has.
   */
  struct Strip {
    int dir = -1;
    int toInd = -1;
    QString name;
    int bx = 0, by = 0;     ///< Top-left, in buffer BLOCKS.
    int cols = 0, rows = 0; ///< Its size, in blocks. N/S: 3 rows. E/W: 3 cols.
  };

  static QVector<Strip> connectionStrips(int mapInd);

  /// Recover the header's raw signed offset from what maps.json kept (see the .cpp).
  static int connectionOffset(const MapDBEntryConnect* connect);

  /// Build @p mapInd's overworld buffer -- the map placed inside its 3-block border ring.
  /// @param borderBlock the block the 3-block ring is filled with. This is `wMapBackgroundTile`
  ///        (`AreaMap::outOfBoundsBlock`) -- **the SAVE's byte**, which is what a console reads, and
  ///        which is allowed to disagree with the map's shipped border. Pass **-1** to fall back to
  ///        the map's own (what the game would put there on a fresh load). (2026-07-13: it used to
  ///        always use the map's, so editing the save's byte changed nothing on screen.)
  static Buffer buildOverworldMap(int mapInd, int borderBlock = -1);

  // ── Palettes, and the "contrast" byte (home/fade.asm) ────────────────────────
  //
  // What the save calls CONTRAST (`wMapPalOffset`, 0x2609) is not a brightness dial. The
  // game does something wonderfully unsafe with it:
  //
  //     LoadGBPal:  hl = FadePal4 - wMapPalOffset ; rBGP = [hl+], rOBP0 = [hl+], rOBP1 = [hl+]
  //
  // It SUBTRACTS the byte from a pointer INTO a table of eight contiguous 3-byte palettes.
  // Land on a multiple of 3 and you get a real entry -- 0, 3, 6, 9 give FadePal4, 3, 2, 1:
  // **the four contrast levels** (0 normal, 6 the dark "needs FLASH" cave palette, 9 black).
  // Land anywhere else and the read **straddles two entries**, producing a palette that
  // exists nowhere in the game -- 1, 2, 4, 5, 7, 8: **the six glitch palettes**.
  //
  // All ten verified against the real console's palette registers (scripts/emu/verify_palettes.py).

  static constexpr int contrastMax = 9;  ///< The last value still inside the fade table.

  /// `rBGP` for @p contrast -- the palette the MAP is drawn with. -1 if past the table.
  static int backgroundPalette(int contrast);

  /// `rOBP0` for @p contrast -- the palette the PLAYER is drawn with. -1 if past the table.
  ///
  /// This is the byte the "harmless" glitch palettes really damage. Contrast 1 and 2 leave
  /// `rBGP` alone -- the map looks normal -- and shift *this* one along the fade table.
  static int spritePalette(int contrast);

  /// Is @p contrast one of the six misaligned reads? (Not a level -- a glitch palette.)
  static bool isGlitchPalette(int contrast);

  /// What this contrast value is, in words ("Normal", "Dark (needs Flash)", "Glitch palette"...).
  static QString contrastName(int contrast);

  /**
   * @brief Render @p buffer with tileset @p tilesetInd at animation @p frame, through @p contrast.
   *
   * @param tileAnim which tiles animate -- 0 Indoor (none), 1 Cave (water), 2 Outdoor (water
   *        + flowers). This is the SAVE's byte (`sTileAnimations`, `AreaTileset::type`), which
   *        is allowed to disagree with the tileset's own default -- and when it does, the save
   *        is what the console would actually run, so the save wins. Pass **-1** to fall back
   *        to the tileset's own value. See notes/reference/tiles.md.
   */
  ///
  /// @param blocksetInd which tileset's BLOCKS to build the map out of. Normally the same tileset
  ///        the graphics come from -- but the save keeps `blockPtr` and `gfxPtr` as **two separate
  ///        pointers**, so a save can legitimately (or mischievously) draw one tileset's blocks with
  ///        another tileset's tiles, and a real console will happily do it. Pass **-1** for "the same
  ///        as @p tilesetInd". (2026-07-13)
  static QImage render(const Buffer& buffer, int tilesetInd, int frame = 0, int contrast = 0,
                       int tileAnim = -1, int blocksetInd = -1);

  /// "Indoor" / "Cave" / "Outdoor" for a `sTileAnimations` value; empty if it isn't 0-2.
  static QString tileAnimName(int tileAnim);

  // ── The semantic overlay: what each tile of the map DOES ──────────────────────
  //
  // Walls, grass, water, warps, doors, ledges, counters... none of which is a thing you can
  // see by looking at the art. TileTraitsDB knows what every tile means; this paints it.
  //
  // Rendered in C++ as ONE image rather than as QML rectangles, and that is not premature
  // optimisation: Route 17 is 78 blocks tall, so a per-tile delegate would be tens of
  // thousands of items and the screen would crawl. One image scales with zoom for free and
  // fades in and out as one thing.

  /// The overlay layers, as a bit set -- several can be shown at once.
  enum Layer : quint32 {
    LayerNone      = 0,
    LayerWalls     = 1 << 0,  ///< Everything NOT in the tileset's collision list.
    LayerGrass     = 1 << 1,  ///< The save's grass tile -- where wild Pokémon are.
    LayerWater     = 1 << 2,
    LayerWarps     = 1 << 3,
    LayerDoors     = 1 << 4,
    LayerLedges    = 1 << 5,  ///< With an arrow the way you jump.
    LayerCounters  = 1 << 6,  ///< The save's talk-over tiles -- shop desks.
    LayerBorder    = 1 << 7,  ///< The 3-block ring, and the out-of-bounds block filling it.
    LayerCutTrees  = 1 << 8,  ///< Block-level, not tile-level.
    LayerElevation = 1 << 9,  ///< Tile-pair collisions -- edges you can't cross.
  };

  /// What the save holds for the tiles only IT knows about. @see render / overlay.
  struct SaveTiles {
    int grassTile = 0xFF;        ///< `wGrassTile` -- 0xFF for none.
    QVector<int> counters;       ///< `wTilesetTalkingOverTiles` -- 0xFF slots are unused.
    int outOfBoundsBlock = -1;   ///< `wMapBackgroundTile` -- the block the ring is filled with.
  };

  /**
   * @brief The overlay for @p layers, the same size as @ref render's image.
   *
   * Transparent everywhere it has nothing to say, so QML can simply stack it over the map
   * and animate its opacity. Each layer has its own hue AND its own 8x8 pattern, so several
   * of them stacked stay legible -- and so it still reads on four shades of grey, and
   * through the glitch palettes, where a colour wash alone would not.
   */
  ///
  /// (No default for @p save: a `= {}` here would need SaveTiles' member initialisers while
  /// MapEngine is still an incomplete type. It is always known at every call site anyway.)
  static QImage overlay(const Buffer& buffer, int tilesetInd, quint32 layers,
                        const SaveTiles& save);

  /// The colour a layer is drawn in, so the chip that toggles it can BE its own legend.
  static QColor layerColor(Layer layer);

  /// The layer's name, and the one-line explanation of what it actually is.
  static QString layerName(Layer layer);
  static QString layerDescription(Layer layer);

  /// Does @p layer have anything at all to show on this map? (So a chip can say "none here"
  /// instead of lying with an empty overlay.)
  static bool layerApplies(const Buffer& buffer, int tilesetInd, Layer layer,
                           const SaveTiles& save);

  // ── The player's sprite (see reference/sprites.md) ────────────────────────────

  /// `SPRITE_FACING_*` (constants/sprite_data_constants.asm).
  enum Facing { FacingDown = 0x0, FacingUp = 0x4, FacingLeft = 0x8, FacingRight = 0xC };

  /// Sprites sit **4 pixels above** their tile row -- "which makes sprites appear to be in
  /// the centre of a tile" (ram/wram.asm). Measured off the console's OAM, not assumed.
  static constexpr int spriteLift = 4;

  /// The save's `playerCurDir` bit flags (1 right, 2 left, 4 down, 8 up) -> a @ref Facing.
  static int facingFromPlayerDir(int playerCurDir);

  /// The player's 16x16 sprite, facing @p facing, through the sprite palette for @p contrast.
  /// Colour 0 comes back **transparent**, as it is on the hardware.
  static QImage playerSprite(int facing, int contrast = 0);

  /// Where a sprite goes, in buffer pixels -- the 4-pixel lift included. (The player is
  /// slot 0 and uses the same geometry as everyone else.)
  static QRect playerRect(int x, int y);

  /**
   * @brief ANY overworld sprite's 16x16 picture -- @p pictureID 1..72, facing @p facing.
   *
   * The artwork is one atlas (`:/assets/sprites/overworld.png`, imported from `pret/pokered`
   * by `scripts/import_sprites.py`): 72 cells of 16x96, cell `pictureID - 1`. How many frames
   * a cell really holds is in the generated `spriteart.h`:
   *
   *   - **6** a walking person -- stand down/up/LEFT, then three walking frames.
   *   - **3** a still person -- stand down/up/left. No walk cycle was ever drawn for them.
   *   - **1** a ball / boulder / fossil -- every facing resolves to the same quad.
   *
   * There is **no right-facing art for anybody**: right is left, X-flipped, on the console and
   * here. Colour 0 comes back transparent. A @p pictureID of 0 (an unused slot) or one we have
   * no art for returns a null image -- the caller draws nothing rather than inventing something.
   *
   * @see notes/reference/sprites.md
   */
  static QImage npcSprite(int pictureID, int facing, int contrast = 0);

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

  /// Every tile id of one block of @p tilesetInd's blockset (16 of them, row-major).
  /// Empty if that block doesn't exist. The Block inspector's raw material.
  /// (Named `blockTileIds`, not `blockTiles` -- that name is already the constant 4.)
  static QByteArray blockTileIds(int tilesetInd, int block);

  /// The block id at buffer block coords (@p bx, @p by), or -1 if outside the buffer.
  static int blockAt(const Buffer& buffer, int bx, int by);

private:
  /// The tileset image id TilesetEngine wants: `<tileset>/<type>/<font>/<frame>`.
  /// @param tileAnim the save's animation byte; -1 = use the tileset's own.
  static QString tilesetId(int tilesetInd, int frame, int tileAnim = -1);

  /// Bleed every connected map's edge strip into @p out's border ring (LoadTileBlockMap).
  static void applyConnections(Buffer& out, const MapDBEntry* map);
};
