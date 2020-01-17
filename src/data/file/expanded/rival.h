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
#ifndef RIVAL_H
#define RIVAL_H

#include <QString>
#include "./expandedinterface.h"
#include "../../../common/types.h"
class SaveFile;

class Rival : ExpandedInterface
{
public:
  Rival(SaveFile* saveFile = nullptr);
  virtual ~Rival();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  // Rival's Name and Starter Pokemon
  // This essentially selects his team that he uses to battle you, it goes
  // by internal Pokemon index and only 3 options are valid, Charmander,
  // Bulbasaur, and Squirtle. I have no idea what will happen if you put a
  // different value in here.
  QString name;
  var8 starter;
};

#endif // RIVAL_H
