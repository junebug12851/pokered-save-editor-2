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
#ifndef AREAPLAYER_H
#define AREAPLAYER_H

#include "../../../../common/types.h"

class SaveFile;

enum class PlayerDir : var8 {
  None = 0,
  Right,
  Left,
  Down,
  Up
};

class AreaPlayer
{
public:
  AreaPlayer(SaveFile* saveFile = nullptr);
  virtual ~AreaPlayer();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(var8 x, var8 y);

  // Direction
  // if the player is moving, the current direction
  // if the player is not moving, zero
  // None     0
  // Right    1
  // Left     2
  // Down     4
  // Up       8
  var8 playerMoveDir;

  // the direction in which the player was moving before the player last stopped
  var8 playerLastStopDir;

  // if the player is moving, the current direction
  // if the player is not moving, the last the direction in which the player moved
  var8 playerCurDir;

  // Coords
  var8 yCoord;
  var8 xCoord;
  var8 yBlockCoord;
  var8 xBlockCoord;
  var8 playerJumpingYScrnCoords;

  // Safari
  bool safariGameOver;
  var8 safariBallCount;
  var16 safariSteps;

  // HMs
  bool strengthOutsideBattle;
  bool surfingAllowed;
  bool flyOutofBattle;

  // Battle
  bool isBattle;
  bool isTrainerBattle;
  bool noBattles;
  bool battleEndedOrBlackout;

  // Warps
  var8 yOffsetSinceLastSpecialWarp;
  var8 xOffsetSinceLastSpecialWarp;
  bool standingOnDoor;
  bool movingThroughDoor;
  bool standingOnWarp;

  // Misc

  // 0x00 = walking
  // 0x01 = biking
  // 0x02 = surfing
  var8 walkBikeSurf;
  bool finalLedgeJumping;
  bool spinPlayer;
  bool usedCardKey;
  bool usingLinkCable;
};

#endif // AREAPLAYER_H
