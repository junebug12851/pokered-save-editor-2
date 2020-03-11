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

  Q_PROPERTY(int curMap MEMBER curMap NOTIFY curMapChanged)
  Q_PROPERTY(int outOfBoundsBlock MEMBER outOfBoundsBlock NOTIFY outOfBoundsBlockChanged)
  Q_PROPERTY(int height MEMBER height NOTIFY heightChanged)
  Q_PROPERTY(int width MEMBER width NOTIFY widthChanged)
  Q_PROPERTY(int height2x2 MEMBER height2x2 NOTIFY height2x2Changed)
  Q_PROPERTY(int width2x2 MEMBER width2x2 NOTIFY width2x2Changed)
  Q_PROPERTY(int dataPtr MEMBER dataPtr NOTIFY dataPtrChanged)
  Q_PROPERTY(int txtPtr MEMBER txtPtr NOTIFY txtPtrChanged)
  Q_PROPERTY(int scriptPtr MEMBER scriptPtr NOTIFY scriptPtrChanged)
  Q_PROPERTY(int currentTileBlockMapViewPointer MEMBER currentTileBlockMapViewPointer NOTIFY currentTileBlockMapViewPointerChanged)
  Q_PROPERTY(int mapViewVRAMPointer MEMBER mapViewVRAMPointer NOTIFY mapViewVRAMPointerChanged)
  Q_PROPERTY(int curMapScript MEMBER curMapScript NOTIFY curMapScriptChanged)
  Q_PROPERTY(int cardKeyDoorX MEMBER cardKeyDoorX NOTIFY cardKeyDoorXChanged)
  Q_PROPERTY(int cardKeyDoorY MEMBER cardKeyDoorY NOTIFY cardKeyDoorYChanged)
  Q_PROPERTY(bool forceBikeRide MEMBER forceBikeRide NOTIFY forceBikeRideChanged)
  Q_PROPERTY(bool blackoutDest MEMBER blackoutDest NOTIFY blackoutDestChanged)
  Q_PROPERTY(bool curMapNextFrame MEMBER curMapNextFrame NOTIFY curMapNextFrameChanged)

public:
  AreaMap(SaveFile* saveFile = nullptr);
  virtual ~AreaMap();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  // You have to provide it a non-glitch map or it will crash, it expects common
  // data to be there like width, height, etc... which are often not in glitch
  // or incompelte maps
  void randomize(MapDBEntry* map, int x, int y);

  // Converts X & Y values to a pointer for currentTileBlockMapViewPointer
  int coordsToPtr(int x, int y, int width);

  Q_INVOKABLE int connCount();
  Q_INVOKABLE MapConnData* connAt(int dir);
  Q_INVOKABLE void connRemove(int dir);
  Q_INVOKABLE void connNew(int dir);

  MapDBEntry* toCurMap();

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
  void reset();

public:
  // Current Map ID
  int curMap;

  // This is not a tile, it's a block. A block consists of multiple tiles
  // Maps are made from pre-created blocks, not individual tiles
  // Every map has a block that's invalid, this is that block
  int outOfBoundsBlock;

  // Map Size including it's double size
  int height;
  int width;
  int height2x2;
  int width2x2;

  // Map basic pointers
  int dataPtr;
  int txtPtr;
  int scriptPtr;

  // Map extra pointers
  int currentTileBlockMapViewPointer; // <- Player coords converted to a ptr
  int mapViewVRAMPointer; // <- Unused, reset at start of gameplay

  // Current map script index
  int curMapScript;

  // Unknown ???
  int cardKeyDoorX;
  int cardKeyDoorY;

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
