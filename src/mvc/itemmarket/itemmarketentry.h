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
#ifndef ITEMMARKETENTRY_H
#define ITEMMARKETENTRY_H

#include <QObject>
#include <QHash>
#include <QVariant>

class ItemMarketModel;

class ItemMarketEntry : public QObject
{
  Q_OBJECT

  // These change
  Q_PROPERTY(int onCart READ getCartCount WRITE setCartCount NOTIFY onCartChanged)
  Q_PROPERTY(QString canCheckout READ canCheckout NOTIFY onCartChanged)
  Q_PROPERTY(int cartWorth READ cartWorth NOTIFY onCartChanged)

  // These do not change, they depend on the mode but if the mode changes they
  // will be re-created anyways. during the lifetime of the class, they will not
  // change.
  // However a signal is in place to force a re-update should it be needed
  Q_PROPERTY(QString name READ name NOTIFY doReUpdateConstants)
  Q_PROPERTY(int inStockCount READ inStockCount NOTIFY doReUpdateConstants)
  Q_PROPERTY(bool canSell READ canSell NOTIFY doReUpdateConstants)
  Q_PROPERTY(int itemWorth READ itemWorth NOTIFY doReUpdateConstants)
  Q_PROPERTY(QString whichType READ whichType NOTIFY doReUpdateConstants)
  Q_PROPERTY(int onCartMax READ onCartMax NOTIFY doReUpdateConstants)

signals:
  // Notified when cart is changed
  void onCartChanged();

  // Emitted externally when data that is normally not re-read should be re-read
  void doReUpdateConstants();

public:
  enum {
    // Don't change these or re-assign different values or ordering
    CompatNo = 0,
    CompatYes,
    CompatEither
  };

  enum {
    HashKeyName,
    HashKeyInStockCount,
    HashKeyCanSell,
    HashKeyItemWorth,
    HashKeyWhichType,
    HashKeyOnCartMax,
  };

  ItemMarketEntry(int compatMoneyCurrency = CompatEither,
                  int compatBuyMode = CompatEither);

  virtual ~ItemMarketEntry();

  // Virtual Table for child classes

  // Usually never change

  // Name of item
  QString name();
  virtual QString _name() = 0;

  // Amount of item to sell (When in sell mode)
  int inStockCount();
  virtual int _inStockCount() = 0;

  // If the item can be sold (When in sell mode)
  bool canSell();
  virtual bool _canSell() = 0;

  // Value of item individually to buy or sell
  int itemWorth();
  virtual int _itemWorth() = 0;

  // Type of this item
  QString whichType();
  virtual QString _whichType() = 0;

  // Maximum that can be placed on the cart
  int onCartMax();
  virtual int _onCartMax() = 0;

  // Can change

  // Value of item total on cart to buy or sell
  virtual int cartWorth() = 0;

  // Do a checkout, can checkout notifies if the checkout can be completed.
  // If it cannot it returns a message why. Calling checkout won't work if the
  // item cannot be checked out.
  virtual QString canCheckout() = 0;

  // Tools for the child class to leverage
  bool requestFilter();

  // Members that wrap around properties
  int getCartCount();

public slots:
  virtual void checkout() = 0;
  void setCartCount(int val);
  void reUpdateConstants();

public:
  // Compatibility of this item, set by child classes
  int compatMoneyCurrency = CompatEither;
  int compatBuyMode = CompatEither;

  // Properties
  int onCart = 0;

  // References to the model class, these are static because the model class
  // is a singleton
  static bool* isMoneyCurrency;
  static bool* isBuyMode;
  static ItemMarketModel* modelClass;

  // Cached data
  // Many data are very expensive to re-create, we cache them here and re-create
  // them only when needed
  QHash<int, QVariant> cache;
};

#endif // ITEMMARKETENTRY_H
