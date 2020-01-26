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
#ifndef AREASIGN_H
#define AREASIGN_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"

class SaveFile;
class SignData;
class MapDBEntry;

constexpr var8 maxSigns = 16;

class AreaSign : public QObject
{
  Q_OBJECT

public:
  AreaSign(SaveFile* saveFile = nullptr);
  virtual ~AreaSign();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void randomize(MapDBEntry* mapData);

  Q_INVOKABLE int signCount();
  Q_INVOKABLE int signMax();
  Q_INVOKABLE SignData* signAt(int ind);
  Q_INVOKABLE void signSwap(int from, int to);
  Q_INVOKABLE void signRemove(int ind);
  Q_INVOKABLE void signNew();

signals:
  void signsChanged();

public slots:
  void reset();

public:
  QVector<SignData*> signs;
};

#endif // AREASIGN_H
