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

/**
 * @file itemmarketentry.cpp
 * @brief Implementation of ItemMarketEntry (the market-row base). See itemmarketentry.h.
 */

#include <QDebug>

#include "./itemmarketentry.h"
#include "../itemmarketmodel.h"
#include <pse-savefile/expanded/player/playerbasics.h>

ItemMarketEntry::ItemMarketEntry(int compatMoneyCurrency, int compatBuyMode)
  : compatMoneyCurrency(compatMoneyCurrency),
    compatBuyMode(compatBuyMode)
{

}

ItemMarketEntry::~ItemMarketEntry()
{
  // Remove all pointers in array, don't delete them as they are not owned by
  // this class
  auto instArr = instances.value(whichType());
  instArr->removeAll(this);

  // Delete whole instances array if it's empty of that type
  if(instArr->empty()) {
    delete instances.value(whichType());
    instances.remove(whichType());
  }

  // Remove all instances combined, again not deleting them because they are not
  // owned by this class
  instancesCombined.removeAll(this);
}

void ItemMarketEntry::initOnce()
{
  //
}

void ItemMarketEntry::finishConstruction()
{
  if(!instances.contains(whichType())) {
    instances.insert(whichType(), new QVector<ItemMarketEntry*>());
    initOnce();
  }

  auto instArr = instances.value(whichType());
  instArr->append(this);

  instancesCombined.append(this);
}

QString ItemMarketEntry::name()
{
  if(!cache.contains(HashKeyName))
    cache.insert(HashKeyName, _name());

  return cache.value(HashKeyName).toString();
}

int ItemMarketEntry::inStockCount()
{
  if(!cache.contains(HashKeyInStockCount))
    cache.insert(HashKeyInStockCount, _inStockCount());

  return cache.value(HashKeyInStockCount).toInt();
}

bool ItemMarketEntry::canSell()
{
  if(!cache.contains(HashKeyCanSell))
    cache.insert(HashKeyCanSell, _canSell());

  return cache.value(HashKeyCanSell).toBool();
}

int ItemMarketEntry::itemWorth()
{
  if(!cache.contains(HashKeyItemWorth))
    cache.insert(HashKeyItemWorth, _itemWorth());

  return cache.value(HashKeyItemWorth).toInt();
}

QString ItemMarketEntry::whichType()
{
  // LATENT-UB NOTE (flagged for review — see notes/plans/testing.md): the base
  // ~ItemMarketEntry() destructor calls whichType() for cleanup. If whichType()
  // was ALREADY called during the object's life (the normal case — the model uses
  // it for grouping/filtering), the destructor hits the cache below and never
  // reaches _whichType(), so it's safe. But if an entry were destroyed WITHOUT
  // whichType() ever being called, _whichType() (pure virtual in this base) would
  // be invoked after the derived part is gone == undefined behavior. clang-tidy
  // (clang-analyzer-cplusplus.PureVirtualCall) correctly flags the latent path.
  // A real fix means caching the type as a plain member set by the derived class
  // (the ctor can't call it either) -- a lifetime refactor of this UAF-historied
  // area, deferred to Twilight rather than changed unilaterally. Suppressed here
  // because the cached-by-then invariant holds in every real code path.
  if(!cache.contains(HashKeyWhichType))
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.PureVirtualCall)
    cache.insert(HashKeyWhichType, _whichType());

  return cache.value(HashKeyWhichType).toString();
}

bool ItemMarketEntry::requestFilter()
{
  // If compatibility is both either then it's auto true
  if(compatMoneyCurrency == CompatEither && compatBuyMode == CompatEither)
    return true;

  // Here we do a hack, compat values line up with bool values.
  // CompatNo = 0 or false and CompatYes = 1 or true
  // Compare those bool values and check they match the target
  bool _isMoneyCurrency = ((bool)compatMoneyCurrency) == (*isMoneyCurrency);
  bool _isBuyMode = ((bool)compatBuyMode) == (*isBuyMode);

  // Now if either one was CompatEither, they would have yielded incorrect
  // results. We auto true any that is compat either.
  if(compatMoneyCurrency == CompatEither)
    _isMoneyCurrency = true;

  if(compatBuyMode == CompatEither)
    _isBuyMode = true;

  // After all is said and done, we simply check both are true meaning this
  // request passes the filter
  return _isMoneyCurrency && _isBuyMode;
}

int ItemMarketEntry::getCartCount()
{
  return onCart;
}

int ItemMarketEntry::cartWorth()
{
  if(!requestFilter())
    return 0;

  return itemWorth() * onCart;
}

int ItemMarketEntry::totalStackCount()
{
  int ret = 0;

  if(activeList == nullptr)
    return 0;

  // Only same-type rows in the CURRENT list (was: the cross-app `instances` registry).
  const QString myType = whichType();
  for(auto el : *activeList) {
    if(el->whichType() == myType)
      ret += el->stackCount();
  }

  return ret;
}

// Signed net worth of the whole cart in its single currency: sells add, buys
// subtract. (Money/exchange rows are excluded -- the exchange is its own mode.)
// Uses the plain cartSignVal member, NOT a virtual, so sweeping a registry that may
// hold a torn-down entry can't deref a freed vtable.
int ItemMarketEntry::totalWorth()
{
  int ret = 0;

  if(activeList == nullptr)
    return 0;

  for(auto el : *activeList) {
    if(el->exclude)
      continue;

    ret += el->cartSignVal * el->cartWorth();
  }

  return ret;
}

// Balance after the whole cart: starting balance in the active currency plus the
// signed net (sells already +, buys already -). One formula for a mixed cart.
int ItemMarketEntry::moneyLeftover()
{
  const int start = (*isMoneyCurrency) ? (int)player->money : (int)player->coins;
  return start + totalWorth();
}

bool ItemMarketEntry::canCheckout()
{
  if(!requestFilter())
    return false;

  // Allow only if there is space left and the total worth doesn't go above
  // the players money/coins
  return (onCart > 0) && (onCartLeft() >= 0) && (moneyLeftover() >= 0);
}

bool ItemMarketEntry::canAnyCheckout()
{
  if(activeList == nullptr)
    return false;

  for(auto el : *activeList) {
    if(el->canCheckout())
      return true;
  }

  return false;
}

void ItemMarketEntry::setCartCount(int val)
{
  onCart = val;

  if(onCart < 0)
    onCart = 0;

  onCartChanged();
}

void ItemMarketEntry::reUpdateConstants()
{
  cache.insert(HashKeyName, _name());
  cache.insert(HashKeyInStockCount, _inStockCount());
  cache.insert(HashKeyCanSell, _canSell());
  cache.insert(HashKeyItemWorth, _itemWorth());
  cache.insert(HashKeyWhichType, _whichType());

  doReUpdateConstants();
}

bool* ItemMarketEntry::isBuyMode = nullptr;
bool* ItemMarketEntry::isMoneyCurrency = nullptr;
PlayerBasics* ItemMarketEntry::player = nullptr;

QHash<QString, QVector<ItemMarketEntry*>*> ItemMarketEntry::instances =
    QHash<QString, QVector<ItemMarketEntry*>*>();

QVector<ItemMarketEntry*> ItemMarketEntry::instancesCombined =
    QVector<ItemMarketEntry*>();

QVector<ItemMarketEntry*>* ItemMarketEntry::activeList = nullptr;
