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
class AreaSign;
class AreaMap;
class AreaPlayer;
class AreaTileset;
class WorldGeneral;

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

  /**
   * @brief Show the sprite fields the console **works out again every time it loads the save**.
   *
   * The walk state, the on-screen pixels, the VRAM slot. Roughly a **third** of a sprite is made of
   * them. Every one is real and every one is editable -- they simply are not what anybody opened the
   * panel for, and stacking them under the fields that DO matter is the difference between a panel
   * and a hex dump.
   *
   * **OFF by default** (Twilight, 2026-07-13). Off, @ref npcFields does not emit them **at all** --
   * not greyed, not collapsed: absent. On, they appear, each wearing its yellow "!". The switch is
   * hard right on the map's toolbar.
   *
   * ⚠️ The filtering lives in the MODEL, on purpose: no view can leak one by accident, and a test can
   * prove they are gone.
   */
  Q_PROPERTY(bool showScratch READ showScratch WRITE setShowScratch NOTIFY showScratchChanged)
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
  // ── The doors (warps) ─────────────────────────────────────────────────────────
  //
  // ⚠️ Read notes/reference/warps.md. The short version, because it changes what this API means:
  //
  // An edited warp is **genuinely live** on Continue. `LoadMapHeader` rebuilds the warp list from
  // ROM on every map load -- but `LoadMainData` sets `BIT_NO_PREVIOUS_MAP` on the saved tileset
  // byte as it reads the save, so the very next `LoadMapHeader` bails out before it copies
  // anything. Verified on the cartridge, including a 4th warp invented in a 3-warp town.
  //
  // And the game puts the map's original doors back the moment the player leaves and walks in
  // again. Which the screen SAYS. @see warpsEdited.

  /// **Where a `$FF` door takes you** -- `wLastMap`, and the most consequential warp byte there is.
  /// Every building's exit warp is `$FF` ("back outside"); this is the map it means. It lives in
  /// WorldGeneral, not AreaWarps, which is exactly why nobody editing warps ever saw it.
  Q_PROPERTY(int lastMap READ lastMap WRITE setLastMap NOTIFY warpsChanged)
  Q_PROPERTY(QString lastMapName READ lastMapName NOTIFY warpsChanged)

  /// **Where you wake up** -- `wLastBlackoutMap`. Blacking out, Dig and Escape Rope all land here.
  Q_PROPERTY(int lastBlackoutMap READ lastBlackoutMap WRITE setLastBlackoutMap NOTIFY warpsChanged)
  Q_PROPERTY(QString lastBlackoutMapName READ lastBlackoutMapName NOTIFY warpsChanged)

  int lastMap() const;
  void setLastMap(int ind);
  QString lastMapName() const;

  int lastBlackoutMap() const;
  void setLastBlackoutMap(int ind);
  QString lastBlackoutMapName() const;

  /**
   * @brief Every door on this map: `{ ind, x, y, destMap, destWarp, rectX/Y/W/H, isReturn,
   *        destName, destLabel, destValid, arrivalCount }`.
   *
   * `x`/`y` are map TILES (what a player would count); the `rect*` are BUFFER pixels, the ring
   * included, so the canvas can draw a chip straight on them.
   *
   * `isReturn` is the `$FF` door -- "back outside" -- and `destName` then resolves through
   * @ref lastMap, live, so changing "Outside is…" re-labels every one of them at once.
   *
   * `destValid` is false when the destination map has no arrival point with that index: the
   * console would copy four arbitrary ROM bytes into the view pointer and the player's coords.
   * Shown, flagged, **never refused**.
   */
  Q_INVOKABLE QVariantList warpList() const;

  /// One door, as @ref warpList shapes it -- or an empty map. What the Details panel reads.
  Q_INVOKABLE QVariantMap warpAt(int ind) const;

  /// Move door @p ind to map tile (@p x, @p y). **Exactly two bytes.** The drag-on-canvas path.
  Q_INVOKABLE void moveWarp(int ind, int x, int y);

  /// Put a new door at map tile (@p x, @p y). It defaults to `$FF` ("back outside"), because that
  /// is what a door usually is. @return its index, or -1 if the map already has @ref warpRoomLeft
  /// of 0 -- and the caller is expected to have said so *before* the click, not after.
  Q_INVOKABLE int addWarp(int x, int y);

  /// Delete door @p ind. The rest slide up -- the game packs its warp list and so do we.
  ///
  /// ⚠️ **Every other door that pointed at an arrival point on THIS map is unaffected** (they name
  /// arrival points, not warps), but a door on this map pointing at an index *past* the new count
  /// of some other map is not our business either. What compaction really breaks is nothing in the
  /// save -- the warp list is positional and the game reads it positionally.
  Q_INVOKABLE void removeWarp(int ind);

  /// How many more doors this map can hold (32 max). 0 means the tool is dead, and we say so
  /// *before* the click rather than swallowing it.
  Q_INVOKABLE int warpRoomLeft() const;

  /// Every editable byte of door @p ind, named and explained -- the Details panel's content.
  /// Same `{ group, key, label, blurb, value, min, max, kind, options, scratch }` shape as
  /// @ref npcFields, so the field kit draws it with no new code.
  Q_INVOKABLE QVariantList warpFields(int ind) const;

  /// Write one of @ref warpFields' keys on door @p ind. `xy` arrives packed as `x | (y << 8)`.
  Q_INVOKABLE void setWarpField(int ind, const QString& key, int value);

  /**
   * @brief The map's warp STATE -- the twelve bytes around the doors, each named in English.
   *
   * Same field shape again, plus two markers that are **different facts** and must not be
   * collapsed into one grey "unused":
   *
   *  | marker | means |
   *  |--------|-------|
   *  | `scratch` | ⚠️ **The console rewrites this on load.** `wStatusFlags3` shares an address with `wCableClubDestinationMap` and `SpecialEnterMap` zeroes it -- so the *whole byte* dies on every Continue. Console-verified. |
   *  | `dead`    | 💀 **It survives perfectly, and nothing in the game ever reads it.** Two writes, zero reads, across the whole disassembly. |
   *  | `gun`     | 🔫 **The console has no table entry for most values of this.** @see AreaWarps::isLegalFlyMap. |
   *
   * Both `scratch` kinds are filtered out unless @ref showScratch -- the same switch, and the same
   * reason, as the sprite panel's.
   */
  Q_INVOKABLE QVariantList warpStateFields() const;

  /// Write one of @ref warpStateFields' keys.
  Q_INVOKABLE void setWarpStateField(const QString& key, int value);

  /// The **13** maps a Fly / special warp may legally name: `{ value, name, hack }`.
  /// (`hack` is never true here -- this list *is* the clean set. @see mapList for all 248.)
  Q_INVOKABLE QVariantList flyWarpMapList() const;

  /// The **7** maps a hole may legally drop you onto: `{ value, name }`.
  Q_INVOKABLE QVariantList dungeonWarpMapList() const;

  /// The legal hole numbers for @p map (1-based). Empty if @p map has no holes -- and Victory
  /// Road 2F has a hole **2** and no hole 1, which is exactly why this is a per-map question.
  Q_INVOKABLE QVariantList dungeonHoleList(int map) const;

  /// The dungeon map currently named in the save (`wDungeonWarpDestinationMap`). The hole picker
  /// binds to it: which holes are legal depends on which floor you fell from, so the two controls
  /// are not independent and pretending they are is how you get an illegal pair.
  Q_PROPERTY(int warpDungeonMap READ warpDungeonMap NOTIFY warpsChanged)
  int warpDungeonMap() const;

  /// "→ Viridian City, arrival point 2 (11, 5)" -- resolved, in words. Empty when it resolves to
  /// nothing, so the caller can say *that* instead.
  Q_INVOKABLE QString warpDestLabel(int destMap, int destWarp) const;

  /// Has the user changed this map's doors in this session? @see npcsEdited -- same rule, same
  /// reason (we track the EDIT, never a diff against the ROM), and the same sentence is owed:
  /// the game restores the map's original doors when the player leaves and walks back in.
  Q_INVOKABLE bool warpsEdited() const;

  // ── The signs (placards) ──────────────────────────────────────────────────────
  //
  // Signs are the doors' quieter sibling: a tile you can READ. They ride the SAME persistence
  // linchpin as warps (`.loadSignData` sits inside `LoadMapHeader`, behind `BIT_NO_PREVIOUS_MAP`),
  // so an edited sign is genuinely live on Continue and the game restores the map's originals when
  // the player leaves and walks back in. The model was already correct -- the rare pass with no bug
  // to fix first. See notes/reference/signs.md.

  /// Every sign on this map: `{ ind, x, y, rectX/Y/W/H, textId, preview, category, scripted,
  /// textValid }`. `x`/`y` are map TILES; the `rect*` are BUFFER pixels (ring included) so the
  /// canvas can draw a chip straight on them. `preview` is the sign's real words (one line);
  /// `textValid` is false when the id points past this map's text table.
  Q_INVOKABLE QVariantList signList() const;

  /// One sign, as @ref signList shapes it -- or an empty map. What the Details panel reads.
  Q_INVOKABLE QVariantMap signAt(int ind) const;

  /// Move sign @p ind to map tile (@p x, @p y). **Exactly two bytes** (its Y and X, in the sign
  /// coord array). The drag-on-canvas path.
  Q_INVOKABLE void moveSign(int ind, int x, int y);

  /// Put a new sign at map tile (@p x, @p y). It defaults to the map's first sign text (or text id
  /// 1), because a fresh placard should say something real. @return its index, or -1 if the map is
  /// already at its @ref signRoomLeft of 0.
  Q_INVOKABLE int addSign(int x, int y);

  /// Delete sign @p ind. The rest slide up -- the game packs its sign list and so do we.
  Q_INVOKABLE void removeSign(int ind);

  /// How many more signs this map can hold (16 max). 0 means the tool is dead, and we say so
  /// *before* the click rather than swallowing it.
  Q_INVOKABLE int signRoomLeft() const;

  /// Every editable byte of sign @p ind, named and explained -- the Details panel's content.
  /// Same `{ group, key, label, blurb, value, min, max, kind, options, scratch }` shape as
  /// @ref warpFields, so the field kit draws it with no new code.
  Q_INVOKABLE QVariantList signFields(int ind) const;

  /// Write one of @ref signFields' keys on sign @p ind. `xy` arrives packed as `x | (y << 8)`.
  Q_INVOKABLE void setSignField(int ind, const QString& key, int value);

  /// The map's whole text table, **grouped** (Signs / People / Other) with the real words, for the
  /// sign's "Says…" picker: `{ value, name, header, hack, category }`. Selection commits; the raw
  /// byte box (kind `enum`) reaches every id 0..255 for the hack values. @see mapTextList (the flat
  /// NPC-panel variant).
  Q_INVOKABLE QVariantList signTextList() const;

  /// The one-line preview of what text id @p textId says on this map -- "(scripted text)" for a
  /// script entry, empty when the id points past the table. What the canvas chip shows.
  Q_INVOKABLE QString signTextPreview(int textId) const;

  /// Has the user changed this map's signs in this session? @see warpsEdited -- same rule, same
  /// reason, and the same sentence is owed (restored on re-entry).
  Q_INVOKABLE bool signsEdited() const;

  // ── The player (his 26-byte map-state block) ──────────────────────────────────
  //
  // ⚠️ Read notes/reference/player-state.md. The v1 model's byte offsets and bit numbers are all
  // correct, but the load story is the point: **ten of the 26 are rewritten the instant the save
  // loads** (wPlayerDirection FORCED to DOWN, wStatusFlags3 zeroed, strength reset unless the
  // battle-over bit is set, the door/ledge/link bits and Jumping Y cleared), **three are dead**
  // (the two special-warp offsets and the "unused card key" bit), and several are misnamed.
  // Console-verified byte-by-byte: scripts/emu/probe_player_state.py.

  /**
   * @brief Every editable byte of the player's map state, named and explained -- the Details panel's
   *        content when the player (slot 0) is selected.
   *
   * Same `{ group, key, label, blurb, value, min, max, kind, options }` shape as @ref warpStateFields,
   * plus the same two "this does nothing" markers -- but the player's are a **third fact** apart:
   *
   *  | marker | means |
   *  |--------|-------|
   *  | `scratch`/`mark:"reload"` | ⚠️ **Rewritten on load.** Unlike the warp block (one wipe does the lot), the player bytes are rewritten by DIFFERENT routines, so each carries its OWN `note`. |
   *  | `dead`/`mark:"dead"`      | 💀 **Survives perfectly, nothing ever reads it.** |
   *
   * Both are filtered out unless @ref showScratch -- the same switch, and the same reason, as the
   * sprite and warp panels'. @see PlayerField.qml
   */
  Q_INVOKABLE QVariantList playerFields() const;

  /// Write one of @ref playerFields' keys. Each writes **exactly one member** (one byte, or one bit
  /// via AreaPlayer::save's bit-preserving setBit) and nothing else. tst_player byte-diffs the save.
  Q_INVOKABLE void setPlayerField(const QString& key, int value);

public:
  MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset, AreaGeneral* general,
           AreaLoadedSprites* sprites = nullptr, AreaSprites* npcs = nullptr,
           AreaWarps* warps = nullptr, WorldGeneral* world = nullptr,
           AreaSign* signs = nullptr);

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

  // ── The connecting routes (edge connections) — EDITING ────────────────────────
  //
  // ⚠️ Read notes/reference/map-connections.md, the "Editing a connection — the human model" section.
  // A connection has only TWO human inputs — the neighbour map and one signed offset (−27…+27) — and
  // the other nine bytes are MACRO-DERIVED (MapEngine::connectionBytes, the cartridge-verified path,
  // never MapDBEntryConnect::stripSize()). So editing is offset-driven with auto-derive by default;
  // raw-byte editing (break-sync) arrives with the inspector (Phase 7c). Rotation is NOT representable
  // in the save; the meaningful equivalent is re-homing to another edge (@ref rehomeConnection).

  /// The four edges, existing or not: `{ dir, dirName, exists, toMap, toName, offset, synced,
  /// offsetMin, offsetMax }`. What the canvas arrows and the inspector both read.
  Q_INVOKABLE QVariantList connectionEditList() const;

  /// Is there a connection on @p dir? (@p dir is MapDBEntryConnect::ConnectDir: N 0, S 1, E 2, W 3.)
  Q_INVOKABLE bool connectionExists(int dir) const;

  /// The signed offset a saved connection was built with, recovered from its stored bytes. 0 if none.
  Q_INVOKABLE int connectionOffsetOf(int dir) const;

  /// Do @p dir's stored bytes equal what the macro derives for (its neighbour, its recovered offset)?
  /// A raw-edited (desynced) connection returns false — the canvas then offers the richer handles and
  /// the inspector unlocks the raw fields.
  Q_INVOKABLE bool connectionSynced(int dir) const;

  /// Add a connection on @p dir to map @p toMapInd, centred (offset 0). Writes the flag bit and the
  /// derived 11-byte slot, and nothing else. @return true, or false if @p dir already has one.
  Q_INVOKABLE bool addConnection(int dir, int toMapInd);

  /// Remove @p dir's connection. Clears the flag bit only (byte fidelity: the stale slot is left as
  /// it lies, exactly as delete-a-warp leaves the packed tail).
  Q_INVOKABLE void removeConnection(int dir);

  /// Point @p dir's connection at a different neighbour @p toMapInd, keeping its offset; re-derives.
  Q_INVOKABLE void setConnectionMap(int dir, int toMapInd);

  /// Slide @p dir's connection to signed @p offset and re-derive the strip. The drag-on-canvas path;
  /// writes only the slot's bytes.
  Q_INVOKABLE void setConnectionOffset(int dir, int offset);

  /// Re-home @p fromDir's connection onto @p toDir (same neighbour + offset, recomputed for the new
  /// edge) — the explicit "attach to another side" option, never automatic. No-op if @p fromDir has
  /// none or @p toDir already has one.
  Q_INVOKABLE void rehomeConnection(int fromDir, int toDir);

  /// How many more connections this map can hold (4 − current).
  Q_INVOKABLE int connectionRoomLeft() const;

  /// Has the user changed this map's connections this session? Same honesty rule as @ref warpsEdited —
  /// the game restores the map's original connections when the player leaves and walks back in.
  Q_INVOKABLE bool connectionsEdited() const;

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

  /**
   * @brief Put a **random** character at map (@p x, @p y) -- the toolbar's 🧍+ tool.
   *
   * ⚠️ **Random out of the pictures THIS MAP HAS LOADED**, never out of all 72. A picture the map
   * has not loaded is one the console draws as garbage (the VRAM slot lookup has no bounds check --
   * see notes/reference/sprite-sets.md), and a one-click tool that lands you in that state one time
   * in six is a trap, not a shortcut. @see vramPictures.
   *
   * The Characters bar stays the *precise* path -- drag exactly who you want, and be told when the
   * one you want isn't loaded. This is the *fast* path, and it is safe by construction.
   *
   * @return the new slot, or -1 if the map is full or has no pictures loaded at all.
   */
  Q_INVOKABLE int addRandomLoadedNpc(int x, int y);

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

  /**
   * @brief Every editable byte of sprite @p slot, named and explained. The Details panel's content.
   *
   * A list of `{ group, key, label, blurb, value, min, max, kind, options, scratch }` --
   * **every byte, full range, hack values included and flagged, never refused**.
   *
   * ⚠️ **`kind` is the point.** It says what the value IS, so the panel can draw the right control
   * rather than a number box with a paragraph beside it:
   *
   *  | kind | the control |
   *  |------|-------------|
   *  | `picture` | a grid of the actual artwork -- you pick a character by looking at them |
   *  | `coords`  | X and Y **together**; the packed value is `x \| (y << 8)` |
   *  | `pixels`  | the same packing, for the two on-screen pixel pairs |
   *  | `enum`    | a combo. The raw byte box appears **only** for a value no option names |
   *  | `frames`  | a countdown, drawn as the duration it is -- never as a number |
   *  | `team`    | which of a trainer class's rosters |
   *  | `byte`    | the last resort |
   *
   * `scratch` marks a byte **the console recomputes when it loads the save**. It wears a yellow "!".
   * It is not hidden and not refused -- just labelled, so nobody carefully sets a value the game is
   * going to throw away.
   *
   * The `Talking to it` group is **variable**: the sprite's kind lives in bits 6-7 of its text byte
   * (`TRAINER`/`ITEM` -- constants/map_object_constants.asm), and it decides whether the item picker
   * or the trainer class + roster exist at all. A Pokéball is not shown a trainer roster.
   *
   * @see setNpcField, mapTextList, itemList, trainerClassList
   */
  Q_INVOKABLE QVariantList npcFields(int slot) const;

  /// Write one of @ref npcFields' keys on sprite @p slot.
  ///
  /// Most keys are one byte. The composite ones (`mapXY`, `screenXY`, `gridXY`) arrive packed as
  /// `x | (y << 8)`; `spriteKind` and `textID` are **two halves of the same byte** and each writes
  /// only its own bits, leaving the other's exactly as they were.
  Q_INVOKABLE void setNpcField(int slot, const QString& key, int value);

  /// The screen box and the draw area **for an arbitrary player position**, in buffer pixels:
  /// `{ screenX, screenY, screenW, screenH, drawX, drawY, drawW, drawH }`.
  ///
  /// The bound `screenX`/`scratchX`/… properties answer for where the player *is*. This answers for
  /// where he is *being dragged to*, so the two boxes track him live instead of snapping into place
  /// when you let go. Same MapEngine routines, so they can never disagree.
  /// **The four shades a contrast value really renders the map in** -- the genuine `rBGP` byte the
  /// console would be holding, glitch reads across the fade table's seam included.
  ///
  /// The contrast strip wears these, so the segments *are* the palette: slide along it and you watch
  /// the map go dark before the map does. (It used to paint them in the app's accent blue, which told
  /// you a value was unusual and nothing at all about what it would do.)
  Q_INVOKABLE QVariantList contrastShades(int contrast) const;

  // ── The output palette (Game Boy Color / custom colour filter) ───────────────────────────────
  //
  // ⚠️ A VIEW SETTING. Not one save byte is touched. Orthogonal to `contrast`. @see MapEngine.

  /// How the map's four shades are coloured: 0 Grey · 1 Game Boy · 2 Super Game Boy · 3 Custom.
  Q_PROPERTY(int colourMode READ colourMode WRITE setColourMode NOTIFY colourModeChanged)

  int colourMode() const;
  void setColourMode(int mode);

  /// The named presets for the picker: `{ mode, name, swatch:[4 colours] }`.
  Q_INVOKABLE QVariantList colourPresets() const;

  /// The four current custom colours, lightest first -- what the Custom swatches edit.
  Q_INVOKABLE QVariantList customColours() const;

  /// Set one custom colour (@p shade 0 lightest .. 3 darkest). Switches to Custom mode.
  Q_INVOKABLE void setCustomColour(int shade, const QColor& colour);

  Q_INVOKABLE QVariantMap viewBoxesAt(int x, int y) const;

  bool showScratch() const;              ///< @see showScratch property.
  void setShowScratch(bool show);        ///< @see showScratch property.

  /**
   * @brief Is this an INDOOR map? (`wCurMap >= FIRST_INDOOR_MAP`, i.e. id >= `$25`.)
   *
   * It is not decoration -- it decides the whole sprite-artwork question. @see vramPictures.
   */
  Q_INVOKABLE bool mapIsIndoors() const;

  /**
   * @brief **The pictures the console would actually have in video memory** after loading this save.
   *
   * ⚠️ This is the routine "will my sprite render?" turns on, and until 2026-07-13 we answered it by
   * reading the save's cached sprite set -- **which the game throws away**. It now does what
   * `engine/overworld/map_sprites.asm` does:
   *
   * **Outdoors** (`InitOutsideMapSprites`): the set comes from the **ROM table** `MapSpriteSets[map]`
   * (splits resolved against the player's position), its 11 pictures go into VRAM, and each NPC's
   * slot is found by *searching that set for its picture id* — a loop with **no bounds check**, so a
   * picture that isn't in the set runs off the end of `wSpriteSet` into WRAM and lands on an
   * arbitrary VRAM slot. That is the garbage. Nothing in the save can change what is loaded.
   *
   * **Indoors** (`InitMapSprites`): `InitOutsideMapSprites` returns immediately and the game copies
   * **each sprite's own picture** into VRAM, deduplicated, by first appearance. **There is no sprite
   * set indoors** — the cast *is* the set, and any picture draws correctly until the video memory
   * runs out: **10** walking slots and **2** four-tile still slots (picture >= `$3D`).
   *
   * @see pictureWouldRender, pictureWouldRenderIfAdded, notes/reference/sprite-sets.md
   */
  QVector<int> vramPictures() const;

  /// Would the console draw sprite picture @p picture correctly on this map, as things stand?
  Q_INVOKABLE bool pictureWouldRender(int picture) const;

  /// Would it draw correctly if you dropped one **here, now**? Outdoors that is the same question.
  /// **Indoors it is not**: the cast is the set, so the answer is "yes, if there is a slot left".
  Q_INVOKABLE bool pictureWouldRenderIfAdded(int picture) const;

  /// **This map's own scripts**, out of the cartridge: `{ value, name, hack }`, where the name says
  /// who the script belongs to ("3 — Fisher 2"). A text id is an index into *this map's* text table,
  /// so a bare number is meaningless and a name is not.
  Q_INVOKABLE QVariantList mapTextList() const;

  /// Every item an item ball can hold: `{ value, name, hack }`. The glitch items are real bytes, so
  /// they are offered -- and flagged.
  Q_INVOKABLE QVariantList itemList() const;

  /// Every trainer class: `{ value, name, hack }`.
  Q_INVOKABLE QVariantList trainerClassList() const;

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

  /// The save's live sprite list. MapSim runs the game's own state machine straight over these bytes,
  /// so it needs the real objects rather than the QVariant view the QML gets.
  ///
  /// (NOT `sprites()` -- that name is already the sprite-SET cache, which is a different thing.)
  AreaSprites* npcSprites() const { return npcs; }

  /// The block that fills the 3-block border ring. Public because MapSim rebuilds the same overworld
  /// buffer the renderer draws, to ask what tile a sprite would be stepping onto.
  int borderBlock() const;

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
  /**
   * @brief Somebody on the map MOVED -- and **nothing else did**.
   *
   * ⚠️ Deliberately NOT `changed()`. `changed()` is wired to `sourceChanged()`, and `source` is the
   * map's render URL -- so emitting it **re-renders the whole map image**. The walk simulation moves
   * somebody ~60 times a second; routing that through `changed()` re-rendered the entire map ~60
   * times a second to shift one 16x16 sprite, and the frame rate collapsed (Twilight, 2026-07-13).
   *
   * A sprite moving does not change one pixel of the map. Only the canvas's sprite layer listens
   * here. An *edit* (add / move / delete / a field write) emits `changed()` **as well**, because
   * that really can change everything else -- the room left, which layers apply, the video memory.
   */
  void castChanged();

  /**
   * @brief A DOOR moved, or was added, deleted or re-aimed -- and nothing else did.
   *
   * The warp analogue of @ref castChanged, and for the same performance reason: `changed()` is
   * wired to `sourceChanged()`, so emitting it re-renders the whole map image. A warp chip moving
   * does not change one pixel of the map.
   *
   * (An edit that really can change everything else -- adding a door, which changes the room left
   * and can light a layer -- emits `changed()` as well.)
   */
  void warpsChanged();

  /**
   * @brief A SIGN moved, or was added, deleted or re-worded -- and nothing else did.
   *
   * The sign analogue of @ref warpsChanged / @ref castChanged, and for the same performance reason:
   * `changed()` is wired to `sourceChanged()`, so emitting it re-renders the whole map image. A sign
   * chip moving does not change one pixel of the map. (An edit that changes the room left -- adding a
   * sign -- emits `changed()` as well.)
   */
  void signsChanged();

  /// The "Reloaded values" switch moved -- the Details panel has a different set of fields now.
  void showScratchChanged();

  /// The output colour palette changed (a view setting; the save is untouched).
  void colourModeChanged();

  /// The shown layers changed (a view setting, not save data).
  void overlayChanged();
  /// The selected block changed.
  void selectionChanged();

private:
  /// Rebuild the selection when the map changes under it.
  void revalidateSelection();

  /// The block filling the 3-block ring -- the SAVE's `wMapBackgroundTile`, which is what the
  /// console reads. Edit it and the edge of the world changes.
  /// The overworld buffer as it is ACTUALLY DRAWN (the save's border block included). Every question
  /// about the map's blocks goes through here, so nothing can disagree with what is on screen.
  MapEngine::Buffer mapBuffer() const;

  AreaLoadedSprites* sprites = nullptr; ///< The save's live sprite-set cache (may be null in tests).
  AreaSprites* npcs = nullptr;    ///< The save's live map cast -- the 16 sprite slots (may be null).
  AreaWarps* warps = nullptr;     ///< The map's doors (may be null in tests).
  AreaSign* signsData = nullptr;  ///< The map's signs (may be null in tests). Named `signsData` to
                                  ///  avoid colliding with the sprite-set `sprites` naming muddle.
  WorldGeneral* world = nullptr;  ///< Where `wLastMap` + `wLastBlackoutMap` actually live.

  /// @see npcsEdited. Set by any edit to the cast; never by loading one.
  bool castEdited = false;

  /// @see warpsEdited. Set by any edit to the doors; never by loading them.
  bool warpsWereEdited = false;

  /// @see signsEdited. Set by any edit to the signs; never by loading them.
  bool signsWereEdited = false;

  /// @see connectionsEdited. Set by any edit to the edge connections; never by loading them.
  bool connectionsWereEdited = false;

  /// @see showScratch. OFF -- it is clutter, and it is a third of the panel.
  bool showScratchFields = false;
  AreaMap* map = nullptr;         ///< The save's live map.
  AreaPlayer* player = nullptr;   ///< The save's live player position.
  AreaTileset* tileset = nullptr; ///< The save's live tileset.
  AreaGeneral* general = nullptr; ///< The save's live "contrast" (wMapPalOffset).

  int shownLayers = 0;            ///< A VIEW setting. Off by default: the map is the point.
  int animFrame = 0;              ///< The animation step. A VIEW setting; 0 is the still map.
  int selX = -1;                  ///< Selected block, in buffer coords. -1 = nothing selected.
  int selY = -1;
};
