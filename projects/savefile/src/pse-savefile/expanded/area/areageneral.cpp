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
#include "areageneral.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-common/random.h>
#include <pse-db/mapsdb.h>

AreaGeneral::AreaGeneral(SaveFile* saveFile)
{
  load(saveFile);
}

AreaGeneral::~AreaGeneral() {}

void AreaGeneral::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset =saveFile->toolset;

  contrast = toolset->getByte(0x2609);
  contrastChanged();

  noLetterDelay = toolset->getBit(0x29DC, 1, 6);
  noLetterDelayChanged();

  countPlaytime = toolset->getBit(0x29DE, 1, 0);
  countPlaytimeChanged();
}

void AreaGeneral::save(SaveFile* saveFile)
{
  auto toolset =saveFile->toolset;

  toolset->setByte(0x2609, contrast);
  toolset->setBit(0x29DC, 1, 6, noLetterDelay);
  toolset->setBit(0x29DE, 1, 0, countPlaytime);
}

void AreaGeneral::reset()
{
  contrast = 0;
  contrastChanged();

  noLetterDelay = false;
  noLetterDelayChanged();

  countPlaytime = false;
  countPlaytimeChanged();
}

void AreaGeneral::randomize()
{
  reset();

  // Pick a number between 1 - 8
  // That's beacuse 9 is solid black which can be fun but not very playable lol
  contrast = Random::rangeInclusive(0, 8);
  contrastChanged();

  // Leaving these options off for now
  noLetterDelay = false;
  noLetterDelayChanged();

  countPlaytime = false;
  countPlaytimeChanged();
}

void AreaGeneral::setTo(MapDBEntry* map)
{
  reset();

  int mapInd = (map == nullptr)
      ? 0
      : map->ind;

  // Set "Needs Flash" contrast level for Rock Tunnel 1 and 2, otherwise normal
  // no flash needed
  contrast = (mapInd == 82 || mapInd == 232)
      ? 6
      : 0;
  contrastChanged();

  // Leaving these options off for now
  noLetterDelay = false;
  noLetterDelayChanged();

  countPlaytime = false;
  countPlaytimeChanged();
}
