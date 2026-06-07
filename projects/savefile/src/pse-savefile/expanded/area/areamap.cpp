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

/**
 * @file areamap.cpp
 * @brief Implementation of AreaMap -- map id/size/pointers and edge connections.
 *        See areamap.h for the documented API.
 */
#include "./areamap.h"
#include "../../qmlownership.h"
#include "../fragments/mapconndata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentryconnect.h>

#include <QRandomGenerator>

AreaMap::AreaMap(SaveFile* saveFile)
{
  load(saveFile);
}

AreaMap::~AreaMap()
{
  for(auto conn : connections)
    conn->deleteLater();
}

int AreaMap::connCount()
{
  return connections.size();
}

MapConnData* AreaMap::connAt(int dir)
{
  return qmlCppOwned(connections.value(dir));
}

void AreaMap::connRemove(int dir)
{
  if(!connections.contains(dir))
    return;

  connections.value(dir)->deleteLater();
  connections.remove(dir);
  connectionsChanged();
}

void AreaMap::connNew(int dir)
{
  connRemove(dir);
  connections.insert(dir, new MapConnData);
  connectionsChanged();
}

MapDBEntry* AreaMap::toCurMap()
{
  return MapsDB::inst()->getIndAt(QString::number(curMap));
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
    connections.insert((var8)MapDBEntryConnect::ConnectDir::EAST, new MapConnData(saveFile, 0x263E));
    connectionsChanged();
  }

  // West Connection
  if(toolset->getBit(0x261C, 1, 1)) {
    connections.insert((var8)MapDBEntryConnect::ConnectDir::WEST, new MapConnData(saveFile, 0x2633));
    connectionsChanged();
  }

  // South Connection
  if(toolset->getBit(0x261C, 1, 2)) {
    connections.insert((var8)MapDBEntryConnect::ConnectDir::SOUTH, new MapConnData(saveFile, 0x2628));
    connectionsChanged();
  }

  // North Connection
  if(toolset->getBit(0x261C, 1, 3)) {
    connections.insert((var8)MapDBEntryConnect::ConnectDir::NORTH, new MapConnData(saveFile, 0x261D));
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

  toolset->setBit(0x261C, 1, 0, connections.contains((var8)MapDBEntryConnect::ConnectDir::EAST));
  toolset->setBit(0x261C, 1, 1, connections.contains((var8)MapDBEntryConnect::ConnectDir::WEST));
  toolset->setBit(0x261C, 1, 2, connections.contains((var8)MapDBEntryConnect::ConnectDir::SOUTH));
  toolset->setBit(0x261C, 1, 3, connections.contains((var8)MapDBEntryConnect::ConnectDir::NORTH));

  if(connections.contains((var8)MapDBEntryConnect::ConnectDir::EAST))
    connections.value((var8)MapDBEntryConnect::ConnectDir::EAST)->save(saveFile, 0x263E);
  if(connections.contains((var8)MapDBEntryConnect::ConnectDir::WEST))
    connections.value((var8)MapDBEntryConnect::ConnectDir::WEST)->save(saveFile, 0x2633);
  if(connections.contains((var8)MapDBEntryConnect::ConnectDir::SOUTH))
    connections.value((var8)MapDBEntryConnect::ConnectDir::SOUTH)->save(saveFile, 0x2628);
  if(connections.contains((var8)MapDBEntryConnect::ConnectDir::NORTH))
    connections.value((var8)MapDBEntryConnect::ConnectDir::NORTH)->save(saveFile, 0x261D);

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
    conn->deleteLater();

  connections.clear();
  connectionsChanged();
}

void AreaMap::randomize(MapDBEntry* map, int x, int y)
{
  reset();

  // Stop here if map is not present
  if(map == nullptr)
    return;

  // Current Map ID
  curMap = map->getInd();
  curMapChanged();

  // Map Size including it's double size
  height = (map->getHeight() >= 0) ? map->getHeight() : 0;
  heightChanged();

  width = (map->getWidth() >= 0) ? map->getWidth() : 0;
  widthChanged();

  height2x2 = (map->getHeight() >= 0) ? map->height2X2() : 0;
  height2x2Changed();

  width2x2 = (map->getWidth() >= 0) ? map->width2X2() : 0;
  width2x2Changed();

  // Map basic pointers
  dataPtr = (map->getDataPtr() >= 0) ? map->getDataPtr() : 0;
  dataPtrChanged();

  txtPtr = (map->getTextPtr() >= 0) ? map->getTextPtr() : 0;
  txtPtrChanged();

  scriptPtr = (map->getScriptPtr() >= 0) ? map->getScriptPtr() : 0;
  scriptPtrChanged();

  // Map extra pointers
  if(map->getWidth() >= 0)
    currentTileBlockMapViewPointer = coordsToPtr(x, y, map->getWidth());
  currentTileBlockMapViewPointerChanged();

  mapViewVRAMPointer = VramBGPtr;
  mapViewVRAMPointerChanged();

  if(map->getBorder() >= 0) {
    outOfBoundsBlock = map->getBorder();
    outOfBoundsBlockChanged();
  }

  // Leave these off for now

  if(map->getConnect().contains((var8)MapDBEntryConnect::ConnectDir::NORTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->getConnect().value((var8)MapDBEntryConnect::ConnectDir::NORTH));
    connections.insert((var8)MapDBEntryConnect::ConnectDir::NORTH, conn);
    connectionsChanged();
  }
  if(map->getConnect().contains((var8)MapDBEntryConnect::ConnectDir::EAST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->getConnect().value((var8)MapDBEntryConnect::ConnectDir::EAST));
    connections.insert((var8)MapDBEntryConnect::ConnectDir::EAST, conn);
    connectionsChanged();
  }
  if(map->getConnect().contains((var8)MapDBEntryConnect::ConnectDir::SOUTH)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->getConnect().value((var8)MapDBEntryConnect::ConnectDir::SOUTH));
    connections.insert((var8)MapDBEntryConnect::ConnectDir::SOUTH, conn);
    connectionsChanged();
  }
  if(map->getConnect().contains((var8)MapDBEntryConnect::ConnectDir::WEST)) {
    auto conn = new MapConnData;
    conn->loadFromData(map->getConnect().value((var8)MapDBEntryConnect::ConnectDir::WEST));
    connections.insert((var8)MapDBEntryConnect::ConnectDir::WEST, conn);
    connectionsChanged();
  }
}

void AreaMap::setTo(MapDBEntry* map, int x, int y)
{
  // Right now randomize doesn't really randomize and instead just loads the map
  // and coords given which is what this does. no use in copying it.
  randomize(map, x, y);
}

int AreaMap::coordsToPtr(int x, int y, int width)
{
  return (width+7)+x+(y*(width+6)) + MapDBEntryConnect::worldMapPtr;
}
