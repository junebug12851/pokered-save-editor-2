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
#include <QPointer>

#include "./itemmarketentry.h"
// Full types (not fwd decls) so QPointer<> can verify they are QObjects. toBox/toItem
// are NOT owned by this entry (they live in the save's item box) and can be freed out
// from under a stale entry that lingers in the static instances registry -- QPointer
// auto-nulls on their destruction so a stale read returns a safe default, not a UAF.
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>

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
  virtual QString infoText() override;    ///< @copydoc ItemMarketEntry::infoText

public slots:
  virtual void checkout() override; ///< Sell the item (remove qty, credit balance).

public:
  QPointer<ItemStorageBox> toBox;  ///< The box the item is sold from (auto-nulls if freed).
  QPointer<Item> toItem;           ///< The item being sold (auto-nulls if freed).
  static constexpr const char* type = "playerItem"; ///< This row's type key.
};
