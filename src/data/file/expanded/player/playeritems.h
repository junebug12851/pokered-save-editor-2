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
#ifndef PLAYERITEMS_H
#define PLAYERITEMS_H

#include <QObject>
#include "../../../../common/types.h"
#include <QVector>

class SaveFile;
struct Item;

constexpr var8 maxBagItems = 20;

class PlayerItems : public QObject
{
  Q_OBJECT

public:
  PlayerItems(SaveFile* saveFile = nullptr);
  virtual ~PlayerItems();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int bagItemCount();
  Q_INVOKABLE int bagItemMax();
  Q_INVOKABLE Item* bagItemAt(int ind);
  Q_INVOKABLE void bagItemSwap(int from, int to);
  Q_INVOKABLE void bagItemRemove(int ind);
  Q_INVOKABLE void bagItemNew();

signals:
  void bagItemsChanged();
  void bagItemSwapChange(int from, int to);
  void bagItemRemoveChange(int ind);
  void bagItemInsertChange();
  void bagItemResetChange();

public slots:
  void reset();
  void randomize();

public:
  QVector<Item*> bagItems;
};

#endif // PLAYERITEMS_H
