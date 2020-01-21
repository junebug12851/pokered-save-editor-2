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
#include "areasign.h"
#include "../fragments/signdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../../random.h"

AreaSign::AreaSign(SaveFile* saveFile)
{
  load(saveFile);
}

AreaSign::~AreaSign()
{
  reset();
}

void AreaSign::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  for (var8 i = 0; i < toolset->getByte(0x275C) && i < 16; i++) {
    signs.append(new SignData(saveFile, i));
  }

  signsChanged();
}

void AreaSign::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x275C, signs.size());
  for (var8 i = 0; i < signs.size() && i < 16; i++) {
    signs.at(i)->save(saveFile, i);
  }
}

void AreaSign::reset()
{
  for(auto sign : signs)
    delete sign;

  signs.clear();

  signsChanged();
}

void AreaSign::randomize(MapDBEntry* mapData)
{
  // Clear all signs
  reset();

  // Grab Map Signs
  auto signData = mapData->signs;

  // Randomize them all if present
  signs = SignData::randomizeAll(signData);
  signsChanged();
}
