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
#ifndef ITEMMARKETMODEL_H
#define ITEMMARKETMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class ItemStorageBox;
class Router;
class PlayerBasics;
class Storage;
class PlayerPokemon;
class ItemMarketEntry;
class SaveFile;

class ItemMarketModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(bool isBuyMode MEMBER isBuyMode NOTIFY isBuyModeChanged)
  Q_PROPERTY(bool isMoneyCurrency MEMBER isMoneyCurrency NOTIFY isMoneyCurrencyChanged)
  Q_PROPERTY(int totalCartWorth READ totalCartWorth NOTIFY reUpdateValues)
  Q_PROPERTY(int totalCartCount READ totalCartCount NOTIFY reUpdateValues)

signals:
  void isBuyModeChanged();
  void isMoneyCurrencyChanged();
  void reUpdateValues();

public:
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

    // Which modes we're in
    BuyModeRole,
    MoneyCurrencyRole,
  };

  ItemMarketModel(ItemStorageBox* itemBag,
                  ItemStorageBox* itemStorage,
                  PlayerBasics* basics,
                  Router* router,
                  PlayerPokemon* playerPokemon,
                  Storage* storage,
                  SaveFile* file);

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  // Value of total on cart
  // Uses -/+ to indicate buy/sell
  int totalCartWorth();
  int totalCartCount();

  // Re-create list cache methods
  void clearList();
  void buildList();
  void buildPlayerItemList();
  void buildMartItemList();

  // Respodning to events and signals
  void pageOpening(QString path);

public slots:
  void checkout();
  void reUpdateAll();

public:
  QVector<ItemMarketEntry*> itemListCache;

  // Buy or Sell, do we view your items or their items?
  // Default to viewing your items
  bool isBuyMode = false;

  // Money or Coins, do we sell your items to money/coins or do we look at the
  // Pokemart or Game Corner Mart to buy with money and coins
  // Default to dealing with money
  bool isMoneyCurrency = true;

  // Connections to the Sav Data
  ItemStorageBox* itemBag = nullptr;
  ItemStorageBox* itemStorage = nullptr;
  Router* router = nullptr;
  PlayerBasics* basics = nullptr;
  PlayerPokemon* playerPokemon = nullptr;
  Storage* storage = nullptr;
  SaveFile* file = nullptr;
};

#endif // ITEMMARKETMODEL_H
