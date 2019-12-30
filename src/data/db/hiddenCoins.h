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
#ifndef HIDDENCOINS_H
#define HIDDENCOINS_H

#include "../../common/types.h"
#include <QString>

#include "./maps.h"

// A list of all the hidden coins in Casino

struct HiddenCoinEntry {
  QString map;
  var8 x;
  var8 y;

  MapEntry* toMap;
};

class HiddenCoins
{
public:
  static void load();
  static void deepLink();

  static QVector<HiddenCoinEntry*>* hiddenCoins;
};

#endif // HIDDENCOINS_H
