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
#include "../../../../common/random.h"

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

  // Load Owned
  QVector<bool> tmp;
  loadPokedex(saveFile, &tmp, 0x25A3);

  for(var8 i = 0; i < maxPokedex && i < tmp.size(); i++) {
    owned[i] = tmp.at(i);
    dexItemChanged(i);
  }

  tmp.clear();

  // Load Seen
  loadPokedex(saveFile, &tmp, 0x25B6);

  for(var8 i = 0; i < maxPokedex && i < tmp.size(); i++) {
    seen[i] = tmp.at(i);
    dexItemChanged(i);
  }

  tmp.clear();

  dexChanged();
}

void PlayerPokedex::save(SaveFile* saveFile)
{
  QVector<bool> tmp;

  // Save owned
  for(var8 i = 0; i < maxPokedex; i++)
    tmp.append(owned[i]);

  savePokedex(saveFile, &tmp, 0x25A3);
  tmp.clear();

  // Save seen
  for(var8 i = 0; i < maxPokedex; i++)
    tmp.append(seen[i]);

  savePokedex(saveFile, &tmp, 0x25B6);
  tmp.clear();
}

void PlayerPokedex::reset()
{
  memset(owned, pokemonDexCount, maxPokedex * sizeof(bool));
  memset(seen, pokemonDexCount, maxPokedex * sizeof(bool));
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

    dexItemChanged(i);
  }

  dexChanged();
}

void PlayerPokedex::toggleAll()
{
  bool valSeen = seen[0];
  bool valOwn = owned[0];

  // Toggle between these 3 states in this order
  // 1. None
  // 2. Seen
  // 3. Owned

  if(valOwn) // Owned (3) -> None (1)
    markAll(DexNone);
  else if(valSeen) // Seen (2) -> Owned (3)
    markAll(DexOwned);
  else // None (1) -> Seen (2)
    markAll(DexSeen);
}

void PlayerPokedex::toggleOne(int val)
{
  bool valSeen = seen[val];
  bool valOwn = owned[val];

  if(valOwn) { // Owned (3) -> None (1)
    this->seen[val] = false;
    this->owned[val] = false;
  }
  else if(valSeen) { // Seen (2) -> Owned (3)
    this->seen[val] = true;
    this->owned[val] = true;
  }
  else { // None (1) -> Seen (2)
    this->seen[val] = true;
    this->owned[val] = false;
  }

  // Notify Changes
  dexItemChanged(val);
  dexChanged();
}

void PlayerPokedex::markAll(int val)
{
  // Start with None
  bool owned = false;
  bool seen = false;

  // If seen only, then mark seen
  if(val == DexSeen)
    seen = true;

  // If owned, then mark both true
  else if(val == DexOwned) {
    seen = true;
    owned = true;
  }

  // Apply changes to all
  for(int i = 0; i < maxPokedex; i++) {
    this->seen[i] = seen;
    this->owned[i] = owned;
    dexItemChanged(i);
  }

  // Notify changes
  dexChanged();
}

void PlayerPokedex::loadPokedex(SaveFile* saveFile, QVector<bool>* toArr, var16 fromOffset)
{
  // Get Toolset
  auto toolset = saveFile->toolset;

  // Erase Array
  toArr->clear();

  // Obtain new array from sav file
  auto ret = toolset->getBitField(fromOffset, 0x13);

  // Merge it into main array
  for(auto entry : ret)
    toArr->append(entry);

  // Trim end off to stay within 151 Pokemon
  // One of the complications of bit-fields is that retrieving a bitfield
  // retrives the whole byte
  toArr->pop_back();
}

void PlayerPokedex::savePokedex(SaveFile* saveFile, QVector<bool>* fromArr, var16 toOffset)
{
  // Get Toolset
  auto toolset = saveFile->toolset;

  // Apply bitfield to sav file
  // fromArr is already the correct size and doesn't need to be trimmed
  // it will be applied correctly
  toolset->setBitField(toOffset, 0x13, *fromArr);
}

int PlayerPokedex::ownedCount()
{
  int ret = 0;

  for(int i = 0; i < maxPokedex; i++) {
    if(owned[i])
      ret++;
  }

  return ret;
}

int PlayerPokedex::seenCount()
{
  int ret = 0;

  for(int i = 0; i < maxPokedex; i++) {
    if(seen[i])
      ret++;
  }

  return ret;
}

int PlayerPokedex::ownedMax()
{
  return maxPokedex;
}

bool PlayerPokedex::ownedAt(int ind)
{
  return owned[ind];
}

void PlayerPokedex::ownedSet(int ind, bool val)
{
  owned[ind] = val;
  dexChanged();
  dexItemChanged(ind);
}

int PlayerPokedex::seenMax()
{
  return maxPokedex;
}

bool PlayerPokedex::seenAt(int ind)
{
  return seen[ind];
}

void PlayerPokedex::seenSet(int ind, bool val)
{
  seen[ind] = val;
  dexChanged();
  dexItemChanged(ind);
}

int PlayerPokedex::getState(int ind)
{
  bool valSeen = seen[ind];
  bool valOwn = owned[ind];

  if(valOwn) { // Owned (3)
    return DexOwned;
  }
  else if(valSeen) { // Seen (2)
    return DexSeen;
  }
  else { // None (1)
    return DexNone;
  }
}
