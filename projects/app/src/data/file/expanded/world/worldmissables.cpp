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
#include <QVector>

#include "./worldmissables.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/missables.h>

WorldMissables::WorldMissables(SaveFile* saveFile)
{
  load(saveFile);
}

WorldMissables::~WorldMissables() {}

void WorldMissables::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Load missables
  auto bits = toolset->getBitField(0x2852, missableByteCount);

  for(var8 i = 0; i < bits.size() && i < missableCount; i++)
    missables[i] = bits.at(i);

  missablesChanged();
}

void WorldMissables::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  QVector<bool> bits;

  // Save Missables
  for(var8 i = 0; i < missableCount; i++)
    bits.append(missables[i]);

  toolset->setBitField(0x2852, missableByteCount, bits);
}

int WorldMissables::missablesCount()
{
  return missableCount;
}

bool WorldMissables::missablesAt(int ind)
{
  return missables[ind];
}

void WorldMissables::missablesSet(int ind, bool val)
{
  missables[ind] = val;
  missablesChanged();
}

void WorldMissables::reset()
{
  // Missables can't be zeroed out whimsically, reset back to game start
  // defaults. Missables can crash the game if mis-handled
  // 1 = Hide, 0 = Show, ensure it's marked one if it's hidden
  for(auto missable : MissablesDB::store)
    missables[missable->ind] = !missable->defShow;

  missablesChanged();
}

// Missables is not something you can blantly randomize, the game will likely
// crash. Also we want the player to progress through the game normaly.
void WorldMissables::randomize() {
  reset();
}
