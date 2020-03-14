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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class WarpData;
class MapDBEntry;

constexpr var8 maxWarps = 32;

class SAVEFILE_AUTOPORT AreaWarps : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool scriptedWarp MEMBER scriptedWarp NOTIFY scriptedWarpChanged)
  Q_PROPERTY(bool isDungeonWarp MEMBER isDungeonWarp NOTIFY isDungeonWarpChanged)
  Q_PROPERTY(bool skipJoypadCheckWarps MEMBER skipJoypadCheckWarps NOTIFY skipJoypadCheckWarpsChanged)
  Q_PROPERTY(int warpDest MEMBER warpDest NOTIFY warpDestChanged)
  Q_PROPERTY(int dungeonWarpDestMap MEMBER dungeonWarpDestMap NOTIFY dungeonWarpDestMapChanged)
  Q_PROPERTY(int specialWarpDestMap MEMBER specialWarpDestMap NOTIFY specialWarpDestMapChanged)
  Q_PROPERTY(bool flyOrDungeonWarp MEMBER flyOrDungeonWarp NOTIFY flyOrDungeonWarpChanged)
  Q_PROPERTY(bool flyWarp MEMBER flyWarp NOTIFY flyWarpChanged)
  Q_PROPERTY(bool dungeonWarp MEMBER dungeonWarp NOTIFY dungeonWarpChanged)
  Q_PROPERTY(int whichDungeonWarp MEMBER whichDungeonWarp NOTIFY whichDungeonWarpChanged)
  Q_PROPERTY(int warpedFromWarp MEMBER warpedFromWarp NOTIFY warpedFromWarpChanged)
  Q_PROPERTY(int warpedfromMap MEMBER warpedfromMap NOTIFY warpedfromMapChanged)

public:
  AreaWarps(SaveFile* saveFile = nullptr);
  virtual ~AreaWarps();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int warpCount();
  Q_INVOKABLE int warpMax();
  Q_INVOKABLE WarpData* warpAt(int ind);
  Q_INVOKABLE void warpSwap(int from, int to);
  Q_INVOKABLE void warpRemove(int ind);
  Q_INVOKABLE void warpNew();

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
  void reset();
  void randomize(MapDBEntry* map);
  void setTo(MapDBEntry* map);

public:
  // Pre-Warp
  bool scriptedWarp; // Do a scripted warp
  bool isDungeonWarp; // On a dungeon warp
  bool skipJoypadCheckWarps; // Skips check for warp after not collided (Forced Warp)??

  // Warping
  int warpDest; // Warp actively warping to or 0xFF to warp to same position
  int dungeonWarpDestMap; // Destination Map for dungeon warps
  int specialWarpDestMap; // Destination Map for special warps
  bool flyOrDungeonWarp; // Is a fly or dungeon warp
  bool flyWarp; // Is a fly warp
  bool dungeonWarp; // Is a dungeon warp

  // Warped
  int whichDungeonWarp; // Warped from which dungeon warp
  int warpedFromWarp; // Warped from which warp
  int warpedfromMap; // Warped from which map

  QVector<WarpData*> warps;
};

#endif // AREAWARPS_H
