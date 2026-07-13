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
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class AreaGeneral;
class AreaMap;
class AreaPlayer;
class AreaTileset;

/**
 * @brief The loaded map, ready to draw: one image URL and four rectangles.
 *
 * Exposed as `brg.map`. Watches the save's live map (AreaMap), player position
 * (AreaPlayer) and tileset (AreaTileset), and republishes them as everything the
 * map screen needs -- the `image://map/...` @ref source and the geometry of the
 * three boxes the game itself works in, all in **buffer pixels** (one screen pixel
 * per Game Boy pixel, 32 px to a block, origin at the top-left of the border ring).
 *
 * The boxes, outermost first:
 *
 * - **the buffer** (@ref imageWidth x @ref imageHeight) -- the map plus its 3-block
 *   border ring: the whole rendered image.
 * - **the map** (@ref mapX ... @ref mapH) -- where the real map sits inside that ring.
 * - **the scratch area** (@ref scratchX ...) -- the 6x5 blocks the game redraws
 *   (`wSurroundingTiles`). Always block-aligned.
 * - **the screen** (@ref screenX ...) -- the 20x18 tiles actually on the Game Boy's
 *   screen, sliding around inside the scratch area by half-block steps.
 *
 * This has to be a C++ model rather than QML bindings: the Area's children are
 * `Q_DECLARE_OPAQUE_POINTER`, so `brg.file.data.dataExpanded.area.map.*` reads as
 * `undefined` from QML (see `reference/qt-patterns.md`).
 *
 * @see MapEngine (the maths + the rendering), MapProvider (serves @ref source).
 */
class MapModel : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool valid READ valid NOTIFY changed)          ///< Is there anything to draw for this map?
  Q_PROPERTY(QString source READ source NOTIFY changed)     ///< `image://map/...` URL for the whole map.
  Q_PROPERTY(QString mapName READ mapName NOTIFY changed)   ///< The loaded map's display name.
  Q_PROPERTY(QString tilesetName READ tilesetName NOTIFY changed) ///< The loaded tileset's name.

  /// Is the loaded id a glitch / half-baked **copy**, drawn from another map's data?
  Q_PROPERTY(bool isCopy READ isCopy NOTIFY changed)
  /// The map that data came from (@see isCopy) -- empty when the id draws its own.
  Q_PROPERTY(QString copyOfName READ copyOfName NOTIFY changed)

  /// The save's "contrast" (`wMapPalOffset`): which palette the map is drawn with.
  Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY changed)
  /// Is that one of the six **glitch palettes** -- a read straddling two fade-table entries?
  Q_PROPERTY(bool contrastIsGlitch READ contrastIsGlitch NOTIFY changed)
  /// What it is, in words ("Normal", "Very dark — the needs-FLASH cave palette", "Glitch palette"...).
  Q_PROPERTY(QString contrastName READ contrastName NOTIFY changed)
  /// The highest contrast still inside the fade table.
  Q_PROPERTY(int contrastMax READ contrastMax CONSTANT)

  Q_PROPERTY(int mapInd READ mapInd NOTIFY changed)         ///< Loaded map id (`wCurMap`).
  Q_PROPERTY(int tilesetInd READ tilesetInd NOTIFY changed) ///< Loaded tileset id (`wCurMapTileset`).
  Q_PROPERTY(int playerX READ playerX NOTIFY changed)       ///< Player x, in half-blocks.
  Q_PROPERTY(int playerY READ playerY NOTIFY changed)       ///< Player y, in half-blocks.

  Q_PROPERTY(int blocksWide READ blocksWide NOTIFY changed) ///< Map width, blocks.
  Q_PROPERTY(int blocksHigh READ blocksHigh NOTIFY changed) ///< Map height, blocks.
  Q_PROPERTY(int blockSize READ blockSize CONSTANT)         ///< Pixels per block (32).

  Q_PROPERTY(int imageWidth READ imageWidth NOTIFY changed)   ///< Rendered buffer width, px.
  Q_PROPERTY(int imageHeight READ imageHeight NOTIFY changed) ///< Rendered buffer height, px.

  Q_PROPERTY(int mapX READ mapX NOTIFY changed) ///< The map inside the border ring, px.
  Q_PROPERTY(int mapY READ mapY NOTIFY changed) ///< @see mapX
  Q_PROPERTY(int mapW READ mapW NOTIFY changed) ///< @see mapX
  Q_PROPERTY(int mapH READ mapH NOTIFY changed) ///< @see mapX

  Q_PROPERTY(int scratchX READ scratchX NOTIFY changed) ///< The 6x5-block scratch area, px.
  Q_PROPERTY(int scratchY READ scratchY NOTIFY changed) ///< @see scratchX
  Q_PROPERTY(int scratchW READ scratchW NOTIFY changed) ///< @see scratchX
  Q_PROPERTY(int scratchH READ scratchH NOTIFY changed) ///< @see scratchX

  Q_PROPERTY(int screenX READ screenX NOTIFY changed) ///< The 20x18-tile visible screen, px.
  Q_PROPERTY(int screenY READ screenY NOTIFY changed) ///< @see screenX
  Q_PROPERTY(int screenW READ screenW NOTIFY changed) ///< @see screenX
  Q_PROPERTY(int screenH READ screenH NOTIFY changed) ///< @see screenX

  /// The player's sprite: `image://player/...` (drawn through the OBJECT palette).
  Q_PROPERTY(QString playerSource READ playerSource NOTIFY changed)
  Q_PROPERTY(int playerRectX READ playerRectX NOTIFY changed) ///< His sprite, in buffer px --
  Q_PROPERTY(int playerRectY READ playerRectY NOTIFY changed) ///< 4 px above his tile row,
  Q_PROPERTY(int playerRectW READ playerRectW NOTIFY changed) ///< exactly as the console
  Q_PROPERTY(int playerRectH READ playerRectH NOTIFY changed) ///< places him.
  /// Which way he is facing (`SPRITE_FACING_*`), from the save's `playerCurDir`.
  Q_PROPERTY(int playerFacing READ playerFacing NOTIFY changed)

  // ── Which tiles animate: the save's `type` byte (0x3522 = sTileAnimations) ────
  //
  // NOT a brightness dial and NOT a bool. Three states -- Indoor (nothing animates), Cave
  // (water animates), Outdoor (water AND flowers animate) -- which is exactly the ROM's
  // TILEANIM_NONE / TILEANIM_WATER / TILEANIM_WATER_FLOWER. tileset.json's friendly names are
  // a verified 1:1 rename of it. See notes/reference/tiles.md.
  Q_PROPERTY(int tileAnim READ tileAnim WRITE setTileAnim NOTIFY changed)    ///< 0/1/2. Writes the save.
  Q_PROPERTY(QString tileAnimName READ tileAnimName NOTIFY changed)          ///< "Indoor"/"Cave"/"Outdoor".
  Q_PROPERTY(QString tileAnimDoes READ tileAnimDoes NOTIFY changed)          ///< What it does, in words.
  /// The value this tileset would have in an unedited game -- so the UI can say when the save differs.
  Q_PROPERTY(int tileAnimDefault READ tileAnimDefault NOTIFY changed)
  Q_PROPERTY(bool tileAnimIsDefault READ tileAnimIsDefault NOTIFY changed)

  // ── The semantic overlay ──────────────────────────────────────────────────────
  /// `image://map/overlay/...` for @ref layers. Empty when no layer is on.
  Q_PROPERTY(QString overlaySource READ overlaySource NOTIFY overlayChanged)
  /// The layers currently shown, as a bit set (MapEngine::Layer).
  Q_PROPERTY(int layers READ layers WRITE setLayers NOTIFY overlayChanged)

  // ── The selected block (click-to-inspect) ─────────────────────────────────────
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
  Q_PROPERTY(int selectedBlockX READ selectedBlockX NOTIFY selectionChanged) ///< In BUFFER block coords.
  Q_PROPERTY(int selectedBlockY READ selectedBlockY NOTIFY selectionChanged)
  Q_PROPERTY(int selectedBlock READ selectedBlock NOTIFY selectionChanged)   ///< The block id there.
  /// Is the selected block out in the 3-block border ring rather than on the map proper?
  Q_PROPERTY(bool selectedIsBorder READ selectedIsBorder NOTIFY selectionChanged)
  /// Where it is on the MAP (not the buffer) -- what a player would call it. -1 in the ring.
  Q_PROPERTY(int selectedMapX READ selectedMapX NOTIFY selectionChanged)
  Q_PROPERTY(int selectedMapY READ selectedMapY NOTIFY selectionChanged)

public:
  MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset, AreaGeneral* general);

  bool valid() const;
  QString source() const;
  QString mapName() const;
  QString tilesetName() const;
  bool isCopy() const;
  QString copyOfName() const;

  int contrast() const;
  void setContrast(int contrast); ///< Writes `wMapPalOffset` in the save.
  bool contrastIsGlitch() const;
  QString contrastName() const;
  int contrastMax() const;

  int mapInd() const;
  int tilesetInd() const;
  int playerX() const;
  int playerY() const;

  int blocksWide() const;
  int blocksHigh() const;
  int blockSize() const;

  int imageWidth() const;
  int imageHeight() const;

  int mapX() const;
  int mapY() const;
  int mapW() const;
  int mapH() const;

  int scratchX() const;
  int scratchY() const;
  int scratchW() const;
  int scratchH() const;

  int screenX() const;
  int screenY() const;
  int screenW() const;
  int screenH() const;

  QString playerSource() const;
  int playerRectX() const;
  int playerRectY() const;
  int playerRectW() const;
  int playerRectH() const;
  int playerFacing() const;

  int tileAnim() const;
  void setTileAnim(int anim);   ///< Writes the save's `type` byte (0x3522) -- and only that byte.
  QString tileAnimName() const;
  QString tileAnimDoes() const;
  int tileAnimDefault() const;
  bool tileAnimIsDefault() const;

  QString overlaySource() const;
  int layers() const;
  void setLayers(int layers);   ///< Purely a view setting. Touches nothing in the save.

  /// Every layer, for the chip bar: id, name, what it is, its colour, and whether this map
  /// has any of it at all. Returns a list of plain JS objects -- no QObject, so no GC trap.
  Q_INVOKABLE QVariantList layerList() const;

  /// Turn one layer on or off. @see layerList.
  Q_INVOKABLE void toggleLayer(int layer);
  Q_INVOKABLE bool layerOn(int layer) const;

  bool hasSelection() const;
  int selectedBlockX() const;
  int selectedBlockY() const;
  int selectedBlock() const;
  bool selectedIsBorder() const;
  int selectedMapX() const;
  int selectedMapY() const;

  /// Select the block containing buffer pixel (@p px, @p py) -- what a click on the map gives us.
  Q_INVOKABLE void selectAtPixel(int px, int py);
  /// Move the selection by whole blocks (arrow keys). Clamped to the buffer, ring included.
  Q_INVOKABLE void moveSelection(int dx, int dy);
  Q_INVOKABLE void clearSelection();

  /**
   * @brief The selected block's 16 tiles, each with what it DOES.
   *
   * One entry per tile, row-major, 4x4. Each is a plain JS object:
   *   { index, tile, x, y, source, wall, passable, grass, water, warp, door, ledge,
   *     ledgeFacing, counter, bookshelf, warpPad, hole, elevation, label }
   *
   * `label` is the one-line human summary ("Wall", "Grass — wild Pokémon", "Ledge — jump
   * down"...), because the point of the whole inspector is that nobody should have to know
   * that tile $52 is grass.
   */
  Q_INVOKABLE QVariantList selectedTiles() const;

  /// The same, for any one tile of the current tileset -- what the tile pickers need.
  Q_INVOKABLE QVariantMap tileInfo(int tile) const;

  /// The 16 tile ids of any block of the current tileset (row-major, 4x4). What the block
  /// pickers draw: there is no "block" image, a block only ever IS its tiles.
  Q_INVOKABLE QVariantList blockTileIds(int block) const;

  /// "indoor"/"cave"/"outdoor" for an animation value -- the string the image providers want.
  /// Exposed so QML never has to re-write that mapping (and get it wrong).
  Q_INVOKABLE QString tileAnimStrFor(int anim) const;

  /// How many blocks the current tileset actually has. A block id past this is not "some
  /// other graphic" -- it is nothing, and the game can't draw it either.
  Q_PROPERTY(int blockCount READ blockCount NOTIFY changed)
  int blockCount() const;

  /**
   * @brief Every tileset, as plain data: { ind, name, type, typeName }.
   *
   * QML cannot read a TilesetDBEntry -- it is a plain struct, not a QObject -- so the DB is
   * never handed across the bridge. (Handing a parentless QObject across it would be the
   * other, worse, way to get this wrong: QML garbage-collects them. See qt-patterns.md.)
   */
  Q_INVOKABLE QVariantList tilesetList() const;

  /**
   * @brief What the CARTRIDGE has for the current tileset: { bank, blockPtr, gfxPtr, collPtr,
   *        tileAnim, grassTile }.
   *
   * The truth the save is compared against, so the panel can say "this doesn't match" -- and
   * SHOW it, never silently rewrite it.
   */
  Q_INVOKABLE QVariantMap canonicalTileset() const;

  /// Put the tileset's pointers back to what the cartridge has for it. The one deliberate way
  /// they change; never a free-typed address, because no version of that is a good idea.
  Q_INVOKABLE void restoreTilesetPointers();

signals:
  /// The loaded map, the tileset or the player moved -- everything above may have changed.
  void changed();
  /// The shown layers changed (a view setting, not save data).
  void overlayChanged();
  /// The selected block changed.
  void selectionChanged();

private:
  /// Rebuild the selection when the map changes under it.
  void revalidateSelection();

  AreaMap* map = nullptr;         ///< The save's live map.
  AreaPlayer* player = nullptr;   ///< The save's live player position.
  AreaTileset* tileset = nullptr; ///< The save's live tileset.
  AreaGeneral* general = nullptr; ///< The save's live "contrast" (wMapPalOffset).

  int shownLayers = 0;            ///< A VIEW setting. Off by default: the map is the point.
  int selX = -1;                  ///< Selected block, in buffer coords. -1 = nothing selected.
  int selY = -1;
};
