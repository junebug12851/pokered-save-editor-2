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
#ifndef HALLOFFAME_H
#define HALLOFFAME_H

#include <QObject>
#include <QVector>
#include <QQmlListProperty>
#include "../../../common/types.h"
class SaveFile;
class HoFRecord;

constexpr var8 recordsMax = 50;

class HallOfFame : public QObject
{
  Q_OBJECT

public:
  HallOfFame(SaveFile* saveFile = nullptr);
  virtual ~HallOfFame();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  // All Hall of Fame Records
  QVector<HoFRecord*> records;
};

#endif // HALLOFFAME_H
