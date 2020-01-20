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

#include <QObject>
#include <QHash>
#include <QVector>

#include "../../../../common/types.h"

class SaveFile;
class MapConnData;
struct MapDBEntry;

// This value never changes, it is the in-memory value in the GB that gen 1
// games use to store the background tilemap for the world and maps.
constexpr var16 VramBGPtr = 0x9800;

class AreaMap : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 curMap_ MEMBER curMap NOTIFY curMapChanged)
  Q_PROPERTY(var8 outOfBoundsBlock_ MEMBER outOfBoundsBlock NOTIFY outOfBoundsBlockChanged)
  Q_PROPERTY(var8 height_ MEMBER height NOTIFY heightChanged)
  Q_PROPERTY(var8 width_ MEMBER width NOTIFY widthChanged)
  Q_PROPERTY(var8 height2x2_ MEMBER height2x2 NOTIFY height2x2Changed)
  Q_PROPERTY(var8 width2x2_ MEMBER width2x2 NOTIFY width2x2Changed)
  Q_PROPERTY(var16 dataPtr_ MEMBER dataPtr NOTIFY dataPtrChanged)
  Q_PROPERTY(var16 txtPtr_ MEMBER txtPtr NOTIFY txtPtrChanged)
  Q_PROPERTY(var16 scriptPtr_ MEMBER scriptPtr NOTIFY scriptPtrChanged)
  Q_PROPERTY(var16 currentTileBlockMapViewPointer_ MEMBER currentTileBlockMapViewPointer NOTIFY currentTileBlockMapViewPointerChanged)
  Q_PROPERTY(var16 mapViewVRAMPointer_ MEMBER mapViewVRAMPointer NOTIFY mapViewVRAMPointerChanged)
  Q_PROPERTY(var8 curMapScript_ MEMBER curMapScript NOTIFY curMapScriptChanged)
  Q_PROPERTY(var8 cardKeyDoorX_ MEMBER cardKeyDoorX NOTIFY cardKeyDoorXChanged)
  Q_PROPERTY(var8 cardKeyDoorY_ MEMBER cardKeyDoorY NOTIFY cardKeyDoorYChanged)
  Q_PROPERTY(bool forceBikeRide_ MEMBER forceBikeRide NOTIFY forceBikeRideChanged)
  Q_PROPERTY(bool blackoutDest_ MEMBER blackoutDest NOTIFY blackoutDestChanged)
  Q_PROPERTY(bool curMapNextFrame_ MEMBER curMapNextFrame NOTIFY curMapNextFrameChanged)

public:
  AreaMap(SaveFile* saveFile = nullptr);
  virtual ~AreaMap();

signals:
  void curMapChanged();
  void outOfBoundsBlockChanged();
  void heightChanged();
  void widthChanged();
  void height2x2Changed();
  void width2x2Changed();
  void dataPtrChanged();
  void txtPtrChanged();
  void scriptPtrChanged();
  void currentTileBlockMapViewPointerChanged();
  void mapViewVRAMPointerChanged();
  void curMapScriptChanged();
  void cardKeyDoorXChanged();
  void cardKeyDoorYChanged();
  void forceBikeRideChanged();
  void blackoutDestChanged();
  void curMapNextFrameChanged();
  void connectionsChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();

  // You have to provide it a non-glitch map or it will crash, it expects common
  // data to be there like width, height, etc... which are often not in glitch
  // or incompelte maps
  void randomize(MapDBEntry* map, var8 x, var8 y);

  // Converts X & Y values to a pointer for currentTileBlockMapViewPointer
  var16 coordsToPtr(var8 x, var8 y, var8 width);

public:
  // Current Map ID
  var8 curMap;

  // This is not a tile, it's a block. A block consists of multiple tiles
  // Maps are made from pre-created blocks, not individual tiles
  // Every map has a block that's invalid, this is that block
  var8 outOfBoundsBlock;

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
  // So here's the thing, QHash technically can be a Q_PROPERTY but because of
  // the template comma it freaks the IDE out and throws a ton of errors. It
  // compiles just fine, but I can't handle all the red errors the IDE gives
  QHash<var8, MapConnData*> connections;
};

#endif // AREAMAP_H
