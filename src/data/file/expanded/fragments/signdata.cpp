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
#include "../../../db/maps.h"
#include "../../../../random.h"

struct TmpSignPos {
  var8 x;
  var8 y;
};

SignData::SignData(SaveFile* saveFile, var8 index)
{
  load(saveFile, index);
}

SignData::~SignData() {}

void SignData::load(SaveFile* saveFile, var8 index)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x275D);
  y = it->getByte();
  yChanged();

  x = it->getByte();
  xChanged();

  it->offsetTo((1 * index) + 0x277D);
  txtId = it->getByte();
  txtIdChanged();

  delete it;
}

void SignData::save(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x275D);
  it->setByte(y);
  it->setByte(x);

  it->offsetTo((1 * index) + 0x277D);
  it->setByte(txtId);

  delete it;
}

void SignData::reset()
{
  x = 0;
  xChanged();

  y = 0;
  yChanged();

  txtId = 0;
  txtIdChanged();
}

QVector<SignData*> SignData::randomizeAll(QVector<MapDBEntrySign*> mapSigns)
{
  // Prepare return value
  QVector<SignData*> signs;

  // Add blank player sprite, very important for it to be at pos 0
  // Player sprite actually has a lot of data but none of it's used at all by
  // the game
  signs.append(new SignData);

  // Make first pass and get all sprite positions on map
  QVector<TmpSignPos*> tmpPos;

  for(auto entry : mapSigns) {
    auto tmp = new TmpSignPos;
    tmp->x = entry->x;
    tmp->y = entry->y;
    tmpPos.append(tmp);
  }

  // Now create new sprites from data and randomize their contents
  for(auto entry : mapSigns) {

    Q_UNUSED(entry)

    // New Sprite from map data
    auto tmp = new SignData;
    signs.append(tmp);

    // Randomize newly created sprite
    tmp->randomize(&tmpPos);
  }

  return signs;
}

void SignData::randomize(QVector<TmpSignPos*>* tmpPos)
{
  // Randomize coordinates if coord list is provided
  if(tmpPos != nullptr) {

    // Pull random coordinates
    var8 rndPos = Random::rangeExclusive(0, tmpPos->size());
    auto rndCoords = tmpPos->at(rndPos);

    // Change coords
    x = rndCoords->x;
    xChanged();

    y = rndCoords->y;
    yChanged();

    // Remove chosen random coordinate
    delete rndCoords;
    tmpPos->removeAt(rndPos);
  }

  // Do nothing more, if no coordinate list is provided we have nothing else
  // to work off of
}
