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

#include <string.h>

#include "./worldtowns.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/fly.h"
#include "../../../../random.h"

WorldTowns::WorldTowns(SaveFile* saveFile)
{
  load(saveFile);
}

WorldTowns::~WorldTowns() {}

void WorldTowns::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  auto bits = toolset->getBitField(0x29B7, townByteCount);

  for(var8 i = 0; i < bits.size() && i < townCount; i++)
    visitedTowns[i] = bits.at(i);

  visitedTownsChanged();
}

void WorldTowns::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  QVector<bool> bits;

  for(var8 i = 0; i < townCount; i++)
    bits.append(visitedTowns[i]);

  toolset->setBitField(0x29B7, townByteCount, bits);
}

void WorldTowns::reset()
{
  memset(visitedTowns, 0, townCount);
  visitedTownsChanged();
}

void WorldTowns::randomize()
{
  reset();

  // Enable Pallet Town because it's home and the starting point
  visitedTowns[0] = true;

  // Randomly enable other towns except for Indigo and Saffron
  // Indigo because it's the end and Saffron because it's the big connecting
  // piece
  for(var8 i = 1; i < 10; i++)
  {
    // Not saffron
    if(i == 7)
      continue;

    // Give a 15% chance of enabling each town
    visitedTowns[i] = Random::chanceSuccess(15);
  }

  visitedTownsChanged();
}
