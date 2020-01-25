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

#include <QObject>
#include "../../../../common/types.h"

class SaveFile;

struct PlayerDir : public QObject
{
  Q_OBJECT
  Q_ENUMS(PlayerDir_)

public:
  enum PlayerDir_ : var8 {
    None = 0,
    Right,
    Left,
    Down,
    Up
  };
};

class AreaPlayer : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 playerMoveDir_ MEMBER playerMoveDir NOTIFY playerMoveDirChanged)
  Q_PROPERTY(var8 playerLastStopDir_ MEMBER playerLastStopDir NOTIFY playerLastStopDirChanged)
  Q_PROPERTY(var8 playerCurDir_ MEMBER playerCurDir NOTIFY playerCurDirChanged)
  Q_PROPERTY(var8 yCoord_ MEMBER yCoord NOTIFY yCoordChanged)
  Q_PROPERTY(var8 xCoord_ MEMBER xCoord NOTIFY xCoordChanged)
  Q_PROPERTY(var8 yBlockCoord_ MEMBER yBlockCoord NOTIFY yBlockCoordChanged)
  Q_PROPERTY(var8 xBlockCoord_ MEMBER xBlockCoord NOTIFY xBlockCoordChanged)
  Q_PROPERTY(var8 playerJumpingYScrnCoords_ MEMBER playerJumpingYScrnCoords NOTIFY playerJumpingYScrnCoordsChanged)
  Q_PROPERTY(bool safariGameOver_ MEMBER safariGameOver NOTIFY safariGameOverChanged)
  Q_PROPERTY(var8 safariBallCount_ MEMBER safariBallCount NOTIFY safariBallCountChanged)
  Q_PROPERTY(var16 safariSteps_ MEMBER safariSteps NOTIFY safariStepsChanged)
  Q_PROPERTY(bool strengthOutsideBattle_ MEMBER strengthOutsideBattle NOTIFY strengthOutsideBattleChanged)
  Q_PROPERTY(bool surfingAllowed_ MEMBER surfingAllowed NOTIFY surfingAllowedChanged)
  Q_PROPERTY(bool flyOutofBattle_ MEMBER flyOutofBattle NOTIFY flyOutofBattleChanged)
  Q_PROPERTY(bool isBattle_ MEMBER isBattle NOTIFY isBattleChanged)
  Q_PROPERTY(bool isTrainerBattle_ MEMBER isTrainerBattle NOTIFY isTrainerBattleChanged)
  Q_PROPERTY(bool noBattles_ MEMBER noBattles NOTIFY noBattlesChanged)
  Q_PROPERTY(bool battleEndedOrBlackout_ MEMBER battleEndedOrBlackout NOTIFY battleEndedOrBlackoutChanged)
  Q_PROPERTY(var8 yOffsetSinceLastSpecialWarp_ MEMBER yOffsetSinceLastSpecialWarp NOTIFY yOffsetSinceLastSpecialWarpChanged)
  Q_PROPERTY(var8 xOffsetSinceLastSpecialWarp_ MEMBER xOffsetSinceLastSpecialWarp NOTIFY xOffsetSinceLastSpecialWarpChanged)
  Q_PROPERTY(bool standingOnDoor_ MEMBER standingOnDoor NOTIFY standingOnDoorChanged)
  Q_PROPERTY(bool movingThroughDoor_ MEMBER movingThroughDoor NOTIFY movingThroughDoorChanged)
  Q_PROPERTY(bool standingOnWarp_ MEMBER standingOnWarp NOTIFY standingOnWarpChanged)
  Q_PROPERTY(var8 walkBikeSurf_ MEMBER walkBikeSurf NOTIFY walkBikeSurfChanged)
  Q_PROPERTY(bool finalLedgeJumping_ MEMBER finalLedgeJumping NOTIFY finalLedgeJumpingChanged)
  Q_PROPERTY(bool spinPlayer_ MEMBER spinPlayer NOTIFY spinPlayerChanged)
  Q_PROPERTY(bool usedCardKey_ MEMBER usedCardKey NOTIFY usedCardKeyChanged)
  Q_PROPERTY(bool usingLinkCable_ MEMBER usingLinkCable NOTIFY usingLinkCableChanged)

public:
  AreaPlayer(SaveFile* saveFile = nullptr);
  virtual ~AreaPlayer();

signals:
  void playerMoveDirChanged();
  void playerLastStopDirChanged();
  void playerCurDirChanged();
  void yCoordChanged();
  void xCoordChanged();
  void yBlockCoordChanged();
  void xBlockCoordChanged();
  void playerJumpingYScrnCoordsChanged();
  void safariGameOverChanged();
  void safariBallCountChanged();
  void safariStepsChanged();
  void strengthOutsideBattleChanged();
  void surfingAllowedChanged();
  void flyOutofBattleChanged();
  void isBattleChanged();
  void isTrainerBattleChanged();
  void noBattlesChanged();
  void battleEndedOrBlackoutChanged();
  void yOffsetSinceLastSpecialWarpChanged();
  void xOffsetSinceLastSpecialWarpChanged();
  void standingOnDoorChanged();
  void movingThroughDoorChanged();
  void standingOnWarpChanged();
  void walkBikeSurfChanged();
  void finalLedgeJumpingChanged();
  void spinPlayerChanged();
  void usedCardKeyChanged();
  void usingLinkCableChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(var8 x, var8 y);

public:
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
