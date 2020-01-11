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
#include "./areapuzzle.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

AreaPuzzle::AreaPuzzle(SaveFile* saveFile)
{
  load(saveFile);
}

AreaPuzzle::~AreaPuzzle() {}

void AreaPuzzle::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  lock1 = toolset->getByte(0x29EF);
  lock2 = toolset->getByte(0x29F0);
  quizOpp = toolset->getByte(0x2CE4);
}

void AreaPuzzle::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x29EF, lock1);
  toolset->setByte(0x29F0, lock2);
  toolset->setByte(0x2CE4, quizOpp);
}

void AreaPuzzle::reset()
{
  lock1 = 0;
  lock2 = 0;
  quizOpp = 0;
}

// Don't know the range to randomize
void AreaPuzzle::randomize() {}
