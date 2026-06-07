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
#include <QHash>
#include <QVector>

#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapConnData;
struct MapDBEntry;

// This value never changes, it is the in-memory value in the GB that gen 1
// games use to store the background tilemap for the world and maps.
constexpr var16 VramBGPtr = 0x9800; ///< Fixed GB VRAM background-tilemap pointer (never changes).

/**
 * @brief Identity, size, pointers, and edge connections of the current map.
 *
 * The structural core of an Area: which map you're on, its dimensions, the various
 * data/text/script pointers the game tracks, and the set of @ref connections to
 * neighbouring maps. Many fields are raw GB memory pointers mirrored from the save;
 * the field comments below describe each. setTo()/randomize() reconfigure the map
 * (with the player at x,y) as part of the "place on any map" feature.
 *
 * @see Area (container), MapConnData (a connection), MapDBEntry (map definitions).
 */
class SAVEFILE_AUTOPORT AreaMap : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int curMap MEMBER curMap NOTIFY curMapChanged)                       ///< Current map id.
  Q_PROPERTY(int outOfBoundsBlock MEMBER outOfBoundsBlock NOTIFY outOfBoundsBlockChanged) ///< The map's invalid/edge block.
  Q_PROPERTY(int height MEMBER height NOTIFY heightChanged)                        ///< Map height (blocks).
  Q_PROPERTY(int width MEMBER width NOTIFY widthChanged)                           ///< Map width (blocks).
  Q_PROPERTY(int height2x2 MEMBER height2x2 NOTIFY height2x2Changed)               ///< Map height in 2x2 units.
  Q_PROPERTY(int width2x2 MEMBER width2x2 NOTIFY width2x2Changed)                  ///< Map width in 2x2 units.
  Q_PROPERTY(int dataPtr MEMBER dataPtr NOTIFY dataPtrChanged)                     ///< Map data pointer.
  Q_PROPERTY(int txtPtr MEMBER txtPtr NOTIFY txtPtrChanged)                        ///< Map text pointer.
  Q_PROPERTY(int scriptPtr MEMBER scriptPtr NOTIFY scriptPtrChanged)              ///< Map script pointer.
  Q_PROPERTY(int currentTileBlockMapViewPointer MEMBER currentTileBlockMapViewPointer NOTIFY currentTileBlockMapViewPointerChanged) ///< Player coords as a pointer.
  Q_PROPERTY(int mapViewVRAMPointer MEMBER mapViewVRAMPointer NOTIFY mapViewVRAMPointerChanged) ///< Unused; reset at gameplay start.
  Q_PROPERTY(int curMapScript MEMBER curMapScript NOTIFY curMapScriptChanged)      ///< Current map script index.
  Q_PROPERTY(int cardKeyDoorX MEMBER cardKeyDoorX NOTIFY cardKeyDoorXChanged)      ///< Card-key door X (purpose unclear).
  Q_PROPERTY(int cardKeyDoorY MEMBER cardKeyDoorY NOTIFY cardKeyDoorYChanged)      ///< Card-key door Y (purpose unclear).
  Q_PROPERTY(bool forceBikeRide MEMBER forceBikeRide NOTIFY forceBikeRideChanged)  ///< Flag (may be unused).
  Q_PROPERTY(bool blackoutDest MEMBER blackoutDest NOTIFY blackoutDestChanged)     ///< Flag (may be unused).
  Q_PROPERTY(bool curMapNextFrame MEMBER curMapNextFrame NOTIFY curMapNextFrameChanged) ///< Flag (may be unused).

public:
  AreaMap(SaveFile* saveFile = nullptr);
  virtual ~AreaMap();

  void load(SaveFile* saveFile = nullptr); ///< Expand the map block from the save.
  void save(SaveFile* saveFile);           ///< Flatten the map block to the save.

  /// Converts X & Y values to a pointer for currentTileBlockMapViewPointer.
  int coordsToPtr(int x, int y, int width);

  Q_INVOKABLE int connCount();             ///< Number of edge connections.
  Q_INVOKABLE MapConnData* connAt(int dir); ///< Connection in direction @p dir (GC-protected return).
  Q_INVOKABLE void connRemove(int dir);    ///< Remove the connection in @p dir.
  Q_INVOKABLE void connNew(int dir);       ///< Add a connection in @p dir.

  MapDBEntry* toCurMap(); ///< The current map's DB entry.

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
  void reset();                              ///< Blank the map block.
  void randomize(MapDBEntry* map, int x, int y); ///< Randomize to @p map with the player at (x,y).
  void setTo(MapDBEntry* map, int x, int y);     ///< Configure to @p map with the player at (x,y).

public:
  /// Current Map ID
  int curMap;

  /// This is not a tile, it's a block. A block consists of multiple tiles
  /// Maps are made from pre-created blocks, not individual tiles
  /// Every map has a block that's invalid, this is that block
  int outOfBoundsBlock;

  /// Map Size including it's double size
  int height;
  int width;
  int height2x2;
  int width2x2;

  /// Map basic pointers
  int dataPtr;
  int txtPtr;
  int scriptPtr;

  // Map extra pointers
  int currentTileBlockMapViewPointer; ///< <- Player coords converted to a ptr
  int mapViewVRAMPointer; ///< <- Unused, reset at start of gameplay

  /// Current map script index
  int curMapScript;

  /// Unknown ???
  int cardKeyDoorX;
  int cardKeyDoorY;

  /// Flags that may not be used, unknown
  bool forceBikeRide;
  bool blackoutDest;
  bool curMapNextFrame;

  // Map Connections
  // So here's the thing, QHash technically can be a Q_PROPERTY but because of
  // the template comma it freaks the IDE out and throws a ton of errors. It
  // compiles just fine, but I can't handle all the red errors the IDE gives
  QHash<var8, MapConnData*> connections; ///< Edge connections by direction (see the IDE note above).
};
