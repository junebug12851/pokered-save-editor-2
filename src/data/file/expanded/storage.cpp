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

#include "./storage.h"
#include "./fragments/pokemonstorageset.h"
#include "./fragments/pokemonstoragebox.h"
#include "./fragments/itemstoragebox.h"
#include "../savefile.h"
#include "../savefiletoolset.h"
#include "../savefileiterator.h"

Storage::Storage(SaveFile* saveFile)
{
  items = new ItemStorageBox;

  for(var8 i = 0; i < maxPokemonStorageSets; i++)
    pokemon[i] = new PokemonStorageSet;

  load(saveFile);
}

Storage::~Storage()
{
  delete items;

  for(var8 i = 0; i < maxPokemonStorageSets; i++)
    delete pokemon[i];
}

void Storage::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Now Pokemon operates off a cached version of a box, so when switching boxes
  // it saves the box from the cache to storage and then loads the new box into
  // cache. All changes work off of the cache and only get saved back to storage
  // when switching boxes. It's imperative we replicate this.

  // Current Box, last bit represents whether the other boxes are formatted or
  // not. If they aren't formatted then don't load them as they contain garbage
  // information
  curBox = (toolset->getByte(0x284C) & 0b01111111);
  boxesFormatted = toolset->getBit(0x284C, 0x1, 7);

  // Load Items and Boxes 1-12 in 2 sets of 1-6
  items->load(saveFile);

  // Determine how to play out current box loading
  // We only want to load all but current box
  bool curSetB = curBox >= setMaxBoxes; // Which set is the current box in
  var8 cur = curBox; // Get the current box
  if(curSetB)
    cur -= setMaxBoxes; // Offset if it's in set b

  // Load all of these and skip current box in correct set only if the boxes
  // have been formatted
  if(boxesFormatted) {
    pokemon[0]->load(saveFile, 0x4000, curSetB ? -1 : cur);
    pokemon[1]->load(saveFile, 0x6000, curSetB ? cur : -1);
  }

  // Load cached box to current box data regardless of boxes being formatted
  // or not
  pokemon[curSetB ? 1 : 0]->loadSpecific(saveFile, 0x30C0, cur);
}

void Storage::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  // Save current box details
  toolset->setByte(0x284C, curBox);
  toolset->setBit(0x284C, 1, 7, boxesFormatted);

  items->save(saveFile);

  bool curSetB = curBox >= setMaxBoxes; // Which set is the current box in
  var8 cur = curBox; // Get the current box
  if(curSetB)
    cur -= setMaxBoxes; // Offset if it's in set b

  // Load all of these and skip current box in correct set only if the boxes
  // have been formatted
  if(boxesFormatted) {
    pokemon[0]->save(saveFile, 0x4000, curSetB ? -1 : cur);
    pokemon[1]->save(saveFile, 0x6000, curSetB ? cur : -1);
  }

  // Load cached box to current box data regardless of boxes being formatted
  // or not
  pokemon[curSetB ? 1 : 0]->saveSpecific(saveFile, 0x30C0, cur);
}

void Storage::reset()
{
  curBox = 0;
  boxesFormatted = false;

  items->reset();

  for(var8 i = 0; i < maxPokemonStorageSets; i++)
    pokemon[i]->reset();
}

void Storage::randomize(PlayerBasics* basics)
{
  items->randomize();

  for(var8 i = 0; i < maxPokemonStorageSets; i++)
    pokemon[i]->randomize(basics);
}
