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
#include <QHash>
#include <QVector>
#include <QVariant>

class PlayerBasics;

/**
 * @brief Abstract base for one row of the item market -- the row-type hierarchy root.
 *
 * Each market row is an ItemMarketEntry subtype: a player item, a store item,
 * money/coins, a message, or a Game Corner Pokemon. This base defines the shared
 * cart mechanics (@ref onCart, worth, stack/space limits, checkout gating) and a
 * @e virtual table of `_name()`, `_inStockCount()`, `_canSell()`, `_itemWorth()`,
 * `_whichType()`, `stackCount()`, `onCartLeft()`, `checkout()` that subtypes
 * implement. The @ref compatMoneyCurrency / @ref compatBuyMode flags filter which
 * rows appear in which of the model's four modes.
 *
 * @note Static members (@ref isMoneyCurrency, @ref isBuyMode, @ref player,
 *       @ref instances) are shared because the market model is effectively a
 *       singleton; expensive per-row values are memoised in @ref cache.
 * @see ItemMarketModel, and the subtypes ItemMarketEntryMoney/Message/StoreItem/
 *      PlayerItem/GCPokemon.
 */
class ItemMarketEntry : public QObject
{
  Q_OBJECT

  // These change
  Q_PROPERTY(int onCart READ getCartCount WRITE setCartCount NOTIFY onCartChanged) ///< Quantity on the cart.
  Q_PROPERTY(bool canCheckout READ canCheckout NOTIFY onCartChanged)               ///< Can this row check out?
  Q_PROPERTY(int cartWorth READ cartWorth NOTIFY onCartChanged)                    ///< Value of this row's cart qty.
  Q_PROPERTY(int stackCount READ stackCount NOTIFY onCartChanged)                  ///< New stack slots this row needs.
  Q_PROPERTY(int onCartLeft READ onCartLeft NOTIFY onCartChanged)                  ///< How many more can be added.
  Q_PROPERTY(int totalStackCount READ totalStackCount NOTIFY onCartChanged)        ///< Total stacks across this type.

  // These do not change, they depend on the mode but if the mode changes they
  // will be re-created anyways. during the lifetime of the class, they will not
  // change.
  // However a signal is in place to force a re-update should it be needed
  Q_PROPERTY(QString name READ name NOTIFY doReUpdateConstants)             ///< Display name (mode-stable).
  Q_PROPERTY(int inStockCount READ inStockCount NOTIFY doReUpdateConstants) ///< Owned/sellable count (mode-stable).
  Q_PROPERTY(bool canSell READ canSell NOTIFY doReUpdateConstants)          ///< Sellable (mode-stable).
  Q_PROPERTY(int itemWorth READ itemWorth NOTIFY doReUpdateConstants)       ///< Unit value (mode-stable).
  Q_PROPERTY(QString whichType READ whichType NOTIFY doReUpdateConstants)   ///< Row type label (mode-stable).

signals:
  // Notified when cart is changed
  void onCartChanged(); ///< This row's cart quantity changed.

  // Emitted externally when data that is normally not re-read should be re-read
  void doReUpdateConstants(); ///< Force a refresh of the "mode-stable" values.

public:
  /// Three-state compatibility for the mode filters.
  enum {
    // Don't change these or re-assign different values or ordering
    CompatNo = 0,
    CompatYes,
    CompatEither
  };

  /// Cache keys for the memoised mode-stable values.
  enum {
    HashKeyName,
    HashKeyInStockCount,
    HashKeyCanSell,
    HashKeyItemWorth,
    HashKeyWhichType,
  };

  /// @param compatMoneyCurrency / @param compatBuyMode which modes this row shows in.
  ItemMarketEntry(int compatMoneyCurrency = CompatEither,
                  int compatBuyMode = CompatEither);

  virtual ~ItemMarketEntry();

  // Called only for the first instance
  virtual void initOnce(); ///< One-time setup for the first instance of a type.

  void finishConstruction(); ///< Finalise construction (register the instance).

  // Virtual Table for child classes

  // Usually never change

  // Name of item
  QString name();          ///< Cached display name.
  virtual QString _name() = 0; ///< Subtype: compute the display name.

  // Amount of item to sell (When in sell mode)
  int inStockCount();          ///< Cached owned/sellable count.
  virtual int _inStockCount() = 0; ///< Subtype: compute the owned/sellable count.

  // If the item can be sold (When in sell mode)
  bool canSell();          ///< Cached sellable flag.
  virtual bool _canSell() = 0; ///< Subtype: compute sellability.

  // Value of item individually to buy or sell
  int itemWorth();         ///< Cached unit value.
  virtual int _itemWorth() = 0; ///< Subtype: compute the unit value.

  // Type of this item
  QString whichType();     ///< Cached type label.
  virtual QString _whichType() = 0; ///< Subtype: report the type label.

  // Stack count of this item
  // This goes for all types, anything that takes up more than one stack
  // Pokemon are a stack of 1, Items are a stack of 99, Money and others don't
  // take up any stack (they are a stack of infinity)
  // This number is for counting new stack space that will be filled, not
  // consuming existing stack space.
  virtual int stackCount() = 0; ///< Subtype: new stack slots needed (see note above).

  // Left that can be placed on the cart
  virtual int onCartLeft() = 0; ///< Subtype: how many more may be added.

  // Tools for the child class to leverage
  bool requestFilter(); ///< Helper: does this row pass the current mode filter?

  // Members that wrap around properties
  int getCartCount(); ///< Current cart quantity (backs @c onCart).

  // Value of item total on cart to buy or sell
  int cartWorth(); ///< Value of the cart quantity.

  // Returns total stack count for all instances in this type
  int totalStackCount();      ///< Stacks across all rows of this type.
  unsigned int totalWorth();  ///< Worth across all rows of this type.

  // Calculate how much money is leftover
  int moneyLeftover(); ///< Money remaining if this checks out.

  // Can checkout notifies if the checkout can be completed.
  virtual bool canCheckout(); ///< Can this row alone check out?
  bool canAnyCheckout();      ///< Can any row check out?

public slots:
  virtual void checkout() = 0; ///< Subtype: apply this row's transaction.
  void setCartCount(int val);  ///< Set the cart quantity (backs @c onCart).
  void reUpdateConstants();    ///< Clear the cached mode-stable values.

public:
  // Compatibility of this item, set by child classes
  int compatMoneyCurrency = CompatEither; ///< Money/coins compatibility.
  int compatBuyMode = CompatEither;       ///< Buy/sell compatibility.

  // Properties
  int onCart = 0; ///< Backing cart quantity.

  // Excude from total calculations
  // Will not be included in most. if not all, calculations such as total items
  // on cart and total worth amongst others.
  bool exclude = false; ///< Exclude from aggregate totals (see note).

  // References to the model class, these are static because the model class
  // is a singleton
  static bool* isMoneyCurrency; ///< Shared: current currency mode.
  static bool* isBuyMode;       ///< Shared: current buy/sell mode.
  static PlayerBasics* player;  ///< Shared: player money/coins.

  // Holds global instances categorized by type
  static QHash<QString, QVector<ItemMarketEntry*>*> instances; ///< All rows, grouped by type.

  // Holds all instances combined regardless of type
  static QVector<ItemMarketEntry*> instancesCombined; ///< All rows, flat.

  // Cached data
  // Many data are very expensive to re-create, we cache them here and re-create
  // them only when needed
  QHash<int, QVariant> cache; ///< Memoised mode-stable values (see HashKey* enum).
};
