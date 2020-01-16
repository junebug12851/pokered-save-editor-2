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
#ifndef SCRIPT_H
#define SCRIPT_H

#include <QVector>
#include <QString>
#include <QHash>

#include <optional>

#include "../../common/types.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Where are you in each map of the world? This tracks your map progression
// for each individual map

struct MapDBEntry;

struct ScriptDBEntry {
  QString name;
  var8 ind;
  var8 size;

  QVector<QString> maps; // Map aliases to deep link to
  std::optional<var8> skip;

  // There is always one map except in one case which results in 2 maps
  // Decided to just put a vector here
  QVector<MapDBEntry*> toMaps;
};

class ScriptsDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<ScriptDBEntry*> store;
  static QHash<QString, ScriptDBEntry*> ind;
};

#endif // SCRIPT_H
