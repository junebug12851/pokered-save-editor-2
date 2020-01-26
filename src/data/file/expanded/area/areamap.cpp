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
#include "./areamap.h"
#include "../fragments/mapconndata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"

#include <QRandomGenerator>

AreaMap::AreaMap(SaveFile* saveFile)
{
  load(saveFile);
}

AreaMap::~AreaMap()
{
  reset();
}

int AreaMap::connCount()
{
  return connections.size();
}

MapConnData* AreaMap::connAt(int dir)
{
  return connections.value(dir);
}

void AreaMap::connRemove(int dir)
{
  if(!connections.contains(dir))
    return;

  delete connections.value(dir);
  connections.remove(dir);
  connectionsChanged();
}

void AreaMap::connNew(int dir)
{
  connRemove(dir);
  connections.insert(dir, new MapConnData);
  connectionsChanged();
}

void AreaMap::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  curMap = toolset->getByte(0x260A);
  curMapChanged();

  height = toolset->getByte(0x2614);
  heightChanged();

  width = toolset->getByte(0x2615);
  widthChanged();

  dataPtr = toolset->getWord(0x2616, true);
  dataPtrChanged();

  txtPtr = toolset->getWord(0x2618, true);
  txtPtrChanged();

  scriptPtr = toolset->getWord(0x261A, true);
  scriptPtrChanged();

  height2x2 = toolset->getByte(0x27D0);
  height2x2Changed();

  width2x2 = toolset->getByte(0x27D1);
  width2x2Changed();

  outOfBoundsBlock = toolset->getByte(0x2659);
  outOfBoundsBlockChanged();

  currentTileBlockMapViewPointer = toolset->getWord(0x260B, true);
  currentTileBlockMapViewPointerChanged();

  // East Connection
  if(toolset->getBit(0x261C, 1, 0)) {
    connections.insert((var8)ConnectDir::EAST, new MapConnData(saveFile, 0x263E));
    connectionsChanged();
  }

  // West Connection
  if(toolset->getBit(0x261C, 1, 1)) {
    connections.insert((var8)ConnectDir::WEST, new MapConnData(saveFile, 0x2633));
    connectionsChanged();
  }

  // South Connection
  if(toolset->getBit(0x261C, 1, 2)) {
    connections.insert((var8)ConnectDir::SOUTH, new MapConnData(saveFile, 0x2628));
    connectionsChanged();
  }

  // North Connection
  if(toolset->getBit(0x261C, 1, 3)) {
    connections.insert((var8)ConnectDir::NORTH, new MapConnData(saveFile, 0x261D));
    connectionsChanged();
  }

  mapViewVRAMPointer = toolset->getWord(0x27D2, true);
  mapViewVRAMPointerChanged();

  forceBikeRide = toolset->getBit(0x29DE, 1, 5);
  forceBikeRideChanged();

  blackoutDest = toolset->getBit(0x29DE, 1, 6);
  blackoutDestChanged();

  curMapNextFrame = toolset->getBit(0x29DF, 1, 4);
  curMapNextFrameChanged();

  cardKeyDoorY = toolset->getByte(0x29EB);
  cardKeyDoorYChanged();

  cardKeyDoorX = toolset->getByte(0x29EC);
  cardKeyDoorXChanged();

  curMapScript = toolset->getByte(0x2CE5);
  curMapScriptChanged();
}

void AreaMap::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x260A, curMap);
  toolset->setByte(0x2614, height);
  toolset->setByte(0x2615, width);
  toolset->setWord(0x2616, dataPtr, true);
  toolset->setWord(0x2618, txtPtr, true);
  toolset->setWord(0x261A, scriptPtr, true);
  toolset->setByte(0x27D0, height2x2);
  toolset->setByte(0x27D1, width2x2);

  toolset->setWord(0x260B, currentTileBlockMapViewPointer, true);

  toolset->setBit(0x261C, 1, 0, connections.contains((var8)ConnectDir::EAST));
  toolset->setBit(0x261C, 1, 1, connections.contains((var8)ConnectDir::WEST));
  toolset->setBit(0x261C, 1, 2, connections.contains((var8)ConnectDir::SOUTH));
  toolset->setBit(0x261C, 1, 3, connections.contains((var8)ConnectDir::NORTH));

  if(connections.contains((var8)ConnectDir::EAST))
    connections.value((var8)ConnectDir::EAST)->save(saveFile, 0x263E);
  if(connections.contains((var8)ConnectDir::WEST))
    connections.value((var8)ConnectDir::WEST)->save(saveFile, 0x2633);
  if(connections.contains((var8)ConnectDir::SOUTH))
    connections.value((var8)ConnectDir::SOUTH)->save(saveFile, 0x2628);
  if(connections.contains((var8)ConnectDir::NORTH))
    connections.value((var8)ConnectDir::NORTH)->save(saveFile, 0x261D);

  toolset->setWord(0x27D2, mapViewVRAMPointer, true);

  toolset->setBit(0x29DE, 1, 5, forceBikeRide);
  toolset->setBit(0x29DE, 1, 6, blackoutDest);
  toolset->setBit(0x29DF, 1, 4, curMapNextFrame);
  toolset->setByte(0x29EB, cardKeyDoorY);
  toolset->setByte(0x29EC, cardKeyDoorX);
  toolset->setByte(0x2CE5, curMapScript);

  toolset->setByte(0x2659, outOfBoundsBlock);
}

void AreaMap::reset()
{
  // Current Map ID
  curMap = 0;
  curMapChanged();

  // Map Size including it's double size
  height = 0;
  heightChanged();

  width = 0;
  widthChanged();

  height2x2 = 0;
  height2x2Changed();

  width2x2 = 0;
  width2x2Changed();

  // Map basic pointers
  dataPtr = 0;
  dataPtrChanged();

  txtPtr = 0;
  txtPtrChanged();

  scriptPtr = 0;
  scriptPtrChanged();

  // Map extra pointers
  currentTileBlockMapViewPointer = 0; // <- Player coords converted to a ptr
  currentTileBlockMapViewPointerChanged();

  mapViewVRAMPointer = 0; // <- Unused, reset at start of gameplay
  mapViewVRAMPointerChanged();

  // Current map script index
  curMapScript = 0;
  curMapScriptChanged();

  // Unknown ???
  cardKeyDoorX = 0;
  cardKeyDoorXChanged();

  cardKeyDoorY = 0;
  cardKeyDoorYChanged();

  // Flags that may not be used, unknown
  forceBikeRide = 0;
  forceBikeRideChanged();

  blackoutDest = 0;
  blackoutDestChanged();

  curMapNextFrame = 0;
  curMapNextFrameChanged();

  outOfBoundsBlock = 0;
  outOfBoundsBlockChanged();

  for(auto conn : connections)
    delete conn;

  connections.clear();
  connectionsChanged();
}

void AreaMap::randomize(MapDBEntry* map, int x, int y)
{
  reset();

  // Current Map ID
  curMap = map->ind;
  curMapChanged();

  // Map Size including it's double size
  height = *map->height;
  heightChanged();

  width = *map->width;
  widthChanged();

  height2x2 = *map->height2X2();
  height2x2Changed();

  width2x2 = *map->width2X2();
  width2x2Changed();

  // Map basic pointers
  dataPtr = *map->dataPtr;
  dataPtrChanged();

  txtPtr = *map->textPtr;
  txtPtrChanged();

  scriptPtr = *map->scriptPtr;
  scriptPtrChanged();

  // Map extra pointers
  currentTileBlockMapViewPointer = coordsToPtr(x, y, *map->width);
  currentTileBlockMapViewPointerChanged();

  mapViewVRAMPointer = VramBGPtr;
  mapViewVRAMPointerChanged();

  if(map->border) {
    outOfBoundsBlock = *map->border;
    outOfBoundsBlockChanged();
  }

  // Leave these off for now

  if(map->connect.contains((var8)ConnectDir::NORTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::NORTH));
    connections.insert((var8)ConnectDir::NORTH, conn);
    connectionsChanged();
  }
  if(map->connect.contains((var8)ConnectDir::EAST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::EAST));
    connections.insert((var8)ConnectDir::EAST, conn);
    connectionsChanged();
  }
  if(map->connect.contains((var8)ConnectDir::SOUTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::SOUTH));
    connections.insert((var8)ConnectDir::SOUTH, conn);
    connectionsChanged();
  }
  if(map->connect.contains((var8)ConnectDir::WEST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::WEST));
    connections.insert((var8)ConnectDir::WEST, conn);
    connectionsChanged();
  }
}

int AreaMap::coordsToPtr(int x, int y, int width)
{
  return (width+7)+x+(y*(width+6)) + worldMapPtr;
}
