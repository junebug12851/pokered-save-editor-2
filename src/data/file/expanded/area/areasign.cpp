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

#include <QRandomGenerator>

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
}

void AreaSign::randomize(MapDBEntry* mapData)
{
  // Clear all signs
  reset();

  auto rnd = QRandomGenerator::global();
  auto signData = mapData->signs;

  // We randomize the sign text ids if there are any
  if(signData.size() == 0)
    return;

  // Get a list of all the sign data
  QVector<var8> ids;

  for(auto sign : signData)
    ids.append(sign->textID);

  // Go through each sign data on map
  // Add a new sign with correct x & y data but a random txt id from ids above
  // Delete id and move onto next sign
  for(auto sign : signData) {
    auto data = new SignData();
    auto ind = rnd->bounded(0, ids.size());

    data->x = sign->x;
    data->y = sign->y;
    data->txtId = ids.at(ind);

    ids.removeAt(ind);
    this->signs.append(data);
  }
}

// Not to be used
void AreaSign::randomize() {}
