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

#include <cstring>

#include "./areatileset.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../db/tileset.h"
#include "../../../../common/random.h"

AreaTileset::AreaTileset(SaveFile* saveFile)
{
  load(saveFile);
}

AreaTileset::~AreaTileset() {}

int AreaTileset::talkingOverTilesCount()
{
  return maxTalkingOverTiles;
}

int AreaTileset::talkingOverTilesAt(int ind)
{
  return talkingOverTiles[ind];
}

void AreaTileset::talkingOverTilesSwap(int from, int to)
{
  auto eFrom = talkingOverTiles[from];
  auto eTo = talkingOverTiles[to];

  talkingOverTiles[to] = eFrom;
  talkingOverTiles[from] = eTo;

  talkingOverTilesChanged();
}

void AreaTileset::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  current = toolset->getByte(0x2613);
  currentChanged();

  bank = toolset->getByte(0x27D7);
  bankChanged();

  blockPtr = toolset->getWord(0x27D8, true);
  blockPtrChanged();

  gfxPtr = toolset->getWord(0x27DA, true);
  gfxPtrChanged();

  collPtr = toolset->getWord(0x27DC, true);
  collPtrChanged();

  auto savTot = toolset->getRange(0x27DE, 3);
  for(var8 i = 0; i < talkCount; i++) {
    talkingOverTiles[i] = savTot[i];
  }
  talkingOverTilesChanged();

  grassTile = toolset->getByte(0x27E1);
  grassTileChanged();

  boulderIndex = toolset->getByte(0x29C4);
  boulderIndexChanged();

  boulderColl = toolset->getByte(0x29C8);
  boulderCollChanged();

  type = toolset->getByte(0x3522);
  typeChanged();
}

void AreaTileset::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x2613, current);
  toolset->setByte(0x27D7, bank);

  toolset->setWord(0x27D8, blockPtr, true);
  toolset->setWord(0x27DA, gfxPtr, true);
  toolset->setWord(0x27DC, collPtr, true);

  for(var8 i = 0; i < talkCount; i++) {
    toolset->setByte(0x27DE + i, talkingOverTiles[i]);
  }

  toolset->setByte(0x27E1, grassTile);
  toolset->setByte(0x29C4, boulderIndex);
  toolset->setByte(0x29C8, boulderColl);

  toolset->setByte(0x3522, type);
}

void AreaTileset::reset()
{
  current = 0;
  currentChanged();

  memset(talkingOverTiles, 0, 3 * sizeof(var8));
  talkingOverTilesChanged();

  grassTile = 0;
  grassTileChanged();

  boulderIndex = 0;
  boulderIndexChanged();

  boulderColl = 0;
  boulderCollChanged();

  type = 0;
  typeChanged();

  bank = 0;
  bankChanged();

  blockPtr = 0;
  blockPtrChanged();

  gfxPtr = 0;
  gfxPtrChanged();

  collPtr = 0;
  collPtrChanged();
}

void AreaTileset::randomize()
{
  reset();

  // Random between types
  type = Random::rangeInclusive(0, 2);
  typeChanged();
}

void AreaTileset::loadFromData(MapDBEntry* map, bool randomType)
{
  reset();

  auto tileset = (map == nullptr)
      ? map->toTileset
      : nullptr;

  // If random, have it clear everything and randomize type
  // Otherwise load usual type
  if(randomType) {
    type = Random::rangeInclusive(0, 2);
    typeChanged();
  }
  else {
    type = (tileset == nullptr)
        ? 0
        : (var8)tileset->typeAsEnum();
    typeChanged();
  }

  // Load other usual data based on map
  current = (tileset == nullptr)
      ? 0
      : tileset->ind;
  currentChanged();

  grassTile = (tileset == nullptr)
      ? 0
      : tileset->grass;
  grassTileChanged();

  bank = (tileset == nullptr)
      ? 0
      : tileset->bank;
  bankChanged();

  blockPtr = (tileset == nullptr)
      ? 0
      : tileset->blockPtr;
  blockPtrChanged();

  gfxPtr = (tileset == nullptr)
      ? 0
      : tileset->gfxPtr;
  gfxPtrChanged();

  collPtr = (tileset == nullptr)
      ? 0
      : tileset->collPtr;
  collPtrChanged();

  for(var8 i = 0; i < talkCount; i++) {
    talkingOverTiles[i] = (tileset == nullptr)
        ? 0
        : tileset->talk[i];
  }
  talkingOverTilesChanged();
}
