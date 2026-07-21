/*
  * Copyright 2020 Fairy Fox
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
 * @file areanpc.cpp
 * @brief Implementation of AreaNPC -- the map-global character-state flags.
 *        See areanpc.h for the documented API and
 *        notes/reference/npc-character-state.md for the console-verified persistence.
 *
 * The offsets/bits below are unchanged from v1; only the names were corrected (2026-07-15),
 * so save output stays byte-identical.
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

  // Sprites -- wStatusFlags3 (0x29D9)
  initTradeCenterFacing = toolset->getBit(0x29D9, 1, 0);  // BIT_INIT_TRADE_CENTER_FACING
  initTradeCenterFacingChanged();

  npcsDoNotFacePlayer = toolset->getBit(0x29D9, 1, 5);    // BIT_NO_NPC_FACE_PLAYER
  npcsDoNotFacePlayerChanged();

  // Controls -- wStatusFlags4 b7 + wStatusFlags5 b0/b5/b7
  initScriptedMovement = toolset->getBit(0x29DA, 1, 7);   // BIT_INIT_SCRIPTED_MOVEMENT (wStatusFlags4)
  initScriptedMovementChanged();

  scriptedNpcMoving = toolset->getBit(0x29DC, 1, 0);      // BIT_SCRIPTED_NPC_MOVEMENT (wStatusFlags5)
  scriptedNpcMovingChanged();

  disableJoypad = toolset->getBit(0x29DC, 1, 5);          // BIT_DISABLE_JOYPAD (wStatusFlags5)
  disableJoypadChanged();

  scriptedMovementActive = toolset->getBit(0x29DC, 1, 7); // BIT_SCRIPTED_MOVEMENT_STATE (wStatusFlags5)
  scriptedMovementActiveChanged();

  // Battle -- wStatusFlags7 b0/b3 + wTrainerHeaderPtr
  testBattle = toolset->getBit(0x29DF, 1, 0);             // BIT_TEST_BATTLE (wStatusFlags7)
  testBattleChanged();

  trainerBattle = toolset->getBit(0x29DF, 1, 3);          // BIT_TRAINER_BATTLE (wStatusFlags7)
  trainerBattleChanged();

  trainerHeaderPtr = toolset->getWord(0x2CDC, true);      // wTrainerHeaderPtr
  trainerHeaderPtrChanged();
}

void AreaNPC::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setBit(0x29D9, 1, 0, initTradeCenterFacing);
  toolset->setBit(0x29D9, 1, 5, npcsDoNotFacePlayer);
  toolset->setBit(0x29DA, 1, 7, initScriptedMovement);
  toolset->setBit(0x29DC, 1, 0, scriptedNpcMoving);
  toolset->setBit(0x29DC, 1, 5, disableJoypad);
  toolset->setBit(0x29DC, 1, 7, scriptedMovementActive);
  toolset->setBit(0x29DF, 1, 0, testBattle);
  toolset->setBit(0x29DF, 1, 3, trainerBattle);
  toolset->setWord(0x2CDC, trainerHeaderPtr, true);
}

void AreaNPC::reset()
{
  // Sprites
  npcsDoNotFacePlayer = false;
  npcsDoNotFacePlayerChanged();

  initTradeCenterFacing = false;
  initTradeCenterFacingChanged();

  // Controls
  initScriptedMovement = false;
  initScriptedMovementChanged();

  scriptedNpcMoving = false;
  scriptedNpcMovingChanged();

  disableJoypad = false;
  disableJoypadChanged();

  scriptedMovementActive = false;
  scriptedMovementActiveChanged();

  // Battle
  testBattle = false;
  testBattleChanged();

  trainerBattle = false;
  trainerBattleChanged();

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
