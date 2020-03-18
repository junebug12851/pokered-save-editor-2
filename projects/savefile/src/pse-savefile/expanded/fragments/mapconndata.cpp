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
#include "mapconndata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>

#include <QRandomGenerator>

MapConnData::MapConnData(SaveFile* saveFile, var16 offset)
{
  load(saveFile, offset);
}

MapConnData::~MapConnData() {}

void MapConnData::load(SaveFile* saveFile, var16 offset)
{
  reset();

  if(saveFile == nullptr) {
    return;
  }

  auto it = saveFile->iterator()->offsetTo(offset);

  mapPtr = it->getByte();
  mapPtrChanged();

  stripSrc = it->getWord(0, true);
  stripSrcChanged();

  stripDst = it->getWord(0, true);
  stripDstChanged();

  stripWidth = it->getByte();
  stripWidthChanged();

  width = it->getByte();
  widthChanged();

  yAlign = it->getByte();
  yAlignChanged();

  xAlign = it->getByte();
  xAlignChanged();

  viewPtr = it->getWord(0, true);
  viewPtrChanged();

  delete it;
}

void MapConnData::save(SaveFile* saveFile, var16 offset)
{
  auto it = saveFile->iterator()->offsetTo(offset);

  it->setByte(mapPtr);
  it->setWord(stripSrc, 0, true);
  it->setWord(stripDst, 0, true);
  it->setByte(stripWidth);
  it->setByte(width);
  it->setByte(yAlign);
  it->setByte(xAlign);
  it->setWord(viewPtr, 0, true);

  delete it;
}

void MapConnData::reset()
{
  mapPtr = 0;
  mapPtrChanged();

  stripSrc = 0;
  stripSrcChanged();

  stripDst = 0;
  stripDstChanged();

  stripWidth = 0;
  stripWidthChanged();

  width = 0;
  widthChanged();

  yAlign = 0;
  yAlignChanged();

  xAlign = 0;
  xAlignChanged();

  viewPtr = 0;
  viewPtrChanged();
}

void MapConnData::loadFromData(MapDBEntryConnect* connect) {
  width = *connect->toMap->width;
  widthChanged();

  mapPtr = connect->toMap->ind;
  mapPtrChanged();

  stripSrc = connect->stripLocation();
  stripSrcChanged();

  stripDst = connect->mapPos();
  stripDstChanged();

  stripWidth = connect->stripSize();
  stripWidthChanged();

  yAlign = connect->yAlign();
  yAlignChanged();

  xAlign = connect->xAlign();
  xAlignChanged();

  viewPtr = connect->window();
  viewPtrChanged();
}

MapDBEntry* MapConnData::toMap()
{
  return MapsDB::ind.value(QString::number(mapPtr), nullptr);
}
