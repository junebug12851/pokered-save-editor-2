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

#include "./worldother.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../../random.h"

WorldOther::WorldOther(SaveFile* saveFile)
{
  load(saveFile);
}

WorldOther::~WorldOther() {}

void WorldOther::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  fossilItemGiven = toolset->getByte(0x29BB);
  fossilPkmnResult = toolset->getByte(0x29BC);
  debugMode = toolset->getBit(0x29DE, 1, 1);


  auto it = saveFile->iterator()->offsetTo(0x2CED);
  playtime.hours = it->getByte();
  playtime.clockMaxed = it->getByte();
  playtime.minutes = it->getByte();
  playtime.seconds = it->getByte();
  playtime.frames = it->getByte();
  delete it;
}

void WorldOther::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

   toolset->setByte(0x29BB, fossilItemGiven);
   toolset->setByte(0x29BC, fossilPkmnResult);
   toolset->setBit(0x29DE, 1, 1, debugMode);

  auto it = saveFile->iterator()->offsetTo(0x2CED);
  it->setByte(playtime.hours);
  it->setByte(playtime.clockMaxed);
  it->setByte(playtime.minutes);
  it->setByte(playtime.seconds);
  it->setByte(playtime.frames);
  delete it;
}

void WorldOther::reset()
{
  debugMode = false;
  playtime.hours = 0;
  playtime.minutes = 0;
  playtime.seconds = 0;
  playtime.frames = 0;
  playtime.clockMaxed = 0;
  fossilItemGiven = 0;
  fossilPkmnResult = 0;
}

void WorldOther::randomize()
{
  reset();

  // 5% chance of being enabled
  debugMode = Random::chanceSuccess(5);

  playtime.hours = Random::rangeInclusive(0, 255);
  playtime.minutes = Random::rangeInclusive(0, 59);
  playtime.seconds = Random::rangeInclusive(0, 59);
  playtime.frames = Random::rangeInclusive(0, 59);
}
