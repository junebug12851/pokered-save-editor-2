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
#include "../../../db/pokemon.h"

#include <QRandomGenerator>

AreaPokemonWild::AreaPokemonWild(var8 index, var8 level)
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
  auto rnd = QRandomGenerator::global();
  auto mon = PokemonDB::ind.value(
        "dex" + QString::number(rnd->bounded(0, pokemonDexCount)));

  index = mon->ind;
  level = rnd->bounded(5, pokemonLevelMax+1);
}

void AreaPokemonWild::reset()
{
  index = 0;
  level = 0;
}

void AreaPokemonWild::load(var8 index, var8 level)
{
  this->index = index;
  this->level = level;
}

void AreaPokemonWild::load(SaveFileIterator* it)
{
  index = it->getByte();
  level = it->getByte();
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
    delete grassMons[i];
    delete waterMons[i];
  }
}

void AreaPokemon::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator();

  pauseMons3Steps = toolset->getBit(0x29D8, 1, 0);
  grassRate = toolset->getByte(0x2B33);
  waterRate = toolset->getByte(0x2B50);

  // Only load wild land Pokemon if present. An encounter rate of zero means
  // They're not present.
  if(grassRate > 0) {
    it->offsetTo(0x2B34);
    for(var8 i = 0; i < wildMonsCount; i++)
      grassMons[i]->load(it);
  }

  if(waterRate > 0) {
    it->offsetTo(0x2B51);
    for(var8 i = 0; i < wildMonsCount; i++)
      waterMons[i]->load(it);
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
  waterRate = 0;
  pauseMons3Steps = false;

  for(var8 i = 0; i < wildMonsCount; i++) {
    grassMons[i]->reset();
    waterMons[i]->reset();
  }
}

void AreaPokemon::randomize()
{
  reset();

  auto rnd = QRandomGenerator::global();

  // Give a reasonable grass and water rate randomization
  grassRate = rnd->bounded(0, 35+1);
  waterRate = rnd->bounded(0, 35+1);

  if(grassRate > 0)
    for(var8 i = 0; i < wildMonsCount; i++)
      grassMons[i]->randomize();

  if(waterRate > 0)
    for(var8 i = 0; i < wildMonsCount; i++)
      waterMons[i]->randomize();
}
