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
#ifndef AREANPC_H
#define AREANPC_H

#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapDBEntry;

class SAVEFILE_AUTOPORT AreaNPC : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool npcsFaceAway MEMBER npcsFaceAway NOTIFY npcsFaceAwayChanged)
  Q_PROPERTY(bool scriptedNPCMovement MEMBER scriptedNPCMovement NOTIFY scriptedNPCMovementChanged)
  Q_PROPERTY(bool npcSpriteMovement MEMBER npcSpriteMovement NOTIFY npcSpriteMovementChanged)
  Q_PROPERTY(bool tradeCenterSpritesFaced MEMBER tradeCenterSpritesFaced NOTIFY tradeCenterSpritesFacedChanged)
  Q_PROPERTY(bool ignoreJoypad MEMBER ignoreJoypad NOTIFY ignoreJoypadChanged)
  Q_PROPERTY(bool joypadSimulation MEMBER joypadSimulation NOTIFY joypadSimulationChanged)
  Q_PROPERTY(bool runningTestBattle MEMBER runningTestBattle NOTIFY runningTestBattleChanged)
  Q_PROPERTY(bool trainerWantsBattle MEMBER trainerWantsBattle NOTIFY trainerWantsBattleChanged)
  Q_PROPERTY(int trainerHeaderPtr MEMBER trainerHeaderPtr NOTIFY trainerHeaderPtrChanged)

public:
  AreaNPC(SaveFile* saveFile = nullptr);
  virtual ~AreaNPC();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

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
  void reset();
  void randomize();
  void setTo(MapDBEntry* map);

public:
  // Sprites
  bool npcsFaceAway;
  bool scriptedNPCMovement;
  bool npcSpriteMovement;
  bool tradeCenterSpritesFaced;

  // Controls
  bool ignoreJoypad;
  bool joypadSimulation;

  // Battle
  bool runningTestBattle;
  bool trainerWantsBattle;
  int  trainerHeaderPtr;
};

#endif // AREANPC_H
