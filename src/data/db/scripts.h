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

#include <QString>
#include <QHash>

#include <optional>

#include "../../common/types.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Where are you in each map of the world? This tracks your map progression
// for each individual map

struct ScriptDBEntry {
  QString name;
  var8 ind;
  var8 size;

  std::optional<var8> skip;
};

class ScriptsDB
{
public:
  static void load();
  static void index();

  static QVector<ScriptDBEntry*> store;
  static QHash<QString, ScriptDBEntry*> ind;
};

#endif // SCRIPT_H
