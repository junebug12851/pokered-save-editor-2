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
#include "hofpokemon.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/pokemon.h>
#include <pse-db/names.h>
#include <pse-common/random.h>

HoFPokemon::HoFPokemon(SaveFile* saveFile, var16 recordOffset, var16 ind)
{
  load(saveFile, recordOffset, ind);
}

HoFPokemon::~HoFPokemon() {}

void HoFPokemon::load(SaveFile* saveFile, var16 recordOffset, var16 ind)
{
  reset();
  auto toolset = saveFile->toolset;

  if(saveFile == nullptr) {
    return;
  }

  // Calculate Pokemon Offset in the record
  // Records are 0x10 in size
  // Multiply record number with 0x10 (Record Size) and add to offset
  // record start
  var16 pokemonOffset = (0x10 * ind) + recordOffset;

  /**
   * Record Data
   */

  // Extract Pokemon Data
  species = toolset->getByte(pokemonOffset + 0);
  speciesChanged();

  level = toolset->getByte(pokemonOffset + 1);
  levelChanged();

  name = toolset->getStr(pokemonOffset + 2, 0xB, 10+1);
  nameChanged();
}

void HoFPokemon::save(SaveFile* saveFile, var16 recordOffset, var16 ind)
{
  var16 pokemonOffset = (0x10 * ind) + recordOffset;
  auto toolset = saveFile->toolset;

  toolset->setByte(pokemonOffset + 0, species);
  toolset->setByte(pokemonOffset + 1, level);
  toolset->setStr(pokemonOffset + 2, 0xB, 10+1, name);

  // Normally the Gameboy will actively zero out padding bytes
  // but we don't set padding bytes per the strict "Only touch whats needed" rule
}

void HoFPokemon::reset()
{
  species = 0;
  speciesChanged();

  level = 0;
  levelChanged();

  name = "";
  nameChanged();
}

void HoFPokemon::randomize()
{
  // Reset
  reset();

  // Generate random dex entry and look it up to get species number
  var8 dex = Random::rangeExclusive(1,pokemonDexCount);
  auto toPokemon = PokemonDB::ind.value("dex" + QString::number(dex), nullptr);

  if(toPokemon != nullptr) {
    species = toPokemon->ind;
    speciesChanged();
  }

  // Random level between 1 - 100
  level = Random::rangeInclusive(5,pokemonLevelMax);
  levelChanged();

  // Random name
  name = NamesDB::randomName();
  nameChanged();
}

PokemonDBEntry* HoFPokemon::toSpecies()
{
  return PokemonDB::ind.value(QString::number(species), nullptr);
}
