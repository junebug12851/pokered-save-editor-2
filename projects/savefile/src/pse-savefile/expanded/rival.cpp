/*
  * Copyright 2020 Twilight
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

#include "./rival.h"
#include "../savefile.h"
#include "../savefiletoolset.h"
#include "../savefileiterator.h"
#include <pse-db/names.h>
#include <pse-db/pokemon.h>
#include <pse-common/random.h>

Rival::Rival(SaveFile* saveFile)
{
  load(saveFile);
}

Rival::~Rival() {}

void Rival::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  name = toolset->getStr(0x25F6, 0xB, 7+1);
  nameChanged();

  starter = toolset->getByte(0x29C1);
  starterChanged();
}

void Rival::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setStr(0x25F6, 0xB, 7+1, name);
  toolset->setByte(0x29C1, starter);
}

void Rival::reset()
{
  name = "";
  nameChanged();

  starter = 0;
  starterChanged();
}

void Rival::randomize()
{
  reset();

  name = Names::inst()->player()->randomExample();
  nameChanged();

  var8 starters[3] = {
    PokemonDB::inst()->getIndAt("Charmander")->ind,
    PokemonDB::inst()->getIndAt("Squirtle")->ind,
    PokemonDB::inst()->getIndAt("Bulbasaur")->ind
  };

  // Get a random starter
  starter = starters[Random::inst()->rangeExclusive(0, 3)];
  starterChanged();
}
