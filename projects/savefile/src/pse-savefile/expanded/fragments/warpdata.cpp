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
#include <pse-db/maps.h>
#include <pse-db/util/mapsearch.h>
#include <pse-common/random.h>

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
  reset();

  if(saveFile == nullptr)
    return;

  auto it = saveFile->iterator()->offsetTo((0x4 * index) + 0x265B);

  y = it->getByte();
  yChanged();

  x = it->getByte();
  xChanged();

  destWarp = it->getByte();
  destWarpChanged();

  destMap = it->getByte();
  destMapChanged();

  delete it;
}

void WarpData::load(MapDBEntryWarpOut* warp)
{
  x = warp->x;
  xChanged();

  y = warp->y;
  yChanged();

  destMap = warp->toMap->ind;
  destMapChanged();

  destWarp = warp->warp;
  destWarpChanged();
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
  yChanged();

  x = 0;
  xChanged();

  destWarp = 0;
  destWarpChanged();

  destMap = 0;
  destMapChanged();
}

void WarpData::randomize() {
  reset();

  // Grab a non-outdoor map
  // The game can get kind of weird and crash if you warp to an outdoor map
  // directly instead of "returning" outdoors using "Last Map"
  auto map = MapsDB::search()->isGood()->notType("Outdoor")->pickRandom();
  auto mapWarps = map->warpIn;

  // Switch out warp to a random non-outdoor warp
  destMap = map->ind;
  destMapChanged();

  destWarp = Random::rangeExclusive(0, mapWarps.size());
  destWarpChanged();

  y = mapWarps[destWarp]->y;
  yChanged();

  x = mapWarps[destWarp]->x;
  xChanged();
}

MapDBEntry* WarpData::toMap()
{
  return MapsDB::ind.value(QString::number(destMap), nullptr);
}
