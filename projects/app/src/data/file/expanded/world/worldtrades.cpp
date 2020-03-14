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

#include "./worldtrades.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

WorldTrades::WorldTrades(SaveFile* saveFile)
{
  load(saveFile);
}

WorldTrades::~WorldTrades() {}

void WorldTrades::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  auto bits = toolset->getBitField(0x29E3, tradeByteCount);

  for(var8 i = 0; i < bits.size() && i < tradeCount; i++)
    completedTrades[i] = bits.at(i);

  completedTradesChanged();
}

void WorldTrades::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  QVector<bool> bits;

  for(var8 i = 0; i < tradeCount; i++)
    bits.append(completedTrades[i]);

  toolset->setBitField(0x29E3, tradeByteCount, bits);
}

int WorldTrades::tradesCount()
{
  return tradeCount;
}

bool WorldTrades::tradesAt(int ind)
{
  return completedTrades[ind];
}

void WorldTrades::tradesSet(int ind, bool val)
{
  completedTrades[ind] = val;
  completedTradesChanged();
}

void WorldTrades::reset()
{
  memset(completedTrades, 0, tradeCount);
  completedTradesChanged();
}

// Don't randomize or complete any trades
void WorldTrades::randomize() {
  reset();
}
