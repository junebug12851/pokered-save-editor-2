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
#include "signdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

#include <QRandomGenerator>

SignData::SignData(SaveFile* saveFile, var8 index)
{
  load(saveFile, index);
}

SignData::~SignData() {}

void SignData::load(SaveFile* saveFile, var8 index)
{
  if(saveFile == nullptr)
    return reset();

  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x275D);
  y = it->getByte();
  x = it->getByte();

  it->offsetTo((1 * index) + 0x277D);
  txtId = it->getByte();
}

void SignData::save(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x275D);
  it->setByte(y);
  it->setByte(x);

  it->offsetTo((1 * index) + 0x277D);
  it->setByte(txtId);
}

void SignData::reset()
{
  x = 0;
  y = 0;
  txtId = 0;
}

// Can't really randomize because we don't know all the text IDs for each map
// @TODO Make this possible
void SignData::randomize() {}

// Not to use
void SignData::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void SignData::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}
