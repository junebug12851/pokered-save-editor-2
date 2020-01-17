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
#ifndef WORLDTRADES_H
#define WORLDTRADES_H

#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;

constexpr var8 tradeCount = 10;
constexpr var8 tradeByteCount = 2; // 6 of 16 bits unused

class WorldTrades : ExpandedInterface
{
public:
  WorldTrades(SaveFile* saveFile = nullptr);
  virtual ~WorldTrades();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  bool completedTrades[tradeCount];
};

#endif // WORLDTRADES_H