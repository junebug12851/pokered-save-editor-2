/*
  * Copyright 2020 June Hanabi
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
#ifndef AREAWARPS_H
#define AREAWARPS_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"

class SaveFile;
class WarpData;
class MapDBEntry;

class AreaWarps : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool scriptedWarp_ MEMBER scriptedWarp NOTIFY scriptedWarpChanged)
  Q_PROPERTY(bool isDungeonWarp_ MEMBER isDungeonWarp NOTIFY isDungeonWarpChanged)
  Q_PROPERTY(bool skipJoypadCheckWarps_ MEMBER skipJoypadCheckWarps NOTIFY skipJoypadCheckWarpsChanged)
  Q_PROPERTY(var8 warpDest_ MEMBER warpDest NOTIFY warpDestChanged)
  Q_PROPERTY(var8 dungeonWarpDestMap_ MEMBER dungeonWarpDestMap NOTIFY dungeonWarpDestMapChanged)
  Q_PROPERTY(var8 specialWarpDestMap_ MEMBER specialWarpDestMap NOTIFY specialWarpDestMapChanged)
  Q_PROPERTY(bool flyOrDungeonWarp_ MEMBER flyOrDungeonWarp NOTIFY flyOrDungeonWarpChanged)
  Q_PROPERTY(bool flyWarp_ MEMBER flyWarp NOTIFY flyWarpChanged)
  Q_PROPERTY(bool dungeonWarp_ MEMBER dungeonWarp NOTIFY dungeonWarpChanged)
  Q_PROPERTY(var8 whichDungeonWarp_ MEMBER whichDungeonWarp NOTIFY whichDungeonWarpChanged)
  Q_PROPERTY(var8 warpedFromWarp_ MEMBER warpedFromWarp NOTIFY warpedFromWarpChanged)
  Q_PROPERTY(var8 warpedfromMap_ MEMBER warpedfromMap NOTIFY warpedfromMapChanged)
  Q_PROPERTY(QVector<WarpData*> warps_ MEMBER warps NOTIFY warpsChanged)

public:
  AreaWarps(SaveFile* saveFile = nullptr);
  virtual ~AreaWarps();

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
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(MapDBEntry* map);

public:
  // Pre-Warp
  bool scriptedWarp; // Do a scripted warp
  bool isDungeonWarp; // On a dungeon warp
  bool skipJoypadCheckWarps; // Skips check for warp after not collided (Forced Warp)??

  // Warping
  var8 warpDest; // Warp actively warping to or 0xFF to warp to same position
  var8 dungeonWarpDestMap; // Destination Map for dungeon warps
  var8 specialWarpDestMap; // Destination Map for special warps
  bool flyOrDungeonWarp; // Is a fly or dungeon warp
  bool flyWarp; // Is a fly warp
  bool dungeonWarp; // Is a dungeon warp

  // Warped
  var8 whichDungeonWarp; // Warped from which dungeon warp
  var8 warpedFromWarp; // Warped from which warp
  var8 warpedfromMap; // Warped from which map

  QVector<WarpData*> warps;
};

#endif // AREAWARPS_H
