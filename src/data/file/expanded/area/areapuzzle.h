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
#ifndef AREAPUZZLE_H
#define AREAPUZZLE_H

#include "../expandedinterface.h"
#include "../../../../common/types.h"

class SaveFile;

class AreaPuzzle : ExpandedInterface
{
public:
  AreaPuzzle(SaveFile* saveFile = nullptr);
  virtual ~AreaPuzzle();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  // Lt. Surge Trash Can Locks
  var8 lock1;
  var8 lock2;

  // Cinnabar Gym Next Opp
  var8 quizOpp;
};

#endif // AREAPUZZLE_H
