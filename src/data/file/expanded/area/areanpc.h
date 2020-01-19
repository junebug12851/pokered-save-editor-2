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

#include "../../../../common/types.h"

class SaveFile;

class AreaNPC
{
public:
  AreaNPC(SaveFile* saveFile = nullptr);
  virtual ~AreaNPC();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

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
  var16  trainerHeaderPtr;
};

#endif // AREANPC_H
