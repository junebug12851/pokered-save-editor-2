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
#ifndef TMHM_H
#define TMHM_H

#include "../common/types.h"
#include <QString>

struct ItemEntry;
struct MoveEntry;

// All the TM's and HM's in the game
// internally, HM's are specially treated TM's that start at TM 51

class TmHms
{
public:
  static void load();
  static void deepLink();

  static QVector<QString>* tmHms;
  static QVector<ItemEntry*>* toTmHmItem;
  static QVector<MoveEntry*>* toTmHmMove;
};

#endif // TMHM_H
