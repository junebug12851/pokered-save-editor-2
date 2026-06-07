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

constexpr var8 maxWarps = 32; ///< Maximum warps on a map.

/**
 * @brief The current map's warps, plus the live warp-transition state.
 *
 * Two things in one: the list of WarpData warp points (up to @ref maxWarps, with
 * QML add/remove/swap/access), and a set of transient flags describing an
 * in-progress warp -- whether it's scripted/dungeon/fly, the destinations, and
 * where the player warped @e from. The field comments below give each flag's
 * meaning. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see WarpData (a warp point), Area, MapDBEntry.
 */
class SAVEFILE_AUTOPORT AreaWarps : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool scriptedWarp MEMBER scriptedWarp NOTIFY scriptedWarpChanged)                 ///< Do a scripted warp.
  Q_PROPERTY(bool isDungeonWarp MEMBER isDungeonWarp NOTIFY isDungeonWarpChanged)              ///< On a dungeon warp.
  Q_PROPERTY(bool skipJoypadCheckWarps MEMBER skipJoypadCheckWarps NOTIFY skipJoypadCheckWarpsChanged) ///< Forced-warp joypad skip.
  Q_PROPERTY(int warpDest MEMBER warpDest NOTIFY warpDestChanged)                              ///< Active warp destination (0xFF = same position).
  Q_PROPERTY(int dungeonWarpDestMap MEMBER dungeonWarpDestMap NOTIFY dungeonWarpDestMapChanged) ///< Dungeon-warp destination map.
  Q_PROPERTY(int specialWarpDestMap MEMBER specialWarpDestMap NOTIFY specialWarpDestMapChanged) ///< Special-warp destination map.
  Q_PROPERTY(bool flyOrDungeonWarp MEMBER flyOrDungeonWarp NOTIFY flyOrDungeonWarpChanged)     ///< Is a fly or dungeon warp.
  Q_PROPERTY(bool flyWarp MEMBER flyWarp NOTIFY flyWarpChanged)                                ///< Is a fly warp.
  Q_PROPERTY(bool dungeonWarp MEMBER dungeonWarp NOTIFY dungeonWarpChanged)                    ///< Is a dungeon warp.
  Q_PROPERTY(int whichDungeonWarp MEMBER whichDungeonWarp NOTIFY whichDungeonWarpChanged)      ///< Warped from which dungeon warp.
  Q_PROPERTY(int warpedFromWarp MEMBER warpedFromWarp NOTIFY warpedFromWarpChanged)            ///< Warped from which warp.
  Q_PROPERTY(int warpedfromMap MEMBER warpedfromMap NOTIFY warpedfromMapChanged)               ///< Warped from which map.

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

signals:
  void scriptedWarpChanged();
  void isDungeonWarpChanged();
  void skipJoypadCheckWarpsChanged();
  void warpDestChanged();
  void dungeonWarpDestMapChanged();
  void specialWarpDestMapChanged();
  void flyOrDungeonWarpChanged();
  void flyWarpChanged();
  void dungeonWarpChanged();
  void whichDungeonWarpChanged();
  void warpedFromWarpChanged();
  void warpedfromMapChanged();
  void warpsChanged();

public slots:
  void reset();                ///< Empty warps + clear state.
  void randomize(MapDBEntry* map); ///< Randomize warps for @p map.
  void setTo(MapDBEntry* map);     ///< Rebuild warps from @p map.

public:
  // Pre-Warp
  bool scriptedWarp; ///< Do a scripted warp
  bool isDungeonWarp; ///< On a dungeon warp
  bool skipJoypadCheckWarps; ///< Skips check for warp after not collided (Forced Warp)??

  // Warping
  int warpDest; ///< Warp actively warping to or 0xFF to warp to same position
  int dungeonWarpDestMap; ///< Destination Map for dungeon warps
  int specialWarpDestMap; ///< Destination Map for special warps
  bool flyOrDungeonWarp; ///< Is a fly or dungeon warp
  bool flyWarp; ///< Is a fly warp
  bool dungeonWarp; ///< Is a dungeon warp

  // Warped
  int whichDungeonWarp; ///< Warped from which dungeon warp
  int warpedFromWarp; ///< Warped from which warp
  int warpedfromMap; ///< Warped from which map

  QVector<WarpData*> warps; ///< The map's warp points.
};
