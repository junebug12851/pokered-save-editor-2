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
#include "warpdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"

#include <QRandomGenerator>

WarpData::WarpData(SaveFile* saveFile, var8 index)
{
  load(saveFile, index);
}

WarpData::~WarpData() {}

void WarpData::load(SaveFile* saveFile, var8 index)
{
  if(saveFile == nullptr)
    return reset();

  auto it = saveFile->iterator()->offsetTo((0x4 * index) + 0x265B);

  y = it->getByte();
  x = it->getByte();
  destWarp = it->getByte();
  destMap = it->getByte();

  delete it;
}

void WarpData::save(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x4 * index) + 0x265B);

  it->setByte(y);
  it->setByte(x);
  it->setByte(destWarp);
  it->setByte(destMap);

  delete it;
}

void WarpData::reset()
{
  y = 0;
  x = 0;
  destWarp = 0;
  destMap = 0;
}

// Can't really do this without more information
// TODO Add more information
void WarpData::randomize() {}

MapDBEntry* WarpData::toMap()
{
  return MapsDB::ind.value(QString::number(destMap), nullptr);
}

// Unused
void WarpData::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void WarpData::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}
