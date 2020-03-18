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
#include "./areanpc.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>

AreaNPC::AreaNPC(SaveFile* saveFile)
{
  load(saveFile);
}

AreaNPC::~AreaNPC() {}

void AreaNPC::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  tradeCenterSpritesFaced = toolset->getBit(0x29D9, 1, 0);
  tradeCenterSpritesFacedChanged();

  npcsFaceAway = toolset->getBit(0x29D9, 1, 5);
  npcsFaceAwayChanged();

  scriptedNPCMovement = toolset->getBit(0x29DA, 1, 7);
  scriptedNPCMovementChanged();

  npcSpriteMovement = toolset->getBit(0x29DC, 1, 0);
  npcSpriteMovementChanged();

  ignoreJoypad = toolset->getBit(0x29DC, 1, 5);
  ignoreJoypadChanged();

  joypadSimulation = toolset->getBit(0x29DC, 1, 7);
  joypadSimulationChanged();

  runningTestBattle = toolset->getBit(0x29DF, 1, 0);
  runningTestBattleChanged();

  trainerWantsBattle = toolset->getBit(0x29DF, 1, 3);
  trainerWantsBattleChanged();

  trainerHeaderPtr = toolset->getWord(0x2CDC, true);
  trainerHeaderPtrChanged();
}

void AreaNPC::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setBit(0x29D9, 1, 0, tradeCenterSpritesFaced);
  toolset->setBit(0x29D9, 1, 5, npcsFaceAway);
  toolset->setBit(0x29DA, 1, 7, scriptedNPCMovement);
  toolset->setBit(0x29DC, 1, 0, npcSpriteMovement);
  toolset->setBit(0x29DC, 1, 5, ignoreJoypad);
  toolset->setBit(0x29DC, 1, 7, joypadSimulation);
  toolset->setBit(0x29DF, 1, 0, runningTestBattle);
  toolset->setBit(0x29DF, 1, 3, trainerWantsBattle);
  toolset->setWord(0x2CDC, trainerHeaderPtr, true);
}

void AreaNPC::reset()
{
  // Sprites
  npcsFaceAway = false;
  npcsFaceAwayChanged();

  scriptedNPCMovement = false;
  scriptedNPCMovementChanged();

  npcSpriteMovement = false;
  npcSpriteMovementChanged();

  tradeCenterSpritesFaced = false;
  tradeCenterSpritesFacedChanged();

  // Controls
  ignoreJoypad = false;
  ignoreJoypadChanged();

  joypadSimulation = false;
  joypadSimulationChanged();

  // Battle
  runningTestBattle = false;
  runningTestBattleChanged();

  trainerWantsBattle = false;
  trainerWantsBattleChanged();

  trainerHeaderPtr = 0;
  trainerHeaderPtrChanged();
}

// Don't do any randomization, I can't think of a single benefit to checking
// any of these randomly apart from potentially crashing the game or other
// quirks that wouldn't be random fun. Random is about creating a random map
// that's fun and sometimes challenging to jump in. If it just crashed the game
// then it wouldn't be used very much
void AreaNPC::randomize()
{
  reset();
}

void AreaNPC::setTo(MapDBEntry* map)
{
  Q_UNUSED(map)
  reset();
}
