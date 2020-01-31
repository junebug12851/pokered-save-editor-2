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
#include "hofrecord.h"
#include "./hofpokemon.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../../common/random.h"

HoFRecord::HoFRecord(SaveFile* saveFile, var8 ind)
{
  load(saveFile, ind);
}

HoFRecord::~HoFRecord()
{
  reset();
}

int HoFRecord::pokemonCount()
{
  return pokemon.size();
}

int HoFRecord::pokemonMax()
{
  return maxPokemon;
}

HoFPokemon* HoFRecord::pokemonAt(int ind)
{
  return pokemon.at(ind);
}

void HoFRecord::pokemonSwap(int from, int to)
{
  auto eFrom = pokemon.at(from);
  auto eTo = pokemon.at(to);

  pokemon.replace(from, eTo);
  pokemon.replace(to, eFrom);

  pokemonChanged();
}

void HoFRecord::pokemonRemove(int ind)
{
  // Can't have a team of no Pokemon
  if(pokemon.size() <= 1)
    return;

  delete pokemon.at(ind);

  pokemon.removeAt(ind);
  pokemonChanged();
}

void HoFRecord::pokemonNew()
{
  if(pokemon.size() >= maxPokemon)
    return;

  pokemon.append(new HoFPokemon);
  pokemonChanged();
}

void HoFRecord::load(SaveFile* saveFile, var8 ind)
{
  // Reset to clear list as all the entries have ti be deleted first
  reset();

  if(saveFile == nullptr) {
    return;
  }

  auto toolset = saveFile->toolset;

  // Calculate HoF Offset for this record
  // Records are 0x60 in size, all records start at 0x598
  // Multiply record number with 0x60 (Record Size) and add to offset
  // of first record (Record 0) which is 0x598
  var16 offset = (0x60 * ind) + 0x598;

  for (var8 i = 0; i < 6; i++) {
    // If Pokemon doesn't exist then don't proceed any further
    // the data stop code is 0xFF
    var16 pokemonOffset = (0x10 * i) + offset;
    var8 speciesByte = toolset->getByte(pokemonOffset + 0);
    if (speciesByte == 0xFF)
      break;

    pokemon.append(new HoFPokemon(saveFile, offset, i));
  }

  pokemonChanged();
}

void HoFRecord::save(SaveFile* saveFile, var8 ind)
{
  auto toolset = saveFile->toolset;
  var16 offset = (0x60 * ind) + 0x598;

  for (var8 i = 0; i < pokemon.size(); i++) {
    pokemon.at(i)->save(saveFile, offset, i);
  }

  // If the record isn't filled up with 6 Pokemon then
  // we need to insert an ending marker and not touch the rest of the bytes
  if(pokemon.size() >= 6)
    return;

  // Calculate the ending marker position which is
  // 1 byte after all record data and set to 0xFF thus sealing the rest
  // of the data

  // Pokemon Record Size * Pokemon Record Number + Record Start Location
  var16 endingOffset = (0x10 * pokemon.size()) + offset;
  toolset->setByte(endingOffset, 0xFF);
}

void HoFRecord::reset()
{
  for(auto entry : pokemon)
    delete entry;

  pokemon.clear();
  pokemonChanged();
}

void HoFRecord::randomize()
{
  // Reset
  reset();

  // Get a random amount of Pokemon between 1-6
  var8 size = Random::rangeInclusive(1,6);

  // Create blank Pokemon entries
  for(var8 i = 0; i < size; i++)
    pokemon.append(new HoFPokemon);

  pokemonChanged();

  // Insert random data
  for(auto entry : pokemon)
    entry->randomize();
}
