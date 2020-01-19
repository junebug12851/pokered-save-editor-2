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
#include "../../../../random.h"

AreaTileset::AreaTileset(SaveFile* saveFile)
{
  load(saveFile);
}

AreaTileset::~AreaTileset() {}

void AreaTileset::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  current = toolset->getByte(0x2613);
  bank = toolset->getByte(0x27D7);

  blockPtr = toolset->getWord(0x27D8, true);
  gfxPtr = toolset->getWord(0x27DA, true);
  collPtr = toolset->getWord(0x27DC, true);

  auto savTot = toolset->getRange(0x27DE, 3);
  for(var8 i = 0; i < talkCount; i++) {
    talkingOverTiles[i] = savTot[i];
  }

  grassTile = toolset->getByte(0x27E1);
  boulderIndex = toolset->getByte(0x29C4);
  boulderColl = toolset->getByte(0x29C8);

  type = toolset->getByte(0x3522);
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
  memset(talkingOverTiles, 0, 3);
  grassTile = 0;
  boulderIndex = 0;
  boulderColl = 0;
  type = 0;
  bank = 0;
  blockPtr = 0;
  gfxPtr = 0;
  collPtr = 0;
}

void AreaTileset::randomize()
{
  reset();

  // Random between types
  type = Random::rangeInclusive(0, 2);
}

void AreaTileset::loadFromData(MapDBEntry* map, bool randomType)
{
  auto tileset = map->toTileset;

  // If random, have it clear everything and randomize type
  // Otherwise load usual type
  if(randomType)
    randomize();
  else
    type = (var8)tileset->typeAsEnum();

  // Load other usual data based on map
  current = tileset->ind;
  grassTile = tileset->grass;
  bank = tileset->bank;
  blockPtr = tileset->blockPtr;
  gfxPtr = tileset->gfxPtr;
  collPtr = tileset->collPtr;

  for(var8 i = 0; i < talkCount; i++) {
    talkingOverTiles[i] = tileset->talk[i];
  }
}
