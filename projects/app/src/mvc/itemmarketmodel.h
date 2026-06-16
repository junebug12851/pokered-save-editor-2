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
#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>
#include <QVariant>

class ItemStorageBox;
class Router;
class PlayerBasics;
class Storage;
class PlayerPokemon;
class ItemMarketEntry;
class SaveFile;
class ItemDBEntry;

/**
 * @brief The Poke-mart / Game Corner "market" model -- buy and sell with a cart.
 *
 * The most complex model in the app. It presents a unified buy/sell market with a
 * cart, switching between four modes (buy/sell x money/coins) and between viewing
 * the player's items and the store's stock. Rows are @ref ItemMarketEntry objects
 * (a small class hierarchy -- player item, store item, money, message, Game Corner
 * Pokemon). It computes cart totals, leftover money, and space checks, and applies
 * the transaction in checkout(). Exposed as `brg.marketModel`.
 *
 * @see ItemMarketEntry (the row base + its subtypes), Storage, PlayerBasics.
 */
class ItemMarketModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(bool isBuyMode MEMBER isBuyMode NOTIFY isBuyModeChanged)             ///< Buy (store) vs sell (player) view.
  Q_PROPERTY(bool isMoneyCurrency MEMBER isMoneyCurrency NOTIFY isMoneyCurrencyChanged) ///< Money vs coins currency.
  Q_PROPERTY(bool isExchangeMode MEMBER isExchangeMode NOTIFY isExchangeModeChanged) ///< Money<->coins exchange (its own list).
  Q_PROPERTY(int whichMode READ whichMode NOTIFY isAnyChanged)                    ///< Combined mode (see SelBuy* enum).

  // Dual-currency totals for the Exchange receipt (money AND coins both move).
  Q_PROPERTY(int exchangeMoneyStart READ exchangeMoneyStart NOTIFY reUpdateValues) ///< Money before the exchange.
  Q_PROPERTY(int exchangeMoneyAfter READ exchangeMoneyAfter NOTIFY reUpdateValues) ///< Money after the cart's swaps.
  Q_PROPERTY(int exchangeCoinsStart READ exchangeCoinsStart NOTIFY reUpdateValues) ///< Coins before the exchange.
  Q_PROPERTY(int exchangeCoinsAfter READ exchangeCoinsAfter NOTIFY reUpdateValues) ///< Coins after the cart's swaps.

  Q_PROPERTY(int totalCartWorth READ totalCartWorth NOTIFY reUpdateValues)        ///< Total cart value (-/+ = buy/sell).
  Q_PROPERTY(int totalCartCount READ totalCartCount NOTIFY reUpdateValues)        ///< Total items in the cart.
  Q_PROPERTY(int moneyStart READ moneyStart NOTIFY reUpdateValues)                ///< Money before checkout.
  Q_PROPERTY(int moneyLeftover READ moneyLeftover NOTIFY reUpdateValues)          ///< Money after checkout.
  Q_PROPERTY(bool anyNotEnoughSpace READ anyNotEnoughSpace NOTIFY reUpdateValues) ///< Any item lacks bag/box space.
  Q_PROPERTY(bool canAnyCheckout READ canAnyCheckout NOTIFY reUpdateValues)       ///< Can the transaction complete?

signals:
  void isBuyModeChanged();
  void isMoneyCurrencyChanged();
  void isExchangeModeChanged();
  void isAnyChanged();
  void reUpdateValues();

public:
  /// Columns (mapped in roleNames()); comments describe each.
  enum ItemRoles {
    // Name of item
    NameRole = Qt::UserRole + 1,

    // Amount of item to sell (When in sell mode)
    InStockCountRole,

    // If the item can be sold (When in sell mode)
    CanSellRole,

    // Value of item individually to buy or sell
    ItemWorthRole,

    // Item Type, There are 5 types
    // * An item the player has which may or may not be sellable
    // * An item the player can buy
    // * A Game Corner Pokemon
    // * Player Money / Coins
    // * A Message
    WhichTypeRole,

    // Stack count of this item
    StackCountRole,

    // Items left that can be placed on the cart
    OnCartLeftRole,

    // Amount of items on cart
    CartCountRole,

    // Value of item total on cart to buy or sell
    CartWorthRole,

    // Stack count of this item
    TotalStackCountRole,
    TotalWorthRole,

    // Can a checkout be done?
    CanCheckoutRole,

    // For this setup, is the item valid?
    ValidItemRole,

    // Exclude this item from most totals, it's not to be mixed in with other
    // items
    ExcludeItemRole,

    // How much money is left after all is said and done
    MoneyLeftRole,

    // Which modes we're in
    BuyModeRole,
    MoneyCurrencyRole,
  };

  /// The four combined modes (returned by whichMode()).
  enum {
    SelBuyMoney = 0,
    SelBuyCoins,
    SelSellMoney,
    SelSellCoins
  };

  ItemMarketModel(ItemStorageBox* itemBag,
                  ItemStorageBox* itemStorage,
                  PlayerBasics* basics,
                  Router* router,
                  PlayerPokemon* playerPokemon,
                  Storage* storage,
                  SaveFile* file);

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.
  bool setData(const QModelIndex& index, const QVariant& value, int role) override; ///< Edit a row (cart count).

  // Value of total on cart
  // Uses -/+ to indicate buy/sell
  int totalCartWorth();   ///< @see totalCartWorth property.
  int totalCartCount();   ///< @see totalCartCount property.
  int whichMode();        ///< @see whichMode property.
  int moneyStart();       ///< @see moneyStart property.
  int moneyLeftover();    ///< @see moneyLeftover property.
  bool anyNotEnoughSpace(); ///< @see anyNotEnoughSpace property.
  bool canAnyCheckout();  ///< @see canAnyCheckout property.

  // Exchange receipt totals (mirror the money rows' checkout deltas exactly).
  int exchangeMoneyStart(); ///< @see exchangeMoneyStart property.
  int exchangeMoneyAfter(); ///< @see exchangeMoneyAfter property.
  int exchangeCoinsStart(); ///< @see exchangeCoinsStart property.
  int exchangeCoinsAfter(); ///< @see exchangeCoinsAfter property.

  void onReUpdateValues(); ///< Recompute the derived totals.

  // Re-create list cache methods
  bool vendorListItem(ItemDBEntry* el); ///< Should @p el appear in the store list?
  void clearList();           ///< Empty the row cache.
  void buildList();           ///< Build the rows for the current mode.
  void buildPlayerItemList(); ///< Build rows from the player's items.
  void buildMartItemList();   ///< Build rows from the store stock.
  void buildExchangeList();   ///< Build the money<->coins exchange rows (both directions).

  // Respodning to events and signals
  void pageOpening(QString path); ///< Hook when the market page opens.

public slots:
  void checkout();   ///< Apply the cart transaction to the save.
  void reUpdateAll(); ///< Rebuild + recompute everything.

public:
  QVector<ItemMarketEntry*> itemListCache; ///< The current market rows.

  // Buy or Sell, do we view your items or their items?
  // Default to viewing your items
  bool isBuyMode = false; ///< @see isBuyMode property.

  // Money or Coins, do we sell your items to money/coins or do we look at the
  // Pokemart or Game Corner Mart to buy with money and coins
  // Default to dealing with money
  bool isMoneyCurrency = true; ///< @see isMoneyCurrency property.

  // The money<->coins exchange as its own list (pulled out of the buy/sell lists).
  // When true the buy/sell + currency choice is irrelevant; the list is the two
  // swap rows.
  bool isExchangeMode = false; ///< @see isExchangeMode property.

  // Connections to the Sav Data
  ItemStorageBox* itemBag = nullptr;     ///< The player's bag.
  ItemStorageBox* itemStorage = nullptr; ///< The PC item box.
  Router* router = nullptr;              ///< For page hooks.
  PlayerBasics* basics = nullptr;        ///< Player money/coins.
  PlayerPokemon* playerPokemon = nullptr; ///< Party (for received Pokemon).
  Storage* storage = nullptr;            ///< PC storage (for received Pokemon).
  SaveFile* file = nullptr;              ///< The live save.
};
