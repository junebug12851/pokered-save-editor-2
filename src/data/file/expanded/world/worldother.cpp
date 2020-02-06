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
#include "../../../../common/random.h"

WorldOther::WorldOther(SaveFile* saveFile)
{
  playtime = new Playtime;
  load(saveFile);
}

WorldOther::~WorldOther() {
  delete playtime;
}

void WorldOther::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  fossilItemGiven = toolset->getByte(0x29BB);
  fossilItemGivenChanged();

  fossilPkmnResult = toolset->getByte(0x29BC);
  fossilPkmnResultChanged();

  debugMode = toolset->getBit(0x29DE, 1, 1);
  debugModeChanged();

  auto it = saveFile->iterator()->offsetTo(0x2CED);
  playtime->hours = it->getByte();
  playtime->hoursChanged();

  playtime->clockMaxed = it->getByte();
  playtime->clockMaxedChanged();

  playtime->minutes = it->getByte();
  playtime->minutesChanged();

  playtime->seconds = it->getByte();
  playtime->secondsChanged();

  playtime->frames = it->getByte();
  playtime->framesChanged();
  delete it;
}

void WorldOther::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

   toolset->setByte(0x29BB, fossilItemGiven);
   toolset->setByte(0x29BC, fossilPkmnResult);
   toolset->setBit(0x29DE, 1, 1, debugMode);

  auto it = saveFile->iterator()->offsetTo(0x2CED);
  it->setByte(playtime->hours);
  it->setByte(playtime->clockMaxed);
  it->setByte(playtime->minutes);
  it->setByte(playtime->seconds);
  it->setByte(playtime->frames);
  delete it;
}

void WorldOther::reset()
{
  debugMode = false;
  debugModeChanged();

  playtime->hours = 0;
  playtime->hoursChanged();

  playtime->minutes = 0;
  playtime->minutesChanged();

  playtime->seconds = 0;
  playtime->secondsChanged();

  playtime->frames = 0;
  playtime->framesChanged();

  playtime->clockMaxed = 0;
  playtime->clockMaxedChanged();

  fossilItemGiven = 0;
  fossilItemGivenChanged();

  fossilPkmnResult = 0;
  fossilPkmnResultChanged();
}

void WorldOther::randomize()
{
  reset();

  // 5% chance of being enabled
  debugMode = Random::chanceSuccess(5);
  debugModeChanged();

  randomizePlaytime();
}

void WorldOther::randomizePlaytime()
{
  playtime->hours = Random::rangeInclusive(0, 255);
  playtime->hoursChanged();

  playtime->minutes = Random::rangeInclusive(0, 59);
  playtime->minutesChanged();

  playtime->seconds = Random::rangeInclusive(0, 59);
  playtime->secondsChanged();

  playtime->frames = Random::rangeInclusive(0, 59);
  playtime->framesChanged();
}

int Playtime::getDays()
{
  return hours / 24;
}

void Playtime::setDays(int val)
{
  int _days = (hours / 24) + val;
  int _hours = hours % 24;

  hours = (_days * 24) + _hours;
  hoursChanged();
}

int Playtime::getHoursAdjusted()
{
  return hours % 24;
}
