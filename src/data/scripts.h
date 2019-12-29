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

#include "../common/types.h"
#include <optional>
#include <QString>

// With amazing help of Quicktype!!!
// https://app.quicktype.io

struct ScriptEntry {
  QString name;
  var8 ind;
  var8 size;

  std::optional<var8> skip;
};

class Scripts
{
public:
  static void load();
  static QVector<ScriptEntry*>* scripts;
};

#endif // SCRIPT_H
