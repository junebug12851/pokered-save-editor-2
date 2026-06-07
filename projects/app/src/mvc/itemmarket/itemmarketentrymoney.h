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

/**
 * @brief Market row representing the player's money/coins balance.
 *
 * An ItemMarketEntry subtype; the overrides implement the base's virtual table for
 * a money/coins row. checkout() applies the balance change. See ItemMarketEntry.
 */
class ItemMarketEntryMoney : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryMoney();
  virtual ~ItemMarketEntryMoney();

  virtual QString _name() override;       ///< @copydoc ItemMarketEntry::_name
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount

public slots:
  virtual void checkout() override; ///< Apply the money/coins change.

public:
  static constexpr const char* type = "money"; ///< This row's type key.
};
