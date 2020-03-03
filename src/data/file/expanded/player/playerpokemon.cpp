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
#include "playerpokemon.h"
#include "./playerbasics.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../fragments/pokemonparty.h"
#include "../../../db/moves.h"
#include "../../../../common/random.h"

PlayerPokemon::PlayerPokemon(SaveFile* saveFile)
  : PokemonStorageBox(boxMaxPokemon)
{
  isParty = true;
  maxSize = 6;
  load(saveFile);
}

PlayerPokemon::~PlayerPokemon()
{
  reset();
}

void PlayerPokemon::load(SaveFile* saveFile, var16 boxOffset)
{
  Q_UNUSED(boxOffset)

  reset();

  this->file = saveFile;

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  for (var8 i = 0; i < toolset->getByte(0x2F2C) && i < 6; i++) {
    pokemon.append(new PokemonParty(
                            saveFile,
                            0x2F34,
                            0x307E,
                            0x303C,
                            i));

    pokemonInsertChange();
  }

  pokemonChanged();
}

void PlayerPokemon::save(SaveFile* saveFile, var16 boxOffset)
{
  Q_UNUSED(boxOffset)

  auto toolset = saveFile->toolset;

  // Set party length and save current party data
  toolset->setByte(0x2F2C, pokemon.size());

  for(var8 i = 0; i < pokemon.size() && i < 6; i++) {
    pokemon.at(i)->save(
          saveFile,
          0x2F34,
          0x2F2D,
          0x307E,
          0x303C,
          i
          );
  }

  // Mark end of species list if not full party
  if(pokemon.size() >= 6)
    return;

  var16 speciesOffset = 0x2F2D + pokemon.size();
  toolset->setByte(speciesOffset, 0xFF);
}

PokemonParty* PlayerPokemon::partyAt(int ind)
{
  return (PokemonParty*)pokemon.at(ind);
}

void PlayerPokemon::randomize(PlayerBasics* basics)
{
  // Randomize up to 5 Pokemon
  var8 count = Random::rangeInclusive(1, 5);

  // Clear Pokemon Party and add them in
  reset();

  for(var8 i = 0; i < count; i++) {
    auto tmp = new PokemonParty;
    pokemon.append(tmp);
    tmp->randomize(basics);
    pokemonInsertChange();
  }

  // Give an extra Pokemon that's an HM slave
  // I have no idea where randomize will drop you so to be able to progress
  // in the game you need to be able to get around
  auto tmp = new PokemonParty;
  pokemon.append(tmp);
  tmp->randomize(basics);

  // Clear out move pool
  for(auto move : tmp->moves)
    delete move;

  tmp->moves.clear();

  // Add in 4 most important HM's
  tmp->moves.append(new PokemonMove(MovesDB::ind.value("FLY")->ind));
  tmp->moves.append(new PokemonMove(MovesDB::ind.value("SURF")->ind));
  tmp->moves.append(new PokemonMove(MovesDB::ind.value("STRENGTH")->ind));
  tmp->moves.append(new PokemonMove(MovesDB::ind.value("CUT")->ind));

  // Generate random PP Ups and heal PP like other Pokemon
  for(auto move : tmp->moves) {
    // Generate random PP Ups
    move->ppUp = Random::rangeInclusive(0, 3);

    // Restore PP of move
    move->restorePP();
  }

  pokemonInsertChange();
  pokemonChanged();
}
