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
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../fragments/pokemonparty.h"

#include <QRandomGenerator>

PlayerPokemon::PlayerPokemon(SaveFile* saveFile)
{
  party = new QVector<PokemonParty*>();

  load(saveFile);
}

PlayerPokemon::~PlayerPokemon()
{
  reset();
  delete party;
}

void PlayerPokemon::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  reset();

  for (var8 i = 0; i < toolset->getByte(0x2F2C) && i < 6; i++) {
    party->append(new PokemonParty(
                            saveFile,
                            0x2F34,
                            0x307E,
                            0x303C,
                            i));
  }
}

void PlayerPokemon::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  // Set party length and save current party data
  toolset->setByte(0x2F2C, party->size());

  for(var8 i = 0; i < party->size() && i < 6; i++) {
    party->at(i)->save(
          saveFile,
          0x2F34,
          0x2F2D,
          0x307E,
          0x303C,
          i
          );
  }

  // Mark end of species list if not full party
  if(party->size() >= 6)
    return;

  var16 speciesOffset = 0x2F2D + party->size();
  toolset->setByte(speciesOffset, 0xFF);
}

void PlayerPokemon::reset()
{
  for(auto mon : *party)
    delete mon;

  party->clear();
}

void PlayerPokemon::randomize()
{
  auto rnd = QRandomGenerator::global();
  var8 count = rnd->bounded(1, 6+1);

  reset();

  for(var8 i = 0; i < count; i++) {
    auto tmp = new PokemonParty;
    party->append(tmp);
    tmp->randomize();
  }
}
