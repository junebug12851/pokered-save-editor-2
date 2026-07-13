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

// For MapEngine::Buffer, which the private mapBuffer() returns by value (so it must be a complete
// type here). MapEngine is app-internal and header-only in its dependencies -- no fan-out.
#include "../engine/mapengine.h"

class AreaGeneral;
class AreaLoadedSprites;
class AreaSprites;
class AreaWarps;
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
  /// `image://map/...` URL for the whole map. Its OWN notify: the URL carries the animation frame,
  /// which moves ~3 times a second, and re-emitting the map-wide `changed()` at that rate would make
  /// every listener (the layer tree's "does this layer apply here?" pass, for one) recompute the
  /// whole overworld buffer three times a second for nothing.
  Q_PROPERTY(QString source READ source NOTIFY sourceChanged)
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

  /// The animation STEP being drawn (MapClock advances it). The renderer is a pure function of
  /// (save, layers, frame) -- it never reads a clock, which is what keeps the screenshot and
  /// visual-regression suites deterministic. Frame 0 is the still map.
  Q_PROPERTY(int frame READ frame WRITE setFrame NOTIFY frameChanged)

  /// Loaded map id (`wCurMap`). Writing it writes **that byte and nothing else** -- the map's size
  /// and pointers are separate bytes in the save, and rewriting them behind the user's back is
  /// exactly what this editor does not do. @see headerMatches / fixMapHeader.
  Q_PROPERTY(int mapInd READ mapInd WRITE setMapInd NOTIFY changed)

  /// Loaded tileset id (`wCurMapTileset`) -- where the map's GRAPHICS come from.
  Q_PROPERTY(int tilesetInd READ tilesetInd WRITE setTilesetInd NOTIFY changed)

  /// Which tileset's BLOCKS the map is built from (the save's `blockPtr`). Normally the same
  /// tileset as the graphics -- but they are two separate pointers in the save, and a console
  /// draws exactly what they say. -1 when `blockPtr` matches no tileset in the game at all.
  Q_PROPERTY(int blocksetInd READ blocksetInd WRITE setBlocksetInd NOTIFY changed)
  /// Do the blocks and the graphics come from the same tileset? (They normally do.)
  Q_PROPERTY(bool blocksetIsTileset READ blocksetIsTileset NOTIFY changed)
  Q_PROPERTY(QString blocksetName READ blocksetName NOTIFY changed)

  /// Does the save's stored map header (its size) agree with the map it says it is on? Changing the
  /// map id alone leaves it stale -- and we SAY so rather than quietly rewriting six more bytes.
  Q_PROPERTY(bool headerMatches READ headerMatches NOTIFY changed)

  /// The palette as a plain percentage -- 0 is black, 100 is a normal screen. What the picker shows.
  Q_PROPERTY(int contrastPercent READ contrastPercent NOTIFY changed)
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

  // ── The sprite set (the "cached sprites") ─────────────────────────────────────
  //
  // 12 bytes the game keeps for the CURRENT outdoor map: the eleven sprite pictures it has loaded
  // into VRAM, and the id of the set they came from. It is a CACHE -- and one the game throws away
  // the moment it loads your save (`LoadMapData` zeroes the id, then `InitOutsideMapSprites`
  // recomputes both from the map you are standing on). See notes/reference/sprite-sets.md.

  /// What the save says is cached: the set id (0-10; 0 = nothing).
  Q_PROPERTY(int spriteSetId READ spriteSetId NOTIFY changed)
  /// Its name ("Pallet & Viridian", "Cycling Road"...), or "None".
  Q_PROPERTY(QString spriteSetName READ spriteSetName NOTIFY changed)

  /// What the GAME would load here, for this map, at the player's position. 0 for an indoor map --
  /// indoor maps have no sprite set at all, and the game leaves whatever was cached alone.
  Q_PROPERTY(int mapSpriteSetId READ mapSpriteSetId NOTIFY changed)
  Q_PROPERTY(QString mapSpriteSetName READ mapSpriteSetName NOTIFY changed)
  /// Is this map one the game loads a sprite set for at all? (Outdoor maps only.)
  Q_PROPERTY(bool mapHasSpriteSet READ mapHasSpriteSet NOTIFY changed)
  /// Does the cache agree with what the game would load? (Shown, never silently corrected.)
  Q_PROPERTY(bool spriteSetMatchesMap READ spriteSetMatchesMap NOTIFY changed)

public:
  MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset, AreaGeneral* general,
           AreaLoadedSprites* sprites = nullptr, AreaSprites* npcs = nullptr,
           AreaWarps* warps = nullptr);

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

  int frame() const;
  void setFrame(int frame);   ///< A VIEW setting. Writes nothing to the save.

  int mapInd() const;
  void setMapInd(int ind);        ///< Writes `wCurMap`. ONE byte. @see fixMapHeader.

  int tilesetInd() const;
  void setTilesetInd(int ind);    ///< Writes `wCurMapTileset` + that tileset's pointers.

  int blocksetInd() const;
  void setBlocksetInd(int ind);   ///< Writes `blockPtr` (and its bank). Nothing else.
  bool blocksetIsTileset() const;
  QString blocksetName() const;

  bool headerMatches() const;
  int contrastPercent() const;

  /// Every map in the game -- `{ ind, name, incomplete, copyOf }`, glitch/half-baked ids included
  /// and labelled. 248 of them, and every one renders.
  Q_INVOKABLE QVariantList mapList() const;

  /**
   * @brief Where each connected map bleeds into the ring: `{ dir, name, x, y, w, h }`, buffer px.
   *
   * The ring is not a wall of trees -- it carries the NEIGHBOURING maps' edges, and working out
   * where each strip lands is the hardest arithmetic in the map engine (see
   * reference/map-connections.md). This is what lets you look at it.
   */
  Q_INVOKABLE QVariantList connectionList() const;

  // ── The sprite set ────────────────────────────────────────────────────────────
  int spriteSetId() const;
  QString spriteSetName() const;
  int mapSpriteSetId() const;
  QString mapSpriteSetName() const;
  bool mapHasSpriteSet() const;
  bool spriteSetMatchesMap() const;

  /// Every sprite the game has: `{ ind, name }`. What the eleven slot pickers list.
  Q_INVOKABLE QVariantList spriteList() const;

  /// The ten sprite sets, by their real names: `{ ind, name, sprites }`. (The twelve SPLIT ids are
  /// not offered: the console never stores one -- it resolves them first. @see
  /// SpriteSetDBEntry::getResolvedSet.)
  Q_INVOKABLE QVariantList spriteSetList() const;

  /// The eleven cached picture ids, as the save holds them: `{ slot, ind, name }`.
  Q_INVOKABLE QVariantList cachedSprites() const;

  /// Put @p picture in cache slot @p slot. ONE byte.
  Q_INVOKABLE void setCachedSprite(int slot, int picture);

  /// Fill the cache with the set @p setId holds (all eleven slots + the id). A deliberate act.
  Q_INVOKABLE void applySpriteSet(int setId);

  /// Fill it with what the GAME would load here -- this map's set, at the player's position.
  Q_INVOKABLE void applyMapSpriteSet();

  /// Put the save's stored map size back to what this map actually is. The ONE deliberate way those
  /// bytes change -- offered, never done behind your back. @see headerMatches.
  Q_INVOKABLE void fixMapHeader();
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

  /**
   * @brief Everybody else on the map -- the other fifteen sprite slots.
   *
   * One entry per used slot: `{ slot, picture, name, x, y, facing, rectX/Y/W/H, source,
   * inSpriteSet, missable }`. `source` is an `image://player/npc/...` id, so the artwork comes
   * back through the OBJECT palette (which is the one the glitch palettes damage). `x`/`y` are
   * map coordinates with the game's +4 bias already taken off -- what a player would call them.
   *
   * `inSpriteSet` is false when this map has **not loaded** that sprite's picture: the console
   * would draw garbage there, and the canvas says so rather than quietly drawing it correctly.
   * @see notes/reference/sprite-sets.md
   */
  Q_INVOKABLE QVariantList npcList() const;

  /// Move sprite @p slot to map (@p x, @p y). **Exactly two bytes** (mapX, mapY), the game's +4
  /// bias included. The drag-on-the-canvas path. Out-of-bounds coordinates are clamped to the
  /// map -- a sprite parked in the border ring is one the game will never show you.
  Q_INVOKABLE void moveNpc(int slot, int x, int y);

  /// Put a sprite of picture @p pictureID at map (@p x, @p y). @return its new slot, or -1 if the
  /// map is full (15 NPCs plus the player). The drag-from-the-Characters-bar path.
  Q_INVOKABLE int addNpc(int pictureID, int x, int y);

  /// Delete sprite @p slot. **The rest slide up** -- the game packs its sprite slots and so do we.
  Q_INVOKABLE void removeNpc(int slot);

  /// Move the PLAYER to map (@p x, @p y). He is slot 0 and he is draggable like anybody else --
  /// but his position lives in his OWN two bytes (`wXCoord`/`wYCoord`), not in the sprite table.
  Q_INVOKABLE void movePlayer(int x, int y);

  /// How many more sprites this map can hold. 0 means the "+" is dead, and we say so *before* the
  /// user hits it rather than swallowing the drop.
  Q_INVOKABLE int npcRoomLeft() const;

  /// Every sprite picture in the game, for the Characters bar: `{ ind, name, group, source,
  /// inSpriteSet }`, in group order. `source` is the artwork; `inSpriteSet` says whether THIS map
  /// has that picture loaded (an amber flag on the canvas, not a refusal).
  Q_INVOKABLE QVariantList spriteCatalog() const;

  /// The sprite in @p slot, as `npcList()` shapes it -- or an empty map. What the Details panel
  /// reads when a sprite is selected.
  Q_INVOKABLE QVariantMap npcAt(int slot) const;

  /// Every editable byte of sprite @p slot, named and explained: a list of
  /// `{ group, key, label, blurb, value, min, max, kind, options }`. This is the Details panel's
  /// content -- **every byte, full range, hack values included and flagged, never refused**.
  Q_INVOKABLE QVariantList npcFields(int slot) const;

  /// Write one of @ref npcFields' keys on sprite @p slot. One field, one byte.
  Q_INVOKABLE void setNpcField(int slot, const QString& key, int value);

  /**
   * @brief Has the user changed this map's cast in this session?
   *
   * The game **rebuilds the map's original cast from ROM the moment the player leaves the map and
   * walks back in** (verified on the cartridge -- see notes/reference/sprites.md, Part 6), so the
   * screen owes the user that sentence *once they have made an edit that it applies to*.
   *
   * @warning It cannot be answered by comparing the save against the ROM, and a first attempt that
   * did was **wrong**: a real save's cast **already** differs from the ROM's, because WALK sprites
   * wander. Pallet Town's Girl stands at (3, 8) in the cartridge and at (3, 6) in the fixture save
   * -- she had simply walked. Comparing would have flashed the warning on essentially every save
   * ever loaded, which is noise, and noise is a bug. So we track the *edit*, not the difference.
   */
  Q_INVOKABLE bool npcsEdited() const;

  /**
   * @brief Somewhere worth looking, in BUFFER pixels: `{ ok, x, y, label }`.
   *
   * The "zoom to..." menu's whole content. @p kind is one of
   * `player` · `sprite` · `object` · `door` · `warp` · `xy` · `connection`.
   *
   * The "random" ones pick a *different* one each time you ask, which is the point: they are how you
   * go and LOOK at a map you have never seen, rather than hunting for something interesting on a
   * 78-block route. `xy` lands on the centre of a random **block** (not a tile, not a pixel).
   *
   * `ok: false` when this map simply has none of that thing -- and the menu greys the entry out and
   * says so, rather than doing nothing when you click it.
   */
  Q_INVOKABLE QVariantMap zoomTarget(const QString& kind);

  /// Which of @ref zoomTarget's kinds this map actually HAS. Keyed by kind, `true`/`false`.
  Q_INVOKABLE QVariantMap zoomTargetsAvailable() const;

  /// Every warp on this map, as a centre in BUFFER pixels: `{ x, y, ind }`.
  Q_INVOKABLE QVariantList warpPoints() const;

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

  /**
   * @brief What is under the cursor at buffer pixel (@p px, @p py) -- the status bar's whole job.
   *
   * `{ valid, blockX, blockY, block, border, mapBlockX, mapBlockY, mapTileX, mapTileY, tile,
   *    label }` -- buffer coords AND map coords (-1 out in the border ring, rather than a negative
   * that reads like a real coordinate), the block id, the tile id, and what that tile DOES in
   * words. Called on every mouse-move, so it renders nothing.
   */
  Q_INVOKABLE QVariantMap describeAt(int px, int py) const;

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
  /// The animation step moved on (a view setting; the save is untouched).
  void frameChanged();
  /// The rendered image's URL changed -- a new frame, or a new map/tileset/palette.
  void sourceChanged();
  /// The shown layers changed (a view setting, not save data).
  void overlayChanged();
  /// The selected block changed.
  void selectionChanged();

private:
  /// Rebuild the selection when the map changes under it.
  void revalidateSelection();

  /// The block filling the 3-block ring -- the SAVE's `wMapBackgroundTile`, which is what the
  /// console reads. Edit it and the edge of the world changes.
  int borderBlock() const;

  /// The overworld buffer as it is ACTUALLY DRAWN (the save's border block included). Every question
  /// about the map's blocks goes through here, so nothing can disagree with what is on screen.
  MapEngine::Buffer mapBuffer() const;

  AreaLoadedSprites* sprites = nullptr; ///< The save's live sprite-set cache (may be null in tests).
  AreaSprites* npcs = nullptr;    ///< The save's live map cast -- the 16 sprite slots (may be null).
  AreaWarps* warps = nullptr;     ///< The map's warp points (may be null in tests).

  /// @see npcsEdited. Set by any edit to the cast; never by loading one.
  bool castEdited = false;
  AreaMap* map = nullptr;         ///< The save's live map.
  AreaPlayer* player = nullptr;   ///< The save's live player position.
  AreaTileset* tileset = nullptr; ///< The save's live tileset.
  AreaGeneral* general = nullptr; ///< The save's live "contrast" (wMapPalOffset).

  int shownLayers = 0;            ///< A VIEW setting. Off by default: the map is the point.
  int animFrame = 0;              ///< The animation step. A VIEW setting; 0 is the still map.
  int selX = -1;                  ///< Selected block, in buffer coords. -1 = nothing selected.
  int selY = -1;
};
