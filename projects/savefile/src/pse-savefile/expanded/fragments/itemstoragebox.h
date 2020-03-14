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
#ifndef ITEMSTORAGEBOX_H
#define ITEMSTORAGEBOX_H

#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class Item;

class SAVEFILE_AUTOPORT ItemStorageBox : public QObject
{
  Q_OBJECT

  // How many items are there
  Q_PROPERTY(int itemsCount READ itemsCount NOTIFY itemsChanged STORED false)

  // How many items including item amounts are there
  Q_PROPERTY(int itemsCountBulk READ itemsCountBulk NOTIFY itemsChanged STORED false)

  // Maximum number of items
  Q_PROPERTY(int itemsMax READ itemsMax STORED false CONSTANT)

  // Is this the players bag or PC storage
  Q_PROPERTY(bool isBag READ getIsBag CONSTANT)

  // Can we relocate? If the other box is full then we cannot
  Q_PROPERTY(bool relocateFull READ relocateFull NOTIFY itemsChanged STORED false)

  // What's the worth of all the items?
  Q_PROPERTY(int itemsAllBuyMoney READ itemsAllBuyMoney NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsAllBuyCoins READ itemsAllBuyCoins NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsAllSellMoney READ itemsAllSellMoney NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsAllSellCoins READ itemsAllSellCoins NOTIFY itemsChanged STORED false)

  // Get destination box
  Q_PROPERTY(ItemStorageBox* destBox READ destBox STORED false CONSTANT)

public:
  ItemStorageBox(bool isBag, int maxSize, SaveFile* saveFile = nullptr, int offset = 0);
  virtual ~ItemStorageBox();

  void load(SaveFile* saveFile = nullptr, int offset = 0);
  void save(SaveFile* saveFile, int offset);

  // These are provided as properties
  int itemsCount();
  int itemsCountBulk();
  int itemsMax();
  bool getIsBag();
  bool relocateFull();
  ItemStorageBox* destBox();

  int itemsAllBuyMoney();
  int itemsAllBuyCoins();
  int itemsAllSellMoney();
  int itemsAllSellCoins();

  // Methods that can be called from QML which cannot be properties and make no
  // sense as slots
  Q_INVOKABLE Item* itemAt(int ind);

  // Auto-called from randomize function depending on box type
  void randomizeStorage();
  void randomizeBag();

signals:
  // When moving an item away from the box to another box, allow the model to
  // perform cleanup actions
  void beforeItemRelocate(Item* item);

  // Alert the model on changes
  void itemsChanged();
  void itemMoveChange(int from, int to);
  void itemRemoveChange(int ind);
  void itemInsertChange();
  void itemsResetChange();

public slots:

  // These alow the model or QML to interact with the box
  void reset();
  void randomize();
  bool itemMove(int from, int to);
  void itemRemove(int ind);
  void itemNew();
  bool relocateAll();
  bool relocateOne(int ind);
  void sort();

public:
  QVector<Item*> items;

  // These are set from the creating class SAVEFILE_AUTOPORT indicating details of the box
  // It's crucial they are read-only
  int maxSize;
  bool isBag;
  SaveFile* file;
};

#endif // ITEMSTORAGEBOX_H
