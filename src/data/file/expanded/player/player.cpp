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
#include "player.h"
#include "../../savefile.h"
#include "./playerbasics.h"
#include "./playerpokedex.h"
#include "./playerpokemon.h"
#include "../fragments/itemstoragebox.h"

Player::Player(SaveFile* saveFile)
{
  basics = new PlayerBasics;
  items = new ItemStorageBox(20); // Max 20 items
  pokedex = new PlayerPokedex;
  pokemon = new PlayerPokemon;

  load(saveFile);
}

Player::~Player()
{
  delete basics;
  delete items;
  delete pokedex;
  delete pokemon;
}

void Player::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  basics->load(saveFile);
  items->load(saveFile, 0x25C9);
  pokedex->load(saveFile);
  pokemon->load(saveFile);
}

void Player::save(SaveFile* saveFile)
{
  basics->save(saveFile);
  items->save(saveFile, 0x25C9);
  pokedex->save(saveFile);
  pokemon->save(saveFile);
}

void Player::reset()
{
  basics->reset();
  items->reset();
  pokedex->reset();
  pokemon->reset();
}

void Player::randomize()
{
  basics->randomize();
  items->randomize();
  pokedex->randomize();
  pokemon->randomize(basics);
}
