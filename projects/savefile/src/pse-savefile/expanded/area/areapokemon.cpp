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
#include "./areapokemon.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/pokemon.h>
#include <pse-common/random.h>
#include <pse-db/mapsdb.h>

AreaPokemonWild::AreaPokemonWild(int index, int level)
{
  load(index, level);
}

AreaPokemonWild::AreaPokemonWild(bool random)
{
  if(random)
    randomize();
}

bool AreaPokemonWild::operator<(const AreaPokemonWild& a)
{
  return level < a.level;
}

bool AreaPokemonWild::operator>(const AreaPokemonWild& a)
{
  return level > a.level;
}

void AreaPokemonWild::randomize()
{
  reset();

  auto mon = PokemonDB::ind.value(
        "dex" + QString::number(Random::rangeExclusive(0, pokemonDexCount)));

  index = mon->ind;
  indexChanged();

  level = Random::rangeInclusive(5, pokemonLevelMax);
  levelChanged();
}

void AreaPokemonWild::reset()
{
  index = 0;
  indexChanged();

  level = 0;
  levelChanged();
}

void AreaPokemonWild::load(int index, int level)
{
  this->index = index;
  indexChanged();

  this->level = level;
  levelChanged();
}

void AreaPokemonWild::load(SaveFileIterator* it)
{
  index = it->getByte();
  indexChanged();

  level = it->getByte();
  levelChanged();
}

void AreaPokemonWild::save(SaveFileIterator* it)
{
  it->setByte(index);
  it->setByte(level);
}

AreaPokemon::AreaPokemon(SaveFile* saveFile)
{
  // Pre-generate 10 Pokemon each, keep these until the class is deleted
  for(var8 i = 0; i < wildMonsCount; i++) {
    grassMons[i] = new AreaPokemonWild;
    waterMons[i] = new AreaPokemonWild;
  }

  load(saveFile);
}

AreaPokemon::~AreaPokemon()
{
  for(var8 i = 0; i < wildMonsCount; i++) {
    grassMons[i]->deleteLater();
    waterMons[i]->deleteLater();
  }
}

int AreaPokemon::grassMonsCount()
{
  return wildMonsCount;
}

AreaPokemonWild* AreaPokemon::grassMonsAt(int ind)
{
  return grassMons[ind];
}

void AreaPokemon::grassMonsSwap(int from, int to)
{
  auto eFrom = grassMons[from];
  auto eTo = grassMons[to];

  grassMons[from] = eTo;
  grassMons[to] = eFrom;

  grassMonsChanged();
}

int AreaPokemon::waterMonsCount()
{
  return wildMonsCount;
}

AreaPokemonWild* AreaPokemon::waterMonsAt(int ind)
{
  return waterMons[ind];
}

void AreaPokemon::waterMonsSwap(int from, int to)
{
  auto eFrom = waterMons[from];
  auto eTo = waterMons[to];

  waterMons[from] = eTo;
  waterMons[to] = eFrom;

  waterMonsChanged();
}

void AreaPokemon::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator();

  pauseMons3Steps = toolset->getBit(0x29D8, 1, 0);
  pauseMons3StepsChanged();

  grassRate = toolset->getByte(0x2B33);
  grassRateChanged();

  waterRate = toolset->getByte(0x2B50);
  waterRateChanged();

  // Only load wild land Pokemon if present. An encounter rate of zero means
  // They're not present.
  if(grassRate > 0) {
    it->offsetTo(0x2B34);
    for(var8 i = 0; i < wildMonsCount; i++)
      grassMons[i]->load(it);
    grassMonsChanged();
  }

  if(waterRate > 0) {
    it->offsetTo(0x2B51);
    for(var8 i = 0; i < wildMonsCount; i++)
      waterMons[i]->load(it);
    waterMonsChanged();
  }

  delete it;
}

void AreaPokemon::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator();

  toolset->setBit(0x29D8, 1, 0, pauseMons3Steps);
  toolset->setByte(0x2B33, grassRate);
  toolset->setByte(0x2B50, waterRate);

  // Only save wild land Pokemon if present. An encounter rate of zero means
  // They're not present.
  if(grassRate > 0) {
    it->offsetTo(0x2B34);
    for(var8 i = 0; i < wildMonsCount; i++)
      grassMons[i]->save(it);
  }

  if(waterRate > 0) {
    it->offsetTo(0x2B51);
    for(var8 i = 0; i < wildMonsCount; i++)
      waterMons[i]->save(it);
  }

  delete it;
}

void AreaPokemon::reset()
{
  grassRate = 0;
  grassRateChanged();

  waterRate = 0;
  waterRateChanged();

  pauseMons3Steps = false;
  pauseMons3StepsChanged();

  for(var8 i = 0; i < wildMonsCount; i++) {
    grassMons[i]->reset();
    grassMonsChanged();

    waterMons[i]->reset();
    waterMonsChanged();
  }
}

void AreaPokemon::randomize()
{
  reset();

  // Give a reasonable grass and water rate randomization
  grassRate = Random::rangeInclusive(0, 35);
  grassRateChanged();

  waterRate = Random::rangeInclusive(0, 35);
  waterRateChanged();

  if(grassRate > 0)
    for(var8 i = 0; i < wildMonsCount; i++) {
      grassMons[i]->randomize();
      grassMonsChanged();
    }

  if(waterRate > 0)
    for(var8 i = 0; i < wildMonsCount; i++) {
      waterMons[i]->randomize();
      waterMonsChanged();
    }
}

void AreaPokemon::setTo(MapDBEntry* map)
{
  reset();

  // Give a reasonable grass and water rate randomization
  grassRate = (map == nullptr || !map->monRate)
      ? 0
      : *map->monRate;
  grassRateChanged();

  waterRate = (map == nullptr || !map->monRateWater)
      ? 0
      : *map->monRateWater;
  waterRateChanged();

  if(map == nullptr)
    return;

  bool redOrBlue = Random::flipCoin();
  auto monData = (redOrBlue) ? map->monsRed : map->monsBlue;

  for(int i = 0; i < monData.size(); i++) {
    grassMons[i]->index = monData.at(i)->toPokemon->ind;
    grassMons[i]->indexChanged();
    grassMonsChanged();

    grassMons[i]->level = monData.at(i)->level;
    grassMons[i]->levelChanged();
    grassMonsChanged();
  }

  auto monDataWater = map->monsWater;

  for(int i = 0; i < monDataWater.size(); i++) {
    waterMons[i]->index = monDataWater.at(i)->toPokemon->ind;
    waterMons[i]->indexChanged();
    waterMonsChanged();

    waterMons[i]->level = monDataWater.at(i)->level;
    waterMons[i]->levelChanged();
    waterMonsChanged();
  }
}
