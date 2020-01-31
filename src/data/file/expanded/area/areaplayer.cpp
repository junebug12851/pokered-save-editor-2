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
#include "../../../../common/random.h"

AreaPlayer::AreaPlayer(SaveFile* saveFile)
{
  load(saveFile);
}

AreaPlayer::~AreaPlayer() {}

void AreaPlayer::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  yCoord = toolset->getByte(0x260D);
  yCoordChanged();

  xCoord = toolset->getByte(0x260E);
  xCoordChanged();

  yBlockCoord = toolset->getByte(0x260F);
  yBlockCoordChanged();

  xBlockCoord = toolset->getByte(0x2610);
  xBlockCoordChanged();

  yOffsetSinceLastSpecialWarp = toolset->getByte(0x278E);
  yOffsetSinceLastSpecialWarpChanged();

  xOffsetSinceLastSpecialWarp = toolset->getByte(0x278F);
  xOffsetSinceLastSpecialWarpChanged();

  playerMoveDir = toolset->getByte(0x27D4);
  playerMoveDirChanged();

  playerLastStopDir = toolset->getByte(0x27D5);
  playerLastStopDirChanged();

  playerCurDir = toolset->getByte(0x27D6);
  playerCurDirChanged();

  walkBikeSurf = toolset->getByte(0x29AC);
  walkBikeSurfChanged();

  safariSteps = toolset->getWord(0x29B9);
  safariStepsChanged();

  playerJumpingYScrnCoords = toolset->getByte(0x29C0);
  playerJumpingYScrnCoordsChanged();

  strengthOutsideBattle = toolset->getBit(0x29D4, 1, 0);
  strengthOutsideBattleChanged();

  surfingAllowed = toolset->getBit(0x29D4, 1, 1);
  surfingAllowedChanged();

  usedCardKey = toolset->getBit(0x29D4, 1, 7);
  usedCardKeyChanged();

  isBattle = toolset->getBit(0x29D9, 1, 6);
  isBattleChanged();

  isTrainerBattle = toolset->getBit(0x29D9, 1, 7);
  isTrainerBattleChanged();

  noBattles = toolset->getBit(0x29DA, 1, 4);
  noBattlesChanged();

  battleEndedOrBlackout = toolset->getBit(0x29DA, 1, 5);
  battleEndedOrBlackoutChanged();

  usingLinkCable = toolset->getBit(0x29DA, 1, 6);
  usingLinkCableChanged();

  flyOutofBattle = toolset->getBit(0x29DF, 1, 7);
  flyOutofBattleChanged();

  standingOnDoor = toolset->getBit(0x29E2, 1, 0);
  standingOnDoorChanged();

  movingThroughDoor = toolset->getBit(0x29E2, 1, 1);
  movingThroughDoorChanged();

  standingOnWarp = toolset->getBit(0x29E2, 1, 2);
  standingOnWarpChanged();

  finalLedgeJumping = toolset->getBit(0x29E2, 1, 6);
  finalLedgeJumpingChanged();

  spinPlayer = toolset->getBit(0x29E2, 1, 7);
  spinPlayerChanged();

  safariGameOver = toolset->getBit(0x2CF2, 1, 0);
  safariGameOverChanged();

  safariBallCount = toolset->getByte(0x2CF3);
  safariBallCountChanged();
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
  playerMoveDirChanged();

  playerLastStopDir = 0;
  playerLastStopDirChanged();

  playerCurDir = 0;
  playerCurDirChanged();

  yCoord = 0;
  yCoordChanged();

  xCoord = 0;
  xCoordChanged();

  yBlockCoord = 0;
  yBlockCoordChanged();

  xBlockCoord = 0;
  xBlockCoordChanged();

  playerJumpingYScrnCoords = 0;
  playerJumpingYScrnCoordsChanged();

  safariGameOver = false;
  safariGameOverChanged();

  safariBallCount = 0;
  safariBallCountChanged();

  safariSteps = 0;
  safariStepsChanged();

  strengthOutsideBattle = false;
  strengthOutsideBattleChanged();

  surfingAllowed = false;
  surfingAllowedChanged();

  flyOutofBattle = false;
  flyOutofBattleChanged();

  isBattle = false;
  isBattleChanged();

  isTrainerBattle = false;
  isTrainerBattleChanged();

  noBattles = false;
  noBattlesChanged();

  battleEndedOrBlackout = false;
  battleEndedOrBlackoutChanged();

  yOffsetSinceLastSpecialWarp = 0;
  yOffsetSinceLastSpecialWarpChanged();

  xOffsetSinceLastSpecialWarp = 0;
  xOffsetSinceLastSpecialWarpChanged();

  standingOnDoor = false;
  standingOnDoorChanged();

  movingThroughDoor = false;
  movingThroughDoorChanged();

  standingOnWarp = false;
  standingOnWarpChanged();

  walkBikeSurf = 0;
  walkBikeSurfChanged();

  finalLedgeJumping = false;
  finalLedgeJumpingChanged();

  spinPlayer = false;
  spinPlayerChanged();

  usedCardKey = false;
  usedCardKeyChanged();

  usingLinkCable = false;
  usingLinkCableChanged();
}

void AreaPlayer::randomize(int x, int y)
{
  // Keep most of them resetted
  reset();

  // Have player be stationary
  playerMoveDir = (var8)PlayerDir::None;
  playerMoveDirChanged();

  // Determine facing and last stop directions
  playerLastStopDir = Random::rangeInclusive(
        (var8)PlayerDir::Right,
        (var8)PlayerDir::Up);
  playerLastStopDirChanged();

  playerCurDir = Random::rangeInclusive(
        (var8)PlayerDir::Right,
        (var8)PlayerDir::Up);
  playerCurDirChanged();

  yCoord = y;
  yCoordChanged();

  xCoord = x;
  xCoordChanged();

  yBlockCoord = (y % 2) ? 0 : 1;
  yBlockCoordChanged();

  xBlockCoord = (x % 2) ? 0 : 1;
  xBlockCoordChanged();
}
