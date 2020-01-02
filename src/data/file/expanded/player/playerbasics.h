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
#ifndef PLAYERBASICS_H
#define PLAYERBASICS_H

#include <QString>
#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;

class PlayerBasics : ExpandedInterface
{
public:
  PlayerBasics(SaveFile* saveFile = nullptr);
  virtual ~PlayerBasics();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  void setBadges(SaveFile* saveFile, var16 offset);

  QString playerName;
  var16 playerID;
  var32 money;
  var16 coins;
  bool badges[8] = {
    false, // Boulder
    false, // Cascade
    false, // Thunder
    false, // Rainbow
    false, // Soul
    false, // Marsh
    false, // Volcano
    false // Earth
  };
  var8 playerStarter = 0;
};

#endif // PLAYERBASICS_H
