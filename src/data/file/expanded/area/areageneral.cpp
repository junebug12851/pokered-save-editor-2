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

#include <QRandomGenerator>


AreaGeneral::AreaGeneral(SaveFile* saveFile)
{
  load(saveFile);
}

AreaGeneral::~AreaGeneral() {}

void AreaGeneral::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset =saveFile->toolset;

  contrast = toolset->getByte(0x2609);
  noLetterDelay = toolset->getBit(0x29DC, 1, 6);
  countPlaytime = toolset->getBit(0x29DE, 1, 0);
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
  noLetterDelay = false;
  countPlaytime = false;
}

void AreaGeneral::randomize()
{
  auto rnd = QRandomGenerator::global();

  // Pick a number between 1 - 8
  // That's beacuse 9 is solid black which can be fun but not very playable lol
  contrast = rnd->bounded(0, 9);

  // Leaving these options off for now
  noLetterDelay = false;
  countPlaytime = false;
}
