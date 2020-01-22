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
#include "./worldhidden.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

WorldHidden::WorldHidden(SaveFile* saveFile)
{
  load(saveFile);
}

WorldHidden::~WorldHidden() {}

void WorldHidden::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Load hidden items
  auto bits = toolset->getBitField(0x299C, hiddenItemByteCount);

  for(var8 i = 0; i < bits.size() && i < hiddenItemCount; i++)
    hiddenItems[i] = bits.at(i);

  hiddenItemsChanged();

  // Load hidden coins
  bits = toolset->getBitField(0x29AA, hiddenCoinByteCount);

  for(var8 i = 0; i < bits.size() && i < hiddenCoinCount; i++)
    hiddenCoins[i] = bits.at(i);

  hiddenCoinsChanged();
}

void WorldHidden::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  QVector<bool> bits;

  // Save Hidden Items
  for(var8 i = 0; i < hiddenItemCount; i++)
    bits.append(hiddenItems[i]);

  toolset->setBitField(0x299C, hiddenItemByteCount, bits);
  bits.clear();

  // Save hidden coins
  for(var8 i = 0; i < hiddenCoinCount; i++)
    bits.append(hiddenCoins[i]);

  toolset->setBitField(0x29AA, hiddenCoinByteCount, bits);
}

void WorldHidden::reset()
{
  memset(hiddenItems, 0, hiddenItemCount);
  hiddenItemsChanged();

  memset(hiddenCoins, 0, hiddenCoinCount);
  hiddenCoinsChanged();
}

// I want the player to collect the hidden items and coins
void WorldHidden::randomize()
{
  reset();
}
