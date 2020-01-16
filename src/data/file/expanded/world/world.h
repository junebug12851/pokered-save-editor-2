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
#ifndef WORLD_H
#define WORLD_H

#include "../expandedinterface.h"
class SaveFile;

class WorldCompleted;
class WorldEvents;
class WorldGeneral;
class WorldHidden;
class WorldMissables;

class World : public ExpandedInterface
{
public:
  World(SaveFile* saveFile = nullptr);
  virtual ~World();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  WorldCompleted* completed = nullptr;
  WorldEvents* events = nullptr;
  WorldGeneral* general = nullptr;
  WorldHidden* hidden = nullptr;
  WorldMissables* missables = nullptr;
};

#endif // WORLD_H