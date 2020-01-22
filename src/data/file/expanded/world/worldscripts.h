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
#ifndef WORLDSCRIPTS_H
#define WORLDSCRIPTS_H

#include <QObject>
#include "../../../../common/types.h"
class SaveFile;

// Can't have a total byte count given scripts are a bit more complicated to
// read
constexpr var8 scriptCount = 97;

class WorldScripts : public QObject
{
  Q_OBJECT

public:
  WorldScripts(SaveFile* saveFile = nullptr);
  virtual ~WorldScripts();

signals:
  void curScriptsChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  var16 curScripts[scriptCount];
};

#endif // WORLDSCRIPTS_H
