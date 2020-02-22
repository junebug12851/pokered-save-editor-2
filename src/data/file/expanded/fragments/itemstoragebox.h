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
#include "../../../../common/types.h"

class SaveFile;
class Item;

class ItemStorageBox : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int itemsCount READ itemsCount NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsCountBulk READ itemsCountBulk NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsMax READ itemsMax STORED false CONSTANT)
  Q_PROPERTY(bool isBag READ getIsBag CONSTANT)
  Q_PROPERTY(bool relocateFull READ relocateFull NOTIFY itemsChanged STORED false)
  Q_PROPERTY(int itemsWorth READ itemsWorth NOTIFY itemsChanged STORED false)

public:
  ItemStorageBox(bool isBag, int maxSize, SaveFile* saveFile = nullptr, int offset = 0);
  virtual ~ItemStorageBox();

  void load(SaveFile* saveFile = nullptr, int offset = 0);
  void save(SaveFile* saveFile, int offset);

  int itemsCount();
  int itemsCountBulk();
  int itemsMax();
  int itemsWorth();
  bool getIsBag();
  bool relocateFull();
  Q_INVOKABLE Item* itemAt(int ind);

signals:
  void beforeItemRelocate(Item* item);
  void itemsChanged();
  void itemMoveChange(int from, int to);
  void itemRemoveChange(int ind);
  void itemInsertChange();
  void itemsResetChange();

public slots:
  void reset();
  void randomize();
  void itemMove(int from, int to);
  void itemRemove(int ind);
  void itemNew();
  bool relocateAll();
  bool relocateOne(int ind);
  void sort();

  bool itemMoveTop(int ind);
  bool itemMoveUp(int ind);
  bool itemMoveDown(int ind);
  bool itemMoveBottom(int ind);

public:
  QVector<Item*> items;
  int maxSize;

  bool isBag;
  SaveFile* file;
};

#endif // ITEMSTORAGEBOX_H
