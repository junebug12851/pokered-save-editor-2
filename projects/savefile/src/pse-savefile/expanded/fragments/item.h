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
#ifndef ITEM_H
#define ITEM_H

#include <QObject>
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFileIterator;
class ItemDBEntry;

class SAVEFILE_AUTOPORT Item : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int ind MEMBER ind NOTIFY indChanged)
  Q_PROPERTY(int amount READ getAmount WRITE setAmount NOTIFY amountChanged)

  Q_PROPERTY(bool canSell READ canSell NOTIFY itemChanged STORED false)

  Q_PROPERTY(int buyPriceOneMoney READ buyPriceOneMoney NOTIFY itemChanged STORED false)
  Q_PROPERTY(int buyPriceOneCoins READ buyPriceOneCoins NOTIFY itemChanged STORED false)
  Q_PROPERTY(int sellPriceOneMoney READ sellPriceOneMoney NOTIFY itemChanged STORED false)
  Q_PROPERTY(int sellPriceOneCoins READ sellPriceOneCoins NOTIFY itemChanged STORED false)

  Q_PROPERTY(int buyPriceAllMoney READ buyPriceAllMoney NOTIFY itemChanged STORED false)
  Q_PROPERTY(int buyPriceAllCoins READ buyPriceAllCoins NOTIFY itemChanged STORED false)
  Q_PROPERTY(int sellPriceAllMoney READ sellPriceAllMoney NOTIFY itemChanged STORED false)
  Q_PROPERTY(int sellPriceAllCoins READ sellPriceAllCoins NOTIFY itemChanged STORED false)

public:
  // New Item from iterator or a blank item
  // If iterator provided it extracts 2 bytes for index and amount
  Item(SaveFileIterator* it = nullptr);

  // New Item from a given index and amount
  Item(var8 ind, var8 amount);

  // New Item either blank or random
  Item(bool random);

  // Given name and amount
  Item(QString name, var8 amount);

  void makeConnect();

  virtual ~Item();

  // Given an iterator, saves 2 bytes. Index and Amount
  void save(SaveFileIterator* it);

  ItemDBEntry* toItem();

  bool canSell();

  int buyPriceOneMoney();
  int buyPriceOneCoins();
  int sellPriceOneMoney();
  int sellPriceOneCoins();

  int buyPriceAllMoney();
  int buyPriceAllCoins();
  int sellPriceAllMoney();
  int sellPriceAllCoins();

  int getAmount();
  void setAmount(int val);

signals:
  void indChanged();
  void amountChanged();
  void itemChanged();

public slots:
  void load(int ind, int amount);
  void load(bool random);
  void load(QString name, int amount);
  void reset();
  void randomize();

public:
  // Item Index
  int ind;

  // Item Amount, max of 99 for gen 1 games
  int amount;
};

#endif // ITEM_H
