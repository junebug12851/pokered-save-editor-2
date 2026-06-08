/*
  * Copyright 2020 Twilight
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

/**
 * @file worldgeneral.cpp
 * @brief Implementation of WorldGeneral (and the Options / LetterDelay
 *        sub-objects). See worldgeneral.h for the documented API.
 */

#include "./worldgeneral.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/util/mapsearch.h>
#include <pse-common/random.h>

WorldGeneral::WorldGeneral(SaveFile* saveFile)
{
  options = new Options;
  letterDelay = new LetterDelay;
  load(saveFile);
}

WorldGeneral::~WorldGeneral() {
  options->deleteLater();
  letterDelay->deleteLater();
}

void WorldGeneral::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Bits 0-3 [max 15]
  options->textSlowness = toolset->getByte(0x2601) & 0b00001111;
  options->textSlownessChanged();

  options->battleStyleSet = toolset->getBit(0x2601, 1, 6);
  options->battleStyleSetChanged();

  options->battleAnimOff = toolset->getBit(0x2601, 1, 7);
  options->battleAnimOffChanged();

  letterDelay->normalDelay = toolset->getBit(0x2604, 1, 0);
  letterDelay->normalDelayChanged();

  letterDelay->dontDelay = toolset->getBit(0x2604, 1, 1);
  letterDelay->dontDelayChanged();

  lastBlackoutMap = toolset->getByte(0x29C5);
  lastBlackoutMapChanged();

  lastMap = toolset->getByte(0x2611);
  lastMapChanged();
}

void WorldGeneral::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x2601, options->textSlowness);
  toolset->setBit(0x2601, 1, 6, options->battleStyleSet);
  toolset->setBit(0x2601, 1, 7, options->battleAnimOff);

  toolset->setBit(0x2604, 1, 0, letterDelay->normalDelay);
  toolset->setBit(0x2604, 1, 1, letterDelay->dontDelay);

  toolset->setByte(0x29C5, lastBlackoutMap);
  toolset->setByte(0x2611, lastMap);
}

void WorldGeneral::reset()
{
  lastBlackoutMap = 0;
  lastBlackoutMapChanged();

  lastMap = 0;
  lastMapChanged();

  options->textSlowness = 0;
  options->textSlownessChanged();

  options->battleStyleSet = false;
  options->battleStyleSetChanged();

  options->battleAnimOff = false;
  options->battleAnimOffChanged();

  letterDelay->normalDelay = false;
  letterDelay->normalDelayChanged();

  letterDelay->dontDelay = false;
  letterDelay->dontDelayChanged();
}

void WorldGeneral::randomize()
{
  reset();

  // Map-dependent randomization disabled until the map randomizer is set up. These
  // pick random maps via MapsDB search, but the type/city searches can come back
  // empty (e.g. isType("Outdoor") matches 0 maps), so pickRandom() returns null and
  // getInd() crashes. Left at the reset() default (0) so the rest of the world
  // randomizes cleanly. Re-enable alongside the map randomizer. (Twilight-authorised,
  // 2026-06-07.)
  // lastBlackoutMap = MapsDB::inst()->search()->isGood()->isCity()->pickRandom()->getInd();
  // lastBlackoutMapChanged();
  // lastMap = MapsDB::inst()->search()->isGood()->isType("Outdoor")->pickRandom()->getInd();
  // lastMapChanged();

  // Options (text speed / battle style / battle animations / letter delay) and the
  // last/blackout maps above are part of the Options and Maps screens, which aren't
  // set up yet, so we don't randomize them until those screens exist. After reset()
  // they're left at defaults. Re-enable with the Options/Maps screens.
  // (Twilight-authorised, 2026-06-07.)
  // options->textSlowness = Random::inst()->rangeInclusive(0, 15);
  // options->textSlownessChanged();
  // options->battleStyleSet = Random::inst()->chanceSuccess(20);
  // options->battleStyleSetChanged();
  // options->battleAnimOff = Random::inst()->chanceSuccess(20);
  // options->battleAnimOffChanged();
  // letterDelay->normalDelay = Random::inst()->chanceSuccess(90);
  // letterDelay->normalDelayChanged();
  // letterDelay->dontDelay = Random::inst()->chanceSuccess(10);
  // letterDelay->dontDelayChanged();
}
