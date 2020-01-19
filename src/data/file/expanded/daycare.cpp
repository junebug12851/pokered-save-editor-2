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

#include "./daycare.h"
#include "./fragments/pokemonbox.h"
#include "./player/playerbasics.h"
#include "../savefile.h"
#include "../savefiletoolset.h"
#include "../savefileiterator.h"
#include "../../../random.h"

Daycare::Daycare(SaveFile* saveFile)
{
  load(saveFile);
}

Daycare::~Daycare()
{
  reset();
}

void Daycare::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  // Is the daycare in use, if so extract daycare Pokemon Information
  if (toolset->getByte(0x2CF4) > 0)
    pokemon = new PokemonBox(saveFile, 0x2D0B, 0x2CF5, 0x2D00, 0);
}

void Daycare::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x2CF4, (pokemon != nullptr) ? 1 : 0);

  if(pokemon != nullptr)
    pokemon->save(saveFile, 0x2D0B, -1, 0x2CF5, 0x2D00, 0);
}

void Daycare::reset()
{
  delete pokemon;
  pokemon = nullptr;
}

void Daycare::randomize(PlayerBasics* basics)
{
  reset();

  // Give a 50/50 chance the daycare will be in use
  if(!Random::chanceSuccess(50))
    return;

  pokemon = new PokemonBox;
  pokemon->randomize(basics);
}

void Daycare::randomize() {}
