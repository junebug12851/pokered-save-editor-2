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
#include <QRandomGenerator>

#include "./worldcompleted.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

WorldCompleted::WorldCompleted(SaveFile* saveFile)
{
  load(saveFile);
}

WorldCompleted::~WorldCompleted() {}

void WorldCompleted::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  obtainedOldRod = toolset->getBit(0x29D4, 1, 3);
  obtainedGoodRod = toolset->getBit(0x29D4, 1, 4);
  obtainedSuperRod = toolset->getBit(0x29D4, 1, 5);
  satisfiedSaffronGuards = toolset->getBit(0x29D4, 1, 6);
  obtainedLapras = toolset->getBit(0x29DA, 1, 0);
  everHealedPokemon = toolset->getBit(0x29DA, 1, 2);
  obtainedStarterPokemon = toolset->getBit(0x29DA, 1, 3);
  defeatedLorelei = toolset->getBit(0x29E0, 1, 1);
}

void WorldCompleted::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setBit(0x29D4, 1, 3, obtainedOldRod);
  toolset->setBit(0x29D4, 1, 4, obtainedGoodRod);
  toolset->setBit(0x29D4, 1, 5, obtainedSuperRod);
  toolset->setBit(0x29D4, 1, 6, satisfiedSaffronGuards);
  toolset->setBit(0x29DA, 1, 0, obtainedLapras);
  toolset->setBit(0x29DA, 1, 2, everHealedPokemon);
  toolset->setBit(0x29DA, 1, 3, obtainedStarterPokemon);
  toolset->setBit(0x29E0, 1, 1, defeatedLorelei);
}

void WorldCompleted::reset()
{
  obtainedOldRod = false;
  obtainedGoodRod = false;
  obtainedSuperRod = false;
  satisfiedSaffronGuards = false;
  obtainedLapras = false;
  everHealedPokemon = false;
  obtainedStarterPokemon = false;
  defeatedLorelei = false;
}

// For now mark these as false, randomize wants you to play a random game
// from the start so it's a matter of how much will randomize complete for
// you
void WorldCompleted::randomize()
{
  reset();
}
