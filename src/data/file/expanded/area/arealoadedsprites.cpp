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
#include "arealoadedsprites.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../db/spriteSet.h"
#include "../../../db/sprites.h"
#include "../../../../random.h"

AreaLoadedSprites::AreaLoadedSprites(SaveFile* saveFile)
{
  load(saveFile);
}

AreaLoadedSprites::~AreaLoadedSprites() {}

void AreaLoadedSprites::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  for(auto entry : toolset->getRange(0x2649, 0xB))
    loadedSprites.append(entry);

  loadedSetId = toolset->getByte(0x2654);
}

void AreaLoadedSprites::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->copyRange(0x2649, 0xB, loadedSprites);
  toolset->setByte(0x2654, loadedSetId);
}

void AreaLoadedSprites::reset()
{
  loadedSprites.clear();
  loadedSetId = 0;
}

void AreaLoadedSprites::randomize(MapDBEntry* map, var8 x, var8 y)
{
  // First check to see if the chosen map already has a sprite set
  if(map->toSpriteSet == nullptr) {
    // It does not
    // Pick a random sprite set and load it
    auto spriteSet = SpriteSetDB::store.at(
          Random::rangeExclusive(0, SpriteSetDB::store.size())
          );

    loadSpriteSet(spriteSet, x, y);
  }
  else
    // It does, use that sprite set
    loadSpriteSet(map->toSpriteSet, x, y);
}

void AreaLoadedSprites::loadSpriteSet(SpriteSetDBEntry* entry, var8 x, var8 y)
{
  auto set = entry->getSprites(
        Random::rangeInclusive(x, y),
        Random::rangeInclusive(x, y));

  auto id = entry->ind;

  for(auto s : set)
    loadedSprites.append(s->ind);

  loadedSetId = id;
}

// Not to be used
void AreaLoadedSprites::randomize() {}
