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
#ifndef WORLDTOWNS_H
#define WORLDTOWNS_H

#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

constexpr var8 townCount = 11;
constexpr var8 townByteCount = 2; // 5 bits of 16 unused

class SAVEFILE_AUTOPORT WorldTowns : public QObject
{
  Q_OBJECT

public:
  WorldTowns(SaveFile* saveFile = nullptr);
  virtual ~WorldTowns();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int townsCount();
  Q_INVOKABLE bool townsAt(int ind);
  Q_INVOKABLE void townsSet(int ind, bool val);

signals:
  void visitedTownsChanged();

public slots:
  void reset();
  void randomize();

public:
  bool visitedTowns[townCount];
};

#endif // WORLDTOWNS_H
