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
#include "warpdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../db/mapsearch.h"
#include "../../../../random.h"

WarpData::WarpData(SaveFile* saveFile, var8 index)
{
  load(saveFile, index);
}

WarpData::WarpData(MapDBEntryWarpOut* warp)
{
  load(warp);
}

WarpData::~WarpData() {}

void WarpData::load(SaveFile* saveFile, var8 index)
{
  if(saveFile == nullptr)
    return reset();

  auto it = saveFile->iterator()->offsetTo((0x4 * index) + 0x265B);

  y = it->getByte();
  x = it->getByte();
  destWarp = it->getByte();
  destMap = it->getByte();

  delete it;
}

void WarpData::load(MapDBEntryWarpOut* warp)
{
  x = warp->x;
  y = warp->y;
  destMap = warp->toMap->ind;
  destWarp = warp->warp;
}

void WarpData::save(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x4 * index) + 0x265B);

  it->setByte(y);
  it->setByte(x);
  it->setByte(destWarp);
  it->setByte(destMap);

  delete it;
}

void WarpData::reset()
{
  y = 0;
  x = 0;
  destWarp = 0;
  destMap = 0;
}

void WarpData::randomize() {

  // Grab a non-outdoor map
  // The game can get kind of weird and crash if you warp to an outdoor map
  // directly instead of "returning" outdoors using "Last Map"
  auto map = MapsDB::search()->isGood()->notType("Outdoor")->pickRandom();
  auto mapWarps = map->warpIn;

  // Switch out warp to a random non-outdoor warp
  destMap = map->ind;
  destWarp = Random::rangeExclusive(0, mapWarps.size());
}

MapDBEntry* WarpData::toMap()
{
  return MapsDB::ind.value(QString::number(destMap), nullptr);
}
