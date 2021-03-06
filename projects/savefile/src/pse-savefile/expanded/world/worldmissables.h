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
#ifndef WORLDMISSABLES_H
#define WORLDMISSABLES_H

#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

// There's actually significantly more missable bits, 256 in total forming
// 32 bytes. But given they are all unused it makes no sense to load entire
// unused bytes
constexpr var8 missableCount = 228;
constexpr var8 missableByteCount = 29; // 4 bits unused of 232

class SAVEFILE_AUTOPORT WorldMissables : public QObject
{
  Q_OBJECT

public:
  WorldMissables(SaveFile* saveFile = nullptr);
  virtual ~WorldMissables();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int missablesCount();
  Q_INVOKABLE bool missablesAt(int ind);
  Q_INVOKABLE void missablesSet(int ind, bool val);

signals:
  void missablesChanged();

public slots:
  void reset();
  void randomize();

public:
  bool missables[missableCount];
};

#endif // WORLDMISSABLES_H
