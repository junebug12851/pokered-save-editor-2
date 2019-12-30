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
#ifndef FLY_H
#define FLY_H

#include "../../common/types.h"
#include <QString>
#include <QHash>

#include "./maps.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Cities you can fly to

struct FlyEntry {
  QString name; // City Name
  var8 ind; // Index in list

  MapEntry* toMap; // Deep link to associated map data
};

class Fly
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<FlyEntry*>* fly;
  static QHash<QString, FlyEntry*>* ind;
};

#endif // FLY_H
