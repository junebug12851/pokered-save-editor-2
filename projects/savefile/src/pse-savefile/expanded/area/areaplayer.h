/*
  * Copyright 2020 Twilight
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
#pragma once
#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapDBEntry;

/**
 * @brief Player facing/movement directions, QML-visible.
 * @note These enum values differ from the raw `playerMoveDir` bit values listed
 *       on that field (None/Right/Left/Down/Up = 0..4 here).
 */
struct SAVEFILE_AUTOPORT PlayerDir : public QObject
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

/**
 * @brief The player's position and movement/HM/battle/warp state on the map.
 *
 * The richest area child: tile/block coordinates, facing directions, HM-usability
 * flags (strength/surf/fly), battle state, warp state, and a grab-bag of misc
 * flags. Many fields are live mid-step gameplay state. The field comments below
 * give the raw value meanings. randomize()/setTo() place the player at (x,y).
 *
 * @see Area, PlayerDir.
 */
class SAVEFILE_AUTOPORT AreaPlayer : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int playerMoveDir MEMBER playerMoveDir NOTIFY playerMoveDirChanged)             ///< Current move direction (raw bits; see field note).
  Q_PROPERTY(int playerLastStopDir MEMBER playerLastStopDir NOTIFY playerLastStopDirChanged) ///< Direction before the last stop.
  Q_PROPERTY(int playerCurDir MEMBER playerCurDir NOTIFY playerCurDirChanged)               ///< Current/last-moved direction.
  Q_PROPERTY(int yCoord MEMBER yCoord NOTIFY yCoordChanged)                                 ///< Tile Y.
  Q_PROPERTY(int xCoord MEMBER xCoord NOTIFY xCoordChanged)                                 ///< Tile X.
  Q_PROPERTY(int yBlockCoord MEMBER yBlockCoord NOTIFY yBlockCoordChanged)                  ///< Block Y.
  Q_PROPERTY(int xBlockCoord MEMBER xBlockCoord NOTIFY xBlockCoordChanged)                  ///< Block X.
  Q_PROPERTY(int playerJumpingYScrnCoords MEMBER playerJumpingYScrnCoords NOTIFY playerJumpingYScrnCoordsChanged) ///< Ledge-jump screen Y.
  Q_PROPERTY(bool strengthOutsideBattle MEMBER strengthOutsideBattle NOTIFY strengthOutsideBattleChanged) ///< Strength active outside battle.
  Q_PROPERTY(bool surfingAllowed MEMBER surfingAllowed NOTIFY surfingAllowedChanged)        ///< Surf usable here.
  Q_PROPERTY(bool flyOutofBattle MEMBER flyOutofBattle NOTIFY flyOutofBattleChanged)        ///< Fly usable here.
  Q_PROPERTY(bool isBattle MEMBER isBattle NOTIFY isBattleChanged)                          ///< In a battle.
  Q_PROPERTY(bool isTrainerBattle MEMBER isTrainerBattle NOTIFY isTrainerBattleChanged)     ///< In a trainer battle.
  Q_PROPERTY(bool noBattles MEMBER noBattles NOTIFY noBattlesChanged)                       ///< Battles suppressed.
  Q_PROPERTY(bool battleEndedOrBlackout MEMBER battleEndedOrBlackout NOTIFY battleEndedOrBlackoutChanged) ///< Battle just ended / blackout.
  Q_PROPERTY(int yOffsetSinceLastSpecialWarp MEMBER yOffsetSinceLastSpecialWarp NOTIFY yOffsetSinceLastSpecialWarpChanged) ///< Y offset since last special warp.
  Q_PROPERTY(int xOffsetSinceLastSpecialWarp MEMBER xOffsetSinceLastSpecialWarp NOTIFY xOffsetSinceLastSpecialWarpChanged) ///< X offset since last special warp.
  Q_PROPERTY(bool standingOnDoor MEMBER standingOnDoor NOTIFY standingOnDoorChanged)        ///< Standing on a door tile.
  Q_PROPERTY(bool movingThroughDoor MEMBER movingThroughDoor NOTIFY movingThroughDoorChanged) ///< Walking through a door.
  Q_PROPERTY(bool standingOnWarp MEMBER standingOnWarp NOTIFY standingOnWarpChanged)        ///< Standing on a warp tile.
  Q_PROPERTY(int walkBikeSurf MEMBER walkBikeSurf NOTIFY walkBikeSurfChanged)               ///< Movement mode (see field note: 0 walk/1 bike/2 surf).
  Q_PROPERTY(bool finalLedgeJumping MEMBER finalLedgeJumping NOTIFY finalLedgeJumpingChanged) ///< Mid final ledge jump.
  Q_PROPERTY(bool spinPlayer MEMBER spinPlayer NOTIFY spinPlayerChanged)                    ///< Spin-tile movement active.
  Q_PROPERTY(bool usedCardKey MEMBER usedCardKey NOTIFY usedCardKeyChanged)                 ///< Card key just used.
  Q_PROPERTY(bool usingLinkCable MEMBER usingLinkCable NOTIFY usingLinkCableChanged)        ///< Link cable in use.

public:
  AreaPlayer(SaveFile* saveFile = nullptr);
  virtual ~AreaPlayer();

  void load(SaveFile* saveFile = nullptr); ///< Expand the player state from the save.
  void save(SaveFile* saveFile);           ///< Flatten the player state to the save.

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
  void reset();                          ///< Blank the player state.
  void randomize(int x, int y);          ///< Randomize, placing the player at (x,y).
  void setTo(MapDBEntry* map, int x, int y); ///< Place the player on @p map at (x,y).

public:
  // Direction
  // if the player is moving, the current direction
  // if the player is not moving, zero
  // None     0
  // Right    1
  // Left     2
  // Down     4
  // Up       8
  int playerMoveDir; ///< Move direction (raw bits per the table above).

  /// the direction in which the player was moving before the player last stopped
  int playerLastStopDir;

  /// if the player is moving, the current direction
  /// if the player is not moving, the last the direction in which the player moved
  int playerCurDir;

  // Coords
  int yCoord;                  ///< @see yCoord property.
  int xCoord;                  ///< @see xCoord property.
  int yBlockCoord;             ///< @see yBlockCoord property.
  int xBlockCoord;             ///< @see xBlockCoord property.
  int playerJumpingYScrnCoords; ///< @see playerJumpingYScrnCoords property.

  // HMs
  bool strengthOutsideBattle;  ///< @see strengthOutsideBattle property.
  bool surfingAllowed;         ///< @see surfingAllowed property.
  bool flyOutofBattle;         ///< @see flyOutofBattle property.

  // Battle
  bool isBattle;               ///< @see isBattle property.
  bool isTrainerBattle;        ///< @see isTrainerBattle property.
  bool noBattles;              ///< @see noBattles property.
  bool battleEndedOrBlackout;  ///< @see battleEndedOrBlackout property.

  // Warps
  int yOffsetSinceLastSpecialWarp; ///< @see yOffsetSinceLastSpecialWarp property.
  int xOffsetSinceLastSpecialWarp; ///< @see xOffsetSinceLastSpecialWarp property.
  bool standingOnDoor;         ///< @see standingOnDoor property.
  bool movingThroughDoor;      ///< @see movingThroughDoor property.
  bool standingOnWarp;         ///< @see standingOnWarp property.

  // Misc

  // 0x00 = walking
  // 0x01 = biking
  // 0x02 = surfing
  int walkBikeSurf;            ///< Movement mode: 0x00 walking, 0x01 biking, 0x02 surfing.
  bool finalLedgeJumping;      ///< @see finalLedgeJumping property.
  bool spinPlayer;             ///< @see spinPlayer property.
  bool usedCardKey;            ///< @see usedCardKey property.
  bool usingLinkCable;         ///< @see usingLinkCable property.
};
