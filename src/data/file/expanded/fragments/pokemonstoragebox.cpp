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

#include "./pokemonstoragebox.h"
#include "./pokemonbox.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../../random.h"

PokemonStorageBox::PokemonStorageBox(SaveFile* saveFile, var16 boxOffset)
{
  load(saveFile, boxOffset);
}

PokemonStorageBox::~PokemonStorageBox()
{
  reset();
}

void PokemonStorageBox::load(SaveFile* saveFile, var16 boxOffset)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Simply read-in and append new Pokemon in box
  for (var8 i = 0; i < toolset->getByte(boxOffset) && i < boxMaxPokemon; i++) {
    pokemon.append(new PokemonBox(
                     saveFile,
                     boxOffset + 0x16,
                     boxOffset + 0x386,
                     boxOffset + 0x2AA,
                     i));
  }
}

void PokemonStorageBox::save(SaveFile* saveFile, var16 boxOffset)
{
  auto toolset = saveFile->toolset;

  // Set box size
  toolset->setByte(boxOffset, pokemon.size());

  // Save each Pokemon
  for (var8 i = 0; i < pokemon.size() && i < boxMaxPokemon; i++) {
    pokemon.at(i)->save(
          saveFile,
          boxOffset + 0x16,
          boxOffset + 0x1,
          boxOffset + 0x386,
          boxOffset + 0x2AA,
          i
          );
  }

  // Mark end of species list if not full box
  if(pokemon.size() >= boxMaxPokemon)
    return;

  var16 speciesOffset = boxOffset + 1 + pokemon.size();
  toolset->setByte(speciesOffset, 0xFF);
}

void PokemonStorageBox::reset()
{
  for(auto mon : pokemon)
    delete mon;

  pokemon.clear();
}

void PokemonStorageBox::randomize(PlayerBasics* basics)
{
  reset();

  // Go from zero up to half box capacity
  var8 count = Random::rangeInclusive(0, boxMaxPokemon * .50);

  // Insert Pokemon
  for(var8 i = 0; i < count; i++) {
    auto tmp = new PokemonBox;
    tmp->randomize(basics);
    pokemon.append(tmp);
  }
}
