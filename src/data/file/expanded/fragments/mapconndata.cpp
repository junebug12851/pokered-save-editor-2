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

#include <QRandomGenerator>

MapConnData::MapConnData(SaveFile* saveFile, var16 offset)
{
  load(saveFile, offset);
}

MapConnData::~MapConnData() {}

void MapConnData::load(SaveFile* saveFile, var16 offset)
{
  if(saveFile == nullptr) {
    reset();
    return;
  }

  auto it = saveFile->iterator()->offsetTo(offset);

  mapPtr = it->getByte();
  stripSrc = it->getWord();
  stripDst = it->getWord();
  stripWidth = it->getByte();
  width = it->getByte();
  yAlign = it->getByte();
  xAlign = it->getByte();
  viewPtr = it->getWord(0, true);
}

void MapConnData::save(SaveFile* saveFile, var16 offset)
{
  auto it = saveFile->iterator()->offsetTo(offset);

  it->setByte(mapPtr);
  it->setWord(stripSrc);
  it->setWord(stripDst);
  it->setByte(stripWidth);
  it->setByte(width);
  it->setByte(yAlign);
  it->setByte(xAlign);
  it->setWord(viewPtr, 0, true);
}

void MapConnData::reset()
{
  mapPtr = 0;
  stripSrc = 0;
  stripDst = 0;
  stripWidth = 0;
  width = 0;
  yAlign = 0;
  xAlign = 0;
  viewPtr = 0;
}

// To produce a valid map, we need coordination with what other map code
// randomize is not in the business to produce corrupted maps but instead
// random maps ^_^
// @TODO complete this
void MapConnData::randomize() {}

// Not to be used
void MapConnData::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void MapConnData::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}