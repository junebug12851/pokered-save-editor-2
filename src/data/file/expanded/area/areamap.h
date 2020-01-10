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
#ifndef AREAMAP_H
#define AREAMAP_H

#include <QHash>
#include <QVector>

#include "../expandedinterface.h"
#include "../../../../common/types.h"

class MapConnData;
struct MapDBEntry;

// This value never changes, it is the in-memory value in the GB that gen 1
// games use to store the background tilemap for the world and maps.
constexpr var16 VramBGPtr = 0x9800;

class AreaMap : ExpandedInterface
{
public:
  AreaMap(SaveFile* saveFile = nullptr);
  virtual ~AreaMap();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();

  // You have to provide it a non-glitch map or it will crash, it expects common
  // data to be there like width, height, etc... which are often not in glitch
  // or incompelte maps
  void randomize(MapDBEntry* map, var8 x, var8 y);

  // Converts X & Y values to a pointer for currentTileBlockMapViewPointer
  var16 coordsToPtr(var8 x, var8 y, var8 width);

  // Current Map ID
  var8 curMap;

  // Map Size including it's double size
  var8 height;
  var8 width;
  var8 height2x2;
  var8 width2x2;

  // Map basic pointers
  var16 dataPtr;
  var16 txtPtr;
  var16 scriptPtr;

  // Map extra pointers
  var16 currentTileBlockMapViewPointer; // <- Player coords converted to a ptr
  var16 mapViewVRAMPointer; // <- Unused, reset at start of gameplay

  // Current map script index
  var8 curMapScript;

  // Unknown ???
  var8 cardKeyDoorX;
  var8 cardKeyDoorY;

  // Flags that may not be used, unknown
  bool forceBikeRide;
  bool blackoutDest;
  bool curMapNextFrame;

  // Map Connections
  QHash<var8, MapConnData*> connections;

private:
  void randomize();
};

#endif // AREAMAP_H
