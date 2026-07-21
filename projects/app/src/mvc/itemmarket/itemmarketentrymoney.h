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
  /// Direction of a money row.
  enum {
    DirGlobal = -1, ///< Follow the model's global buy/sell flag (legacy behaviour).
    DirToMoney = 0, ///< Coins => Money (the "sell" direction).
    DirToCoins = 1  ///< Money => Coins (the "buy" direction).
  };

  /// @param forceDir fix this row's exchange direction (see Dir*); DirGlobal
  ///        follows the model's buy/sell flag. The dedicated Exchange list builds
  ///        one of each fixed direction so both swaps appear at once.
  explicit ItemMarketEntryMoney(int forceDir = DirGlobal);
  virtual ~ItemMarketEntryMoney();

  virtual QString _name() override;       ///< @copydoc ItemMarketEntry::_name
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount
  virtual bool canCheckout() override;    ///< Exchange-aware affordability gate.

  /// This row's effective direction: true = Money=>Coins (spend money), false =
  /// Coins=>Money (spend coins). Honours @ref forceDir, else the global buy flag.
  bool buying() const;

  // The unit traded is ONE COIN (onCart = number of coins). A coin costs
  // GameCornerDB buy price (~20) to buy and returns the sell price (~half) when
  // sold -- the source of truth for the whole Game Corner / Mart economy.
  int moneyDelta() const; ///< Signed money change for this row's onCart (-cost buying, +gain selling).
  int coinsDelta() const; ///< Signed coins change for this row's onCart (+buying, -selling).

public slots:
  virtual void checkout() override; ///< Apply the money/coins change.

public:
  static constexpr const char* type = "money"; ///< This row's type key.

  int forceDir = DirGlobal; ///< Fixed direction, or DirGlobal to follow the model.
};
