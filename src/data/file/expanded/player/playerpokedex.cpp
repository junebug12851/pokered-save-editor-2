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
#include "playerpokedex.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/names.h"
#include "../../../db/pokemon.h"
#include "../../../../random.h"

PlayerPokedex::PlayerPokedex(SaveFile* saveFile)
{
  load(saveFile);
}

PlayerPokedex::~PlayerPokedex() {}

void PlayerPokedex::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr) {
    return;
  }

  loadPokedex(saveFile, owned, 0x25A3);
  ownedChanged();

  loadPokedex(saveFile, seen, 0x25B6);
  seenChanged();
}

void PlayerPokedex::save(SaveFile* saveFile)
{
  savePokedex(saveFile, owned, 0x25A3);
  savePokedex(saveFile, seen, 0x25B6);
}

void PlayerPokedex::reset()
{
  owned.clear();
  seen.clear();

  owned.fill(false, pokemonDexCount);
  seen.fill(false, pokemonDexCount);
}

// Gives all Pokedex entries a 2 out of 5 chance (40% chance) of being
// seen and/or owned.
void PlayerPokedex::randomize()
{
  reset();

  for(var8 i = 0; i < 151; i++)
  {
    // 15% chance of having them seen or owned
    bool markSeen = Random::chanceSuccess(15);
    bool markOwned = Random::chanceSuccess(15);

    owned[i] = markOwned;
    seen[i] = markSeen;
  }

  ownedChanged();
  seenChanged();
}

void PlayerPokedex::loadPokedex(SaveFile* saveFile, QVector<bool> toArr, var16 fromOffset)
{
  // Get Toolset
  auto toolset = saveFile->toolset;

  // Erase Array
  toArr.clear();

  // Obtain new array from sav file
  auto ret = toolset->getBitField(fromOffset, 0x13);

  // Merge it into main array
  for(auto entry : ret)
    toArr.append(entry);

  // Trim end off to stay within 151 Pokemon
  // One of the complications of bit-fields is that retrieving a bitfield
  // retrives the whole byte
  toArr.pop_back();
}

void PlayerPokedex::savePokedex(SaveFile* saveFile, QVector<bool> fromArr, var16 toOffset)
{
  // Get Toolset
  auto toolset = saveFile->toolset;

  // Apply bitfield to sav file
  // fromArr is already the correct size and doesn't need to be trimmed
  // it will be applied correctly
  toolset->setBitField(toOffset, 0x13, fromArr);
}
