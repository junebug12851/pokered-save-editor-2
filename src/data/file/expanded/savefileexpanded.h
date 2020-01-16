/*
  * Copyright 2019 June Hanabi
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
#ifndef SAVEFILEEXPANDED_H
#define SAVEFILEEXPANDED_H

#include "./expandedinterface.h"

class SaveFile;
class Player;
class Area;
class World;
class Daycare;
class HallOfFame;

class SaveFileExpanded: public ExpandedInterface
{
public:
  SaveFileExpanded(SaveFile* saveFile = nullptr);
  virtual ~SaveFileExpanded();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  Player* player = nullptr;
  Area* area = nullptr;
  World* world = nullptr;
  Daycare* daycare = nullptr;
  HallOfFame* hof = nullptr;
};

#endif // SAVEFILEEXPANDED_H
