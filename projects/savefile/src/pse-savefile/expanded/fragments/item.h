/*
  * Copyright 2020 Fairy Fox
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
#pragma once
#include <QObject>
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFileIterator;
class ItemDBEntry;

/**
 * @brief One inventory slot: an item index and an amount, with live pricing.
 *
 * On disk an item is just two bytes (index + amount). On top of those, this
 * object computes a set of QML pricing properties -- buy/sell, single/all,
 * money/coins -- by resolving @ref ind through the items DB. Lives inside an
 * ItemStorageBox (bag or PC).
 *
 * @see ItemStorageBox (container), ItemDBEntry (item data / prices).
 */
class SAVEFILE_AUTOPORT Item : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int ind MEMBER ind NOTIFY indChanged)                     ///< Item index (into the items DB).
  Q_PROPERTY(int amount READ getAmount WRITE setAmount NOTIFY amountChanged) ///< Quantity (max 99 in Gen 1).

  Q_PROPERTY(bool canSell READ canSell NOTIFY itemChanged STORED false) ///< Is this item sellable?

  Q_PROPERTY(int buyPriceOneMoney READ buyPriceOneMoney NOTIFY itemChanged STORED false)   ///< Buy price (one) in money.
  Q_PROPERTY(int buyPriceOneCoins READ buyPriceOneCoins NOTIFY itemChanged STORED false)   ///< Buy price (one) in coins.
  Q_PROPERTY(int sellPriceOneMoney READ sellPriceOneMoney NOTIFY itemChanged STORED false) ///< Sell price (one) in money.
  Q_PROPERTY(int sellPriceOneCoins READ sellPriceOneCoins NOTIFY itemChanged STORED false) ///< Sell price (one) in coins.

  Q_PROPERTY(int buyPriceAllMoney READ buyPriceAllMoney NOTIFY itemChanged STORED false)   ///< Buy price (whole stack) in money.
  Q_PROPERTY(int buyPriceAllCoins READ buyPriceAllCoins NOTIFY itemChanged STORED false)   ///< Buy price (whole stack) in coins.
  Q_PROPERTY(int sellPriceAllMoney READ sellPriceAllMoney NOTIFY itemChanged STORED false) ///< Sell price (whole stack) in money.
  Q_PROPERTY(int sellPriceAllCoins READ sellPriceAllCoins NOTIFY itemChanged STORED false) ///< Sell price (whole stack) in coins.

public:
  /// New Item from iterator or a blank item.
  /// If iterator provided it extracts 2 bytes for index and amount.
  Item(SaveFileIterator* it = nullptr);

  /// New Item from a given index and amount.
  Item(var8 ind, var8 amount);

  /// New Item either blank or random.
  Item(bool random);

  /// Given name and amount.
  Item(QString name, var8 amount);

  void makeConnect(); ///< Wire up internal signal connections.

  virtual ~Item();

  /// Given an iterator, saves 2 bytes: index and amount.
  void save(SaveFileIterator* it);

  ItemDBEntry* toItem(); ///< Resolve @ref ind to its DB entry.

  bool canSell(); ///< Whether the item may be sold.

  int buyPriceOneMoney();   ///< @see buyPriceOneMoney property.
  int buyPriceOneCoins();   ///< @see buyPriceOneCoins property.
  int sellPriceOneMoney();  ///< @see sellPriceOneMoney property.
  int sellPriceOneCoins();  ///< @see sellPriceOneCoins property.

  int buyPriceAllMoney();   ///< @see buyPriceAllMoney property.
  int buyPriceAllCoins();   ///< @see buyPriceAllCoins property.
  int sellPriceAllMoney();  ///< @see sellPriceAllMoney property.
  int sellPriceAllCoins();  ///< @see sellPriceAllCoins property.

  int getAmount();         ///< Current amount (backs property READ).
  void setAmount(int val); ///< Set amount (backs property WRITE; clamped to the Gen 1 max).

signals:
  void indChanged();
  void amountChanged();
  void itemChanged();

public slots:
  void load(int ind, int amount);   ///< Set from an index + amount.
  void load(bool random);           ///< Set blank or random.
  void load(QString name, int amount); ///< Set from an item name + amount.
  void reset();                     ///< Blank this slot.
  void randomize();                 ///< Randomize this slot.

public:
  // Item Index
  int ind;    ///< Item index (backs property).

  // Item Amount, max of 99 for gen 1 games
  int amount; ///< Item amount (max 99 in Gen 1; backs property).
};
