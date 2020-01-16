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
#ifndef WORLDOTHER_H
#define WORLDOTHER_H

#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;

// Counts playtime up to
// 10 days, 15 hours, 59 minutes, 59 seconds, and 59 frames
struct Playtime {
  var8 hours; // Max 255
  var8 minutes; // Max 59, any higher will reset to zero and increment hr by 1
  var8 seconds; // Max 59, any higher will reset to zero and increment min by 1
  var8 frames; // Max 59, any higher will reset to zero and increment sec by 1

  // Any value, it just stops the clock completely
  var8 clockMaxed;
};

class WorldOther : ExpandedInterface
{
public:
  WorldOther(SaveFile* saveFile = nullptr);
  virtual ~WorldOther();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  // Hold B to avoid wild battles
  bool debugMode;

  // Playtime
  Playtime playtime;

  // Fossils
  var8 fossilItemGiven;
  var8 fossilPkmnResult;
};

#endif // WORLDOTHER_H
