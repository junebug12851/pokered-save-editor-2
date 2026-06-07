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

class Item;
class ItemDBEntry;
class ItemStorageBox;

/// Result of working out where a bought stack lands (full stacks + partial in bag/box).
struct StackReturn {
  int full = 0; ///< Number of full new stacks.

  int partialBag = 0; ///< Remainder added to an existing bag stack.
  int partialBox = 0; ///< Remainder added to an existing PC-box stack.

  Item* partialElBag = nullptr; ///< The bag item a partial merges into.
  Item* partialElBox = nullptr; ///< The PC-box item a partial merges into.
};

/**
 * @brief Market row for an item the player can buy from the store.
 *
 * An ItemMarketEntry subtype representing a purchasable item (@ref data) that lands
 * in the bag (@ref toBag) or, if full, the PC box (@ref toBox). calculateStacks()
 * works out the bag/box distribution; checkout() performs the purchase. See
 * ItemMarketEntry.
 */
class ItemMarketEntryStoreItem : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryStoreItem(ItemDBEntry* data, ItemStorageBox* toBag, ItemStorageBox* toBox);
  virtual ~ItemMarketEntryStoreItem();

  StackReturn calculateStacks(); ///< Work out the full/partial bag-and-box landing for the cart qty.

  virtual QString _name() override;       ///< @copydoc ItemMarketEntry::_name
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount

public slots:
  virtual void checkout() override; ///< Buy the item (into bag/box).

public:
  static constexpr const char* type = "storeItem"; ///< This row's type key.
  ItemDBEntry* data = nullptr;       ///< The item being sold.
  ItemStorageBox* toBag = nullptr;   ///< Destination bag.
  ItemStorageBox* toBox = nullptr;   ///< Overflow PC item box.
};
