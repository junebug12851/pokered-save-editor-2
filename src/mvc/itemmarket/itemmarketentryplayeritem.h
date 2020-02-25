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
#ifndef ITEMMARKETENTRYPLAYERITEM_H
#define ITEMMARKETENTRYPLAYERITEM_H

#include "./itemmarketentry.h"

class ItemStorageBox;
class Item;
class PlayerBasics;

// Selling player inventory for money or coins

class ItemMarketEntryPlayerItem : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryPlayerItem(ItemStorageBox* toBox, Item* toItem, PlayerBasics* player);

  virtual QString _name() override;
  virtual int _inStockCount() override;
  virtual bool _canSell() override;
  virtual int _itemWorth() override;
  virtual QString _whichType() override;
  virtual int _onCartMax() override;
  virtual int cartWorth() override;
  virtual QString canCheckout() override;

public slots:
  virtual void checkout() override;

public:
  ItemStorageBox* toBox = nullptr;
  Item* toItem = nullptr;
  PlayerBasics* player = nullptr;
  static constexpr const char* type = "playerItem";
};

#endif // ITEMMARKETENTRYPLAYERITEM_H
