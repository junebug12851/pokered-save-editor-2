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
#include <pse-common/types.h>

class SaveFile;
class MapDBEntry;

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

  Q_PROPERTY(int playerMoveDir MEMBER playerMoveDir NOTIFY playerMoveDirChanged)
  Q_PROPERTY(int playerLastStopDir MEMBER playerLastStopDir NOTIFY playerLastStopDirChanged)
  Q_PROPERTY(int playerCurDir MEMBER playerCurDir NOTIFY playerCurDirChanged)
  Q_PROPERTY(int yCoord MEMBER yCoord NOTIFY yCoordChanged)
  Q_PROPERTY(int xCoord MEMBER xCoord NOTIFY xCoordChanged)
  Q_PROPERTY(int yBlockCoord MEMBER yBlockCoord NOTIFY yBlockCoordChanged)
  Q_PROPERTY(int xBlockCoord MEMBER xBlockCoord NOTIFY xBlockCoordChanged)
  Q_PROPERTY(int playerJumpingYScrnCoords MEMBER playerJumpingYScrnCoords NOTIFY playerJumpingYScrnCoordsChanged)
  Q_PROPERTY(bool strengthOutsideBattle MEMBER strengthOutsideBattle NOTIFY strengthOutsideBattleChanged)
  Q_PROPERTY(bool surfingAllowed MEMBER surfingAllowed NOTIFY surfingAllowedChanged)
  Q_PROPERTY(bool flyOutofBattle MEMBER flyOutofBattle NOTIFY flyOutofBattleChanged)
  Q_PROPERTY(bool isBattle MEMBER isBattle NOTIFY isBattleChanged)
  Q_PROPERTY(bool isTrainerBattle MEMBER isTrainerBattle NOTIFY isTrainerBattleChanged)
  Q_PROPERTY(bool noBattles MEMBER noBattles NOTIFY noBattlesChanged)
  Q_PROPERTY(bool battleEndedOrBlackout MEMBER battleEndedOrBlackout NOTIFY battleEndedOrBlackoutChanged)
  Q_PROPERTY(int yOffsetSinceLastSpecialWarp MEMBER yOffsetSinceLastSpecialWarp NOTIFY yOffsetSinceLastSpecialWarpChanged)
  Q_PROPERTY(int xOffsetSinceLastSpecialWarp MEMBER xOffsetSinceLastSpecialWarp NOTIFY xOffsetSinceLastSpecialWarpChanged)
  Q_PROPERTY(bool standingOnDoor MEMBER standingOnDoor NOTIFY standingOnDoorChanged)
  Q_PROPERTY(bool movingThroughDoor MEMBER movingThroughDoor NOTIFY movingThroughDoorChanged)
  Q_PROPERTY(bool standingOnWarp MEMBER standingOnWarp NOTIFY standingOnWarpChanged)
  Q_PROPERTY(int walkBikeSurf MEMBER walkBikeSurf NOTIFY walkBikeSurfChanged)
  Q_PROPERTY(bool finalLedgeJumping MEMBER finalLedgeJumping NOTIFY finalLedgeJumpingChanged)
  Q_PROPERTY(bool spinPlayer MEMBER spinPlayer NOTIFY spinPlayerChanged)
  Q_PROPERTY(bool usedCardKey MEMBER usedCardKey NOTIFY usedCardKeyChanged)
  Q_PROPERTY(bool usingLinkCable MEMBER usingLinkCable NOTIFY usingLinkCableChanged)

public:
  AreaPlayer(SaveFile* saveFile = nullptr);
  virtual ~AreaPlayer();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void playerMoveDirChanged();
  void playerLastStopDirChanged();
  void playerCurDirChanged();
  void yCoordChanged();
  void xCoordChanged();
  void yBlockCoordChanged();
  void xBlockCoordChanged();
  void playerJumpingYScrnCoordsChanged();
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
  void reset();
  void randomize(int x, int y);
  void setTo(MapDBEntry* map, int x, int y);

public:
  // Direction
  // if the player is moving, the current direction
  // if the player is not moving, zero
  // None     0
  // Right    1
  // Left     2
  // Down     4
  // Up       8
  int playerMoveDir;

  // the direction in which the player was moving before the player last stopped
  int playerLastStopDir;

  // if the player is moving, the current direction
  // if the player is not moving, the last the direction in which the player moved
  int playerCurDir;

  // Coords
  int yCoord;
  int xCoord;
  int yBlockCoord;
  int xBlockCoord;
  int playerJumpingYScrnCoords;

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
  int yOffsetSinceLastSpecialWarp;
  int xOffsetSinceLastSpecialWarp;
  bool standingOnDoor;
  bool movingThroughDoor;
  bool standingOnWarp;

  // Misc

  // 0x00 = walking
  // 0x01 = biking
  // 0x02 = surfing
  int walkBikeSurf;
  bool finalLedgeJumping;
  bool spinPlayer;
  bool usedCardKey;
  bool usingLinkCable;
};

#endif // AREAPLAYER_H
