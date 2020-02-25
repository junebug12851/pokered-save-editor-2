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
#ifndef ITEMMARKETENTRYSTOREITEM_H
#define ITEMMARKETENTRYSTOREITEM_H

#include "./itemmarketentry.h"

class Item;
class ItemDBEntry;
class ItemStorageBox;

struct StackReturn {
  int full = 0;

  int partialBag = 0;
  int partialBox = 0;

  Item* partialElBag = nullptr;
  Item* partialElBox = nullptr;
};

class ItemMarketEntryStoreItem : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryStoreItem(ItemDBEntry* data, ItemStorageBox* toBag, ItemStorageBox* toBox);
  virtual ~ItemMarketEntryStoreItem();

  StackReturn calculateStacks();

  virtual QString _name() override;
  virtual int _inStockCount() override;
  virtual bool _canSell() override;
  virtual int _itemWorth() override;
  virtual QString _whichType() override;
  virtual int onCartMax() override;
  virtual int stackCount() override;

public slots:
  virtual void checkout() override;

public:
  static constexpr const char* type = "storeItem";
  ItemDBEntry* data = nullptr;
  ItemStorageBox* toBag = nullptr;
  ItemStorageBox* toBox = nullptr;
};

#endif // ITEMMARKETENTRYSTOREITEM_H
