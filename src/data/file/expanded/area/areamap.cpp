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

void AreaMap::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  curMap = toolset->getByte(0x260A);
  height = toolset->getByte(0x2614);
  width = toolset->getByte(0x2615);
  dataPtr = toolset->getWord(0x2616, true);
  txtPtr = toolset->getWord(0x2618, true);
  scriptPtr = toolset->getWord(0x261A, true);
  height2x2 = toolset->getByte(0x27D0);
  width2x2 = toolset->getByte(0x27D1);

  outOfBoundsBlock = toolset->getByte(0x2659);

  currentTileBlockMapViewPointer = toolset->getWord(0x260B, true);

  // East Connection
  if(toolset->getBit(0x261C, 1, 0))
    connections.insert((var8)ConnectDir::EAST, new MapConnData(saveFile, 0x263E));

  // West Connection
  if(toolset->getBit(0x261C, 1, 1))
    connections.insert((var8)ConnectDir::WEST, new MapConnData(saveFile, 0x2633));

  // South Connection
  if(toolset->getBit(0x261C, 1, 2))
    connections.insert((var8)ConnectDir::SOUTH, new MapConnData(saveFile, 0x2628));

  // North Connection
  if(toolset->getBit(0x261C, 1, 3))
    connections.insert((var8)ConnectDir::NORTH, new MapConnData(saveFile, 0x261D));

  mapViewVRAMPointer = toolset->getWord(0x27D2, true);

  forceBikeRide = toolset->getBit(0x29DE, 1, 5);
  blackoutDest = toolset->getBit(0x29DE, 1, 6);
  curMapNextFrame = toolset->getBit(0x29DF, 1, 4);
  cardKeyDoorY = toolset->getByte(0x29EB);
  cardKeyDoorX = toolset->getByte(0x29EC);
  curMapScript = toolset->getByte(0x2CE5);
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

  // Map Size including it's double size
  height = 0;
  width = 0;
  height2x2 = 0;
  width2x2 = 0;

  // Map basic pointers
  dataPtr = 0;
  txtPtr = 0;
  scriptPtr = 0;

  // Map extra pointers
  currentTileBlockMapViewPointer = 0; // <- Player coords converted to a ptr
  mapViewVRAMPointer = 0; // <- Unused, reset at start of gameplay

  // Current map script index
  curMapScript = 0;

  // Unknown ???
  cardKeyDoorX = 0;
  cardKeyDoorY = 0;

  // Flags that may not be used, unknown
  forceBikeRide = 0;
  blackoutDest = 0;
  curMapNextFrame = 0;

  outOfBoundsBlock = 0;

  for(auto conn : connections)
    delete conn;

  connections.clear();
}

void AreaMap::randomize(MapDBEntry* map, var8 x, var8 y)
{
  reset();

  // Current Map ID
  curMap = map->ind;

  // Map Size including it's double size
  height = *map->height;
  width = *map->width;
  height2x2 = *map->height2X2();
  width2x2 = *map->width2X2();

  // Map basic pointers
  dataPtr = *map->dataPtr;
  txtPtr = *map->textPtr;
  scriptPtr = *map->scriptPtr;

  // Map extra pointers
  currentTileBlockMapViewPointer = coordsToPtr(x, y, *map->width);
  mapViewVRAMPointer = VramBGPtr;

  if(map->border)
    outOfBoundsBlock = *map->border;

  // Leave these off for now

  // Current map script index
  curMapScript = 0;

  // Unknown ???
  cardKeyDoorX = 0;
  cardKeyDoorY = 0;

  // Flags that may not be used, unknown
  forceBikeRide = 0;
  blackoutDest = 0;
  curMapNextFrame = 0;

  if(map->connect.contains((var8)ConnectDir::NORTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::NORTH));
    connections.insert((var8)ConnectDir::NORTH, conn);
  }
  if(map->connect.contains((var8)ConnectDir::EAST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::EAST));
    connections.insert((var8)ConnectDir::EAST, conn);
  }
  if(map->connect.contains((var8)ConnectDir::SOUTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::SOUTH));
    connections.insert((var8)ConnectDir::SOUTH, conn);
  }
  if(map->connect.contains((var8)ConnectDir::WEST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->connect.value((var8)ConnectDir::WEST));
    connections.insert((var8)ConnectDir::WEST, conn);
  }
}

var16 AreaMap::coordsToPtr(var8 x, var8 y, var8 width)
{
  return (width+7)+x+(y*(width+6)) + worldMapPtr;
}
