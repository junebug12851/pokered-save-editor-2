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

  Q_PROPERTY(QVector<HoFRecord*> records MEMBER records NOTIFY recordsChanged)
  Q_PROPERTY(var8 recordMax_ READ recordMax NOTIFY recordsChanged)

public:
  HallOfFame(SaveFile* saveFile = nullptr);
  virtual ~HallOfFame();

  // Since Qt has tied my hands in so many ways on fixing the issue of no arrays
  // but primitive arrays being able to be sent to QML, I'm left with no other
  // options outside of a custom model to flood classes with a series of methods
  // for each and every array of any kind that's not primitive
  Q_INVOKABLE var8 recordCount();
  Q_INVOKABLE var8 recordMax();
  Q_INVOKABLE HoFRecord* recordAt(var8 ind);
  Q_INVOKABLE void recordSwap(var8 from, var8 to);
  Q_INVOKABLE void recordRemove(var8 ind);
  Q_INVOKABLE void recordNew();

signals:
  void recordsChanged();

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
