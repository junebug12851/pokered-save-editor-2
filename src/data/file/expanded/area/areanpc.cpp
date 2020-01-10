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

AreaNPC::AreaNPC(SaveFile* saveFile)
{
  load(saveFile);
}

AreaNPC::~AreaNPC() {}

void AreaNPC::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  tradeCenterSpritesFaced = toolset->getBit(0x29D9, 1, 0);
  npcsFaceAway = toolset->getBit(0x29D9, 1, 5);
  scriptedNPCMovement = toolset->getBit(0x29DA, 1, 7);
  npcSpriteMovement = toolset->getBit(0x29DC, 1, 0);
  ignoreJoypad = toolset->getBit(0x29DC, 1, 5);
  joypadSimulation = toolset->getBit(0x29DC, 1, 7);
  runningTestBattle = toolset->getBit(0x29DF, 1, 0);
  trainerWantsBattle = toolset->getBit(0x29DF, 1, 3);
  trainerHeaderPtr = toolset->getWord(0x2CDC, true);
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
  scriptedNPCMovement = false;
  npcSpriteMovement = false;
  tradeCenterSpritesFaced = false;

  // Controls
  ignoreJoypad = false;
  joypadSimulation = false;

  // Battle
  runningTestBattle = false;
  trainerWantsBattle = false;
  trainerHeaderPtr = 0;
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
