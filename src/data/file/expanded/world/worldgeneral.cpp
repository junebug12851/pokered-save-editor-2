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

#include "./worldgeneral.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../db/mapsearch.h"
#include "../../../../random.h"

WorldGeneral::WorldGeneral(SaveFile* saveFile)
{
  load(saveFile);
}

WorldGeneral::~WorldGeneral() {}

void WorldGeneral::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Bits 0-3 [max 15]
  options.textSlowness = toolset->getByte(0x2601) & 0b00001111;
  options.textSlownessChanged();

  options.battleStyleSet = toolset->getBit(0x2601, 1, 6);
  options.battleStyleSetChanged();

  options.battleAnimOff = toolset->getBit(0x2601, 1, 7);
  options.battleAnimOffChanged();

  letterDelay.normalDelay = toolset->getBit(0x2604, 1, 0);
  letterDelay.normalDelayChanged();

  letterDelay.dontDelay = toolset->getBit(0x2604, 1, 1);
  letterDelay.dontDelayChanged();

  lastBlackoutMap = toolset->getByte(0x29C5);
  lastBlackoutMapChanged();

  lastMap = toolset->getByte(0x2611);
  lastMapChanged();
}

void WorldGeneral::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x2601, options.textSlowness);
  toolset->setBit(0x2601, 1, 6, options.battleStyleSet);
  toolset->setBit(0x2601, 1, 7, options.battleAnimOff);

  toolset->setBit(0x2604, 1, 0, letterDelay.normalDelay);
  toolset->setBit(0x2604, 1, 1, letterDelay.dontDelay);

  toolset->setByte(0x29C5, lastBlackoutMap);
  toolset->setByte(0x2611, lastMap);
}

void WorldGeneral::reset()
{
  lastBlackoutMap = 0;
  lastBlackoutMapChanged();

  lastMap = 0;
  lastMapChanged();

  options.textSlowness = 0;
  options.textSlownessChanged();

  options.battleStyleSet = false;
  options.battleStyleSetChanged();

  options.battleAnimOff = false;
  options.battleAnimOffChanged();

  letterDelay.normalDelay = false;
  letterDelay.normalDelayChanged();

  letterDelay.dontDelay = false;
  letterDelay.dontDelayChanged();
}

void WorldGeneral::randomize()
{
  reset();

  // Blackout map needs to be one of the cities
  lastBlackoutMap = MapsDB::search()->isGood()->isCity()->pickRandom()->ind;
  lastBlackoutMapChanged();

  // Last Map needs to be some outdoor map
  lastMap = MapsDB::search()->isGood()->isType("Outdoor")->pickRandom()->ind;
  lastMapChanged();

  // Odds are your text is going to be pretty slow lol
  options.textSlowness = Random::rangeInclusive(0, 15);
  options.textSlownessChanged();

  // 20% chance to have battle style set and animations off
  options.battleStyleSet = Random::chanceSuccess(20);
  options.battleStyleSetChanged();

  options.battleAnimOff = Random::chanceSuccess(20);
  options.battleAnimOffChanged();

  // 90% chance to have normal letter delay
  letterDelay.normalDelay = Random::chanceSuccess(90);
  letterDelay.normalDelayChanged();

  // 10% chance to not delay letters
  letterDelay.dontDelay = Random::chanceSuccess(10);
  letterDelay.dontDelayChanged();
}
