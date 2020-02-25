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

#include "./itemmarketentry.h"
#include "../itemmarketmodel.h"

ItemMarketEntry::ItemMarketEntry(int compatMoneyCurrency, int compatBuyMode)
  : compatMoneyCurrency(compatMoneyCurrency),
    compatBuyMode(compatBuyMode)
{

}

ItemMarketEntry::~ItemMarketEntry()
{}

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
  if(!cache.contains(HashKeyWhichType))
    cache.insert(HashKeyWhichType, _whichType());

  return cache.value(HashKeyWhichType).toString();
}

int ItemMarketEntry::onCartMax()
{
  if(!cache.contains(HashKeyOnCartMax))
    cache.insert(HashKeyOnCartMax, _onCartMax());

  return cache.value(HashKeyOnCartMax).toInt();
}

bool ItemMarketEntry::requestFilter()
{
  // If compatibility is both either then it's auto true
  if(compatMoneyCurrency == CompatEither && compatBuyMode == CompatEither)
    return true;

  // Here we do a hack, compat values line up with bool values.
  // CompatNo = 0 or false and CompatYes = 1 or true
  // Compare those bool values and check they match the target
  bool _isMoneyCurrency = ((bool)compatMoneyCurrency) == *isMoneyCurrency;
  bool _isBuyMode = ((bool)compatBuyMode) == *isBuyMode;

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
  cache.insert(HashKeyOnCartMax, _onCartMax());

  doReUpdateConstants();
}

bool* ItemMarketEntry::isBuyMode = nullptr;
bool* ItemMarketEntry::isMoneyCurrency = nullptr;
ItemMarketModel* ItemMarketEntry::modelClass = nullptr;
