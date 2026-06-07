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
 * @brief Transient NPC/control/battle flags for the current map.
 *
 * A bag of runtime flags grouped (in the fields below) into sprites, controls, and
 * battle state -- e.g. whether NPCs face away, the joypad is ignored/simulated, or
 * a trainer wants to battle, plus the trainer-header pointer. Mostly mid-gameplay
 * state. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see Area.
 */
class SAVEFILE_AUTOPORT AreaNPC : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool npcsFaceAway MEMBER npcsFaceAway NOTIFY npcsFaceAwayChanged)                  ///< NPCs face away from the player.
  Q_PROPERTY(bool scriptedNPCMovement MEMBER scriptedNPCMovement NOTIFY scriptedNPCMovementChanged) ///< NPC movement is script-driven.
  Q_PROPERTY(bool npcSpriteMovement MEMBER npcSpriteMovement NOTIFY npcSpriteMovementChanged)   ///< NPC sprites are moving.
  Q_PROPERTY(bool tradeCenterSpritesFaced MEMBER tradeCenterSpritesFaced NOTIFY tradeCenterSpritesFacedChanged) ///< Trade-center sprites have faced.
  Q_PROPERTY(bool ignoreJoypad MEMBER ignoreJoypad NOTIFY ignoreJoypadChanged)                 ///< Joypad input is ignored.
  Q_PROPERTY(bool joypadSimulation MEMBER joypadSimulation NOTIFY joypadSimulationChanged)     ///< Joypad input is being simulated.
  Q_PROPERTY(bool runningTestBattle MEMBER runningTestBattle NOTIFY runningTestBattleChanged)  ///< A test battle is running.
  Q_PROPERTY(bool trainerWantsBattle MEMBER trainerWantsBattle NOTIFY trainerWantsBattleChanged) ///< A trainer is initiating battle.
  Q_PROPERTY(int trainerHeaderPtr MEMBER trainerHeaderPtr NOTIFY trainerHeaderPtrChanged)      ///< Pointer to the active trainer header.

public:
  AreaNPC(SaveFile* saveFile = nullptr);
  virtual ~AreaNPC();

  void load(SaveFile* saveFile = nullptr); ///< Expand these flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten these flags to the save.

signals:
  void npcsFaceAwayChanged();
  void scriptedNPCMovementChanged();
  void npcSpriteMovementChanged();
  void tradeCenterSpritesFacedChanged();
  void ignoreJoypadChanged();
  void joypadSimulationChanged();
  void runningTestBattleChanged();
  void trainerWantsBattleChanged();
  void trainerHeaderPtrChanged();

public slots:
  void reset();              ///< Blank the flags.
  void randomize();          ///< Randomize the flags.
  void setTo(MapDBEntry* map); ///< Set from a chosen map's defaults.

public:
  // Sprites
  bool npcsFaceAway;            ///< @see npcsFaceAway property.
  bool scriptedNPCMovement;     ///< @see scriptedNPCMovement property.
  bool npcSpriteMovement;       ///< @see npcSpriteMovement property.
  bool tradeCenterSpritesFaced; ///< @see tradeCenterSpritesFaced property.

  // Controls
  bool ignoreJoypad;            ///< @see ignoreJoypad property.
  bool joypadSimulation;        ///< @see joypadSimulation property.

  // Battle
  bool runningTestBattle;       ///< @see runningTestBattle property.
  bool trainerWantsBattle;      ///< @see trainerWantsBattle property.
  int  trainerHeaderPtr;        ///< @see trainerHeaderPtr property.
};
