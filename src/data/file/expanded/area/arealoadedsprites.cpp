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
#include "../../../../common/random.h"

AreaLoadedSprites::AreaLoadedSprites(SaveFile* saveFile)
{
  memset(loadedSprites, 0, maxLoadedSprites * sizeof(var8));
  load(saveFile);
}

AreaLoadedSprites::~AreaLoadedSprites() {}

int AreaLoadedSprites::lSpriteCount()
{
  return maxLoadedSprites;
}

int AreaLoadedSprites::lSpriteAt(int ind)
{
  return loadedSprites[ind];
}

void AreaLoadedSprites::lSpriteSwap(int from, int to)
{
  auto eFrom = loadedSprites[from];
  auto eTo = loadedSprites[to];

  loadedSprites[to] = eFrom;
  loadedSprites[from] = eTo;

  loadedSetIdChanged();
}

void AreaLoadedSprites::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  auto rng = toolset->getRange(0x2649, 0xB);
  for(var8 i = 0; i < maxLoadedSprites; i++)
  {
    loadedSprites[i] = rng.at(i);
  }

  loadedSpritesChanged();

  loadedSetId = toolset->getByte(0x2654);
  loadedSetIdChanged();
}

void AreaLoadedSprites::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  QVector<var8> rng;
  for(var8 i = 0; i < maxLoadedSprites; i++)
  {
    rng.append(loadedSprites[i]);
  }


  toolset->copyRange(0x2649, 0xB, rng);
  toolset->setByte(0x2654, loadedSetId);
}

void AreaLoadedSprites::reset()
{
  memset(loadedSprites, 0, maxLoadedSprites * sizeof(var8));
  loadedSpritesChanged();

  loadedSetId = 0;
  loadedSetIdChanged();
}

void AreaLoadedSprites::randomize(MapDBEntry* map, int x, int y)
{
  reset();

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

void AreaLoadedSprites::loadSpriteSet(SpriteSetDBEntry* entry, int x, int y)
{
  auto set = entry->getSprites(
        Random::rangeInclusive(x, y),
        Random::rangeInclusive(x, y));

  auto id = entry->ind;

  for(var8 i = 0; i < set.size(); i++)
  {
    loadedSprites[i] = set.at(i)->ind;
  }

  loadedSpritesChanged();

  loadedSetId = id;
  loadedSetIdChanged();
}
