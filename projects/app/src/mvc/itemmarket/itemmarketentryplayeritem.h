/*
  * Copyright 2020 Twilight
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
#include "./itemmarketentry.h"

class ItemStorageBox;
class Item;
class PlayerBasics;

// Selling player inventory for money or coins

/**
 * @brief Market row for one of the player's own items being sold.
 *
 * An ItemMarketEntry subtype for selling a player-owned @ref toItem (from
 * @ref toBox) for money or coins. checkout() removes the sold quantity and credits
 * the balance. See ItemMarketEntry.
 */
class ItemMarketEntryPlayerItem : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryPlayerItem(ItemStorageBox* toBox, Item* toItem);
  virtual ~ItemMarketEntryPlayerItem();

  virtual QString _name() override;       ///< @copydoc ItemMarketEntry::_name
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount

public slots:
  virtual void checkout() override; ///< Sell the item (remove qty, credit balance).

public:
  ItemStorageBox* toBox = nullptr; ///< The box the item is sold from.
  Item* toItem = nullptr;          ///< The item being sold.
  static constexpr const char* type = "playerItem"; ///< This row's type key.
};
