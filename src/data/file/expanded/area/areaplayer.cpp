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
#include "areaplayer.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

#include <QRandomGenerator>

AreaPlayer::AreaPlayer(SaveFile* saveFile)
{
  load(saveFile);
}

AreaPlayer::~AreaPlayer() {}

void AreaPlayer::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  yCoord = toolset->getByte(0x260D);
  xCoord = toolset->getByte(0x260E);
  yBlockCoord = toolset->getByte(0x260F);
  xBlockCoord = toolset->getByte(0x2610);
  yOffsetSinceLastSpecialWarp = toolset->getByte(0x278E);
  xOffsetSinceLastSpecialWarp = toolset->getByte(0x278F);
  playerMoveDir = toolset->getByte(0x27D4);
  playerLastStopDir = toolset->getByte(0x27D5);
  playerCurDir = toolset->getByte(0x27D6);
  walkBikeSurf = toolset->getByte(0x29AC);
  safariSteps = toolset->getWord(0x29B9);
  playerJumpingYScrnCoords = toolset->getByte(0x29C0);
  strengthOutsideBattle = toolset->getBit(0x29D4, 1, 0);
  surfingAllowed = toolset->getBit(0x29D4, 1, 1);
  usedCardKey = toolset->getBit(0x29D4, 1, 7);
  isBattle = toolset->getBit(0x29D9, 1, 6);
  isTrainerBattle = toolset->getBit(0x29D9, 1, 7);
  noBattles = toolset->getBit(0x29DA, 1, 4);
  battleEndedOrBlackout = toolset->getBit(0x29DA, 1, 5);
  usingLinkCable = toolset->getBit(0x29DA, 1, 6);
  flyOutofBattle = toolset->getBit(0x29DF, 1, 7);
  standingOnDoor = toolset->getBit(0x29E2, 1, 0);
  movingThroughDoor = toolset->getBit(0x29E2, 1, 1);
  standingOnWarp = toolset->getBit(0x29E2, 1, 2);
  finalLedgeJumping = toolset->getBit(0x29E2, 1, 6);
  spinPlayer = toolset->getBit(0x29E2, 1, 7);
  safariGameOver = toolset->getBit(0x2CF2, 1, 0);
  safariBallCount = toolset->getByte(0x2CF3);
}

void AreaPlayer::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x260D, yCoord);
  toolset->setByte(0x260E, xCoord);
  toolset->setByte(0x260F, yBlockCoord);
  toolset->setByte(0x2610, xBlockCoord);
  toolset->setByte(0x278E, yOffsetSinceLastSpecialWarp);
  toolset->setByte(0x278F, xOffsetSinceLastSpecialWarp);
  toolset->setByte(0x27D4, playerMoveDir);
  toolset->setByte(0x27D5, playerLastStopDir);
  toolset->setByte(0x27D6, playerCurDir);
  toolset->setByte(0x29AC, walkBikeSurf);
  toolset->setWord(0x29B9, safariSteps);
  toolset->setByte(0x29C0, playerJumpingYScrnCoords);
  toolset->setBit(0x29D4, 1, 0, strengthOutsideBattle);
  toolset->setBit(0x29D4, 1, 1, surfingAllowed);
  toolset->setBit(0x29D4, 1, 7, usedCardKey);
  toolset->setBit(0x29D9, 1, 6, isBattle);
  toolset->setBit(0x29D9, 1, 7, isTrainerBattle);
  toolset->setBit(0x29DA, 1, 4, noBattles);
  toolset->setBit(0x29DA, 1, 5, battleEndedOrBlackout);
  toolset->setBit(0x29DA, 1, 6, usingLinkCable);
  toolset->setBit(0x29DF, 1, 7, flyOutofBattle);
  toolset->setBit(0x29E2, 1, 0, standingOnDoor);
  toolset->setBit(0x29E2, 1, 1, movingThroughDoor);
  toolset->setBit(0x29E2, 1, 2, standingOnWarp);
  toolset->setBit(0x29E2, 1, 6, finalLedgeJumping);
  toolset->setBit(0x29E2, 1, 7, spinPlayer);
  toolset->setBit(0x2CF2, 1, 0, safariGameOver);
  toolset->setByte(0x2CF3, safariBallCount);
}

void AreaPlayer::reset()
{
  playerMoveDir = 0;
  playerLastStopDir = 0;
  playerCurDir = 0;

  yCoord = 0;
  xCoord = 0;
  yBlockCoord = 0;
  xBlockCoord = 0;
  playerJumpingYScrnCoords = 0;

  safariGameOver = false;
  safariBallCount = 0;
  safariSteps = 0;

  strengthOutsideBattle = false;
  surfingAllowed = false;
  flyOutofBattle = false;

  isBattle = false;
  isTrainerBattle = false;
  noBattles = false;
  battleEndedOrBlackout = false;

  yOffsetSinceLastSpecialWarp = 0;
  xOffsetSinceLastSpecialWarp = 0;
  standingOnDoor = false;
  movingThroughDoor = false;
  standingOnWarp = false;

  walkBikeSurf = 0;
  finalLedgeJumping = false;
  spinPlayer = false;
  usedCardKey = false;
  usingLinkCable = false;
}

void AreaPlayer::randomize(var8 x, var8 y)
{
  // Keep most of them resetted
  reset();

  // Only change certain ones
  auto rnd = QRandomGenerator::global();

  // Have player be stationary
  playerMoveDir = (var8)PlayerDir::None;

  // Determine facing and last stop directions
  playerLastStopDir = rnd->bounded(
        (var8)PlayerDir::Right,
        (var8)PlayerDir::Up);

  playerCurDir = rnd->bounded(
        (var8)PlayerDir::Right,
        (var8)PlayerDir::Up);

  yCoord = y;
  xCoord = x;
  yBlockCoord = (y & 2) ? 0 : 1;
  xBlockCoord = (x & 2) ? 0 : 1;
}

// Not to use
void AreaPlayer::randomize() {}
