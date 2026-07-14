/*
  * Copyright 2020 Twilight
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
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class WarpData;
class MapDBEntry;

constexpr var8 maxWarps = 32; ///< Maximum warps on a map (the ROM's `MAX_WARP_EVENTS`).

/**
 * @brief The current map's doors, plus the live warp-transition state.
 *
 * Two things in one: the list of WarpData **warp points** (up to @ref maxWarps), and the
 * dozen bytes that describe a warp **in flight** -- fly, hole, Dig, scripted.
 *
 * ⚠️ **Read `notes/reference/warps.md` before touching any of this.** It is verified against
 * the cartridge, and several of these fields are not what v1 called them:
 *
 * - **The warp list is LIVE on Continue.** `LoadMapHeader` rebuilds it from ROM on every map
 *   load -- but `LoadMainData` sets `BIT_NO_PREVIOUS_MAP` on the saved tileset byte as it
 *   reads the save, which makes the very next `LoadMapHeader` bail out before it copies
 *   anything. So an edited warp really is there. The game restores the map's original doors
 *   the moment the player leaves the map and walks back in.
 * - ⚠️ **@ref scriptedWarp and @ref isDungeonWarp CANNOT survive a save.** They live in
 *   `wStatusFlags3`, which shares an address with `wCableClubDestinationMap` -- and
 *   `SpecialEnterMap`, on the Continue path, writes 0 to it. **The whole byte is zeroed on
 *   every load** (console-verified: wrote `$FF`, read back `$00`).
 * - 💀 **@ref warpedFromWarp and @ref warpedfromMap are DEAD.** The game writes them on every
 *   warp and **nothing anywhere reads them**. Two writes, zero reads, game-wide.
 * - 🔫 **@ref specialWarpDestMap and the @ref dungeonWarpDestMap / @ref whichDungeonWarp pair
 *   are LOADED GUNS.** Their lookup tables have no bounds check. @see isLegalFlyMap,
 *   isLegalDungeonWarp.
 *
 * The two bytes that matter most for warps -- `wLastMap` (where a `$FF` door returns you) and
 * `wLastBlackoutMap` (where Dig / Escape Rope / blacking out put you) -- are **not here**.
 * They live in WorldGeneral, and always did.
 *
 * @see WarpData (a warp point), Area, MapDBEntry, notes/reference/warps.md.
 */
class SAVEFILE_AUTOPORT AreaWarps : public QObject
{
  Q_OBJECT

  // ── Group C: a script wants a warp (wStatusFlags3 -- ZEROED ON EVERY LOAD) ─────────────
  Q_PROPERTY(bool scriptedWarp MEMBER scriptedWarp NOTIFY scriptedWarpChanged)                 ///< `BIT_WARP_FROM_CUR_SCRIPT`. ⚠️ Wiped on load.
  Q_PROPERTY(bool isDungeonWarp MEMBER isDungeonWarp NOTIFY isDungeonWarpChanged)              ///< `BIT_ON_DUNGEON_WARP`. ⚠️ Wiped on load.

  // ── Group D: the joypad rule (wStatusFlags7) ──────────────────────────────────────────
  Q_PROPERTY(bool forcedWarp MEMBER forcedWarp NOTIFY forcedWarpChanged)                       ///< `BIT_FORCED_WARP` -- doors fire without walking into them.

  // ── Group B: where does it go ─────────────────────────────────────────────────────────
  Q_PROPERTY(int warpDest MEMBER warpDest NOTIFY warpDestChanged)                              ///< `wDestinationWarpID`. `0xFF` = don't move the player.
  Q_PROPERTY(int dungeonWarpDestMap MEMBER dungeonWarpDestMap NOTIFY dungeonWarpDestMapChanged) ///< `wDungeonWarpDestinationMap`. 🔫
  Q_PROPERTY(int specialWarpDestMap MEMBER specialWarpDestMap NOTIFY specialWarpDestMapChanged) ///< `wDestinationMap`. 🔫
  Q_PROPERTY(int whichDungeonWarp MEMBER whichDungeonWarp NOTIFY whichDungeonWarpChanged)      ///< `wWhichDungeonWarp`. **1-based.** 🔫

  // ── Group A: a warp is happening (wStatusFlags6) ──────────────────────────────────────
  Q_PROPERTY(bool flyOrDungeonWarp MEMBER flyOrDungeonWarp NOTIFY flyOrDungeonWarpChanged)     ///< `BIT_FLY_OR_DUNGEON_WARP` -- a special warp is in progress.
  Q_PROPERTY(bool flyWarp MEMBER flyWarp NOTIFY flyWarpChanged)                                ///< `BIT_FLY_WARP` -- arrive with the drop-in animation.
  Q_PROPERTY(bool dungeonWarp MEMBER dungeonWarp NOTIFY dungeonWarpChanged)                    ///< `BIT_DUNGEON_WARP` -- you fell down a hole.
  Q_PROPERTY(bool escapeWarp MEMBER escapeWarp NOTIFY escapeWarpChanged)                       ///< `BIT_ESCAPE_WARP` -- Dig / Escape Rope / blacked out. (Was `AreaMap::blackoutDest`, which it never was.)

  // ── Group E: where you came from -- 💀 DEAD. Written by the game, read by nothing. ─────
  Q_PROPERTY(int warpedFromWarp MEMBER warpedFromWarp NOTIFY warpedFromWarpChanged)            ///< `wWarpedFromWhichWarp`. 💀 Nothing reads it.
  Q_PROPERTY(int warpedfromMap MEMBER warpedfromMap NOTIFY warpedfromMapChanged)               ///< `wWarpedFromWhichMap`. 💀 Nothing reads it.

public:
  AreaWarps(SaveFile* saveFile = nullptr);
  virtual ~AreaWarps();

  void load(SaveFile* saveFile = nullptr); ///< Expand the warp list + state from the save.
  void save(SaveFile* saveFile);           ///< Flatten the warp list + state to the save.

  Q_INVOKABLE int warpCount();             ///< Number of warps.
  Q_INVOKABLE int warpMax();               ///< Capacity (maxWarps).
  Q_INVOKABLE WarpData* warpAt(int ind);   ///< Warp @p ind (GC-protected return).
  Q_INVOKABLE void warpSwap(int from, int to); ///< Reorder warps.
  Q_INVOKABLE void warpRemove(int ind);    ///< Remove warp @p ind.
  Q_INVOKABLE void warpNew();              ///< Add a fresh warp.

  // ── 🔫 The legal-value tables ────────────────────────────────────────────────────────
  //
  // `PrepareForSpecialWarp` (engine/overworld/special_warps.asm) looks both destinations up in
  // ROM tables that have **no bounds check** -- and `FlyWarpDataPtr` has no terminator at all.
  // A value outside them makes the console run off the end of the table, read whatever bytes
  // follow as a pointer, and copy six arbitrary ROM bytes into the view pointer + the player's
  // coordinates.
  //
  // We do not REFUSE an illegal value -- every byte the save can hold stays editable -- we
  // simply know which ones they are, so the screen can say so. @see notes/reference/warps.md §5.

  /// The **13** maps `wDestinationMap` may legally name (`FlyWarpDataPtr`, data/maps/special_warps.asm).
  static const QVector<int>& legalFlyMaps();

  /// Is @p map one of them?
  static bool isLegalFlyMap(int map);

  /// The **12** legal `(dungeonWarpDestMap, whichDungeonWarp)` pairs (`DungeonWarpList`).
  /// Hole numbers are **1-based** -- `IsPlayerOnDungeonWarp` writes `wCoordIndex`, which starts at 1.
  static const QVector<QPair<int, int>>& legalDungeonWarps();

  /// Is this pair one of them? (A legal map with the wrong hole number is just as broken as an
  /// illegal map -- Victory Road 2F has a hole 2 and no hole 1.)
  static bool isLegalDungeonWarp(int map, int which);

signals:
  void scriptedWarpChanged();
  void isDungeonWarpChanged();
  void forcedWarpChanged();
  void warpDestChanged();
  void dungeonWarpDestMapChanged();
  void specialWarpDestMapChanged();
  void flyOrDungeonWarpChanged();
  void flyWarpChanged();
  void dungeonWarpChanged();
  void escapeWarpChanged();
  void whichDungeonWarpChanged();
  void warpedFromWarpChanged();
  void warpedfromMapChanged();
  void warpsChanged();

public slots:
  void reset();                    ///< Empty warps + clear state.
  void randomize(MapDBEntry* map); ///< Randomize the warp list. Legal values only.
  void setTo(MapDBEntry* map);     ///< Rebuild the warp list from @p map. **Touches no state byte.**

public:
  // Pre-Warp -- ⚠️ wStatusFlags3. The console ZEROES this whole byte on every save load.
  bool scriptedWarp;  ///< `BIT_WARP_FROM_CUR_SCRIPT` -- "warp me now, no tile needed".
  bool isDungeonWarp; ///< `BIT_ON_DUNGEON_WARP` -- "standing on a hole" (suppresses wild battles).
  bool forcedWarp;    ///< `BIT_FORCED_WARP` -- a door fires the instant you touch it, without
                      ///  having to walk *into* it. How the Seafoam current sweeps you along.

  // Warping
  int warpDest;           ///< `wDestinationWarpID` -- which arrival point of the map you're entering.
                          ///  `0xFF` = "don't move the player" (every special warp sets this).
  int dungeonWarpDestMap; ///< `wDungeonWarpDestinationMap` -- the floor a hole drops you onto. 🔫
  int specialWarpDestMap; ///< `wDestinationMap` -- where Fly / the Hall of Fame / the Cable Club go. 🔫
  bool flyOrDungeonWarp;  ///< `BIT_FLY_OR_DUNGEON_WARP` -- a special warp is in progress.
  bool flyWarp;           ///< `BIT_FLY_WARP` -- arrive with the drop-in animation.
  bool dungeonWarp;       ///< `BIT_DUNGEON_WARP` -- you fell down a hole.
  bool escapeWarp;        ///< `BIT_ESCAPE_WARP` -- Dig / Escape Rope / blacked out.

  // Warped -- 💀 both DEAD. The game writes them on every warp and never reads them.
  int whichDungeonWarp; ///< `wWhichDungeonWarp` -- which hole you fell through. **1-based.** 🔫
  int warpedFromWarp;   ///< `wWarpedFromWhichWarp`. 💀
  int warpedfromMap;    ///< `wWarpedFromWhichMap`. 💀

  QVector<WarpData*> warps; ///< The map's doors.
};
