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
#ifndef PLAYERPOKEDEX_H
#define PLAYERPOKEDEX_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"
class SaveFile;

constexpr var8 maxPokedex = 151;

class PlayerPokedex : public QObject
{
  Q_OBJECT

public:
  PlayerPokedex(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokedex();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  void loadPokedex(SaveFile* saveFile, QVector<bool> toArr, var16 fromOffset);
  void savePokedex(SaveFile* saveFile, QVector<bool> fromArr, var16 toOffset);

  Q_INVOKABLE int ownedCount();
  Q_INVOKABLE bool ownedAt(int ind);
  Q_INVOKABLE void ownedSet(int ind, bool val);

  Q_INVOKABLE int seenCount();
  Q_INVOKABLE bool seenAt(int ind);
  Q_INVOKABLE void seenSet(int ind, bool val);

signals:
  void ownedChanged();
  void seenChanged();

public slots:
  void reset();
  void randomize();

public:
  bool owned[maxPokedex];
  bool seen[maxPokedex];
};

#endif // PLAYERPOKEDEX_H
