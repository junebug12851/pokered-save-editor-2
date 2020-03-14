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

#include <QString>
#include <QJsonValue>

#include <pse-common/types.h>

struct MapDBEntry;

// A list of all the hidden coins in Casino

struct HiddenCoinDBEntry {
  HiddenCoinDBEntry();
  HiddenCoinDBEntry(QJsonValue& data);
  void deepLink();

  QString map;
  var8 x = 0;
  var8 y = 0;

  MapDBEntry* toMap = nullptr;
};

class HiddenCoinsDB
{
public:
  static void load();
  static void deepLink();

  static QVector<HiddenCoinDBEntry*> store;
};

#endif // HIDDENCOINS_H
