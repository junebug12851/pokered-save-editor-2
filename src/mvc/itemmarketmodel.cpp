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

#include <algorithm>
#include <QCollator>

#include "./itemmarketmodel.h"
#include "../data/db/items.h"
#include "../data/db/gamecorner.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"
#include "../data/file/expanded/fragments/item.h"
#include "../data/file/expanded/player/playerbasics.h"
#include "../bridge/router.h"

ItemMarketSelectEntryData::ItemMarketSelectEntryData(ItemStorageBox* toBox, Item* toItem, int onCart)
  : onCart(onCart),
    toBox(toBox),
    toItem(toItem)
{}

ItemMarketSelectEntryData::ItemMarketSelectEntryData(ItemDBEntry* data)
  : toData(data)
{}

ItemMarketSelectEntryData::ItemMarketSelectEntryData(GameCornerDBEntry* data)
  : toGameCorner(data)
{}

ItemMarketSelectEntryData::ItemMarketSelectEntryData(PlayerBasics* data)
  : basics(data)
{}

ItemMarketSelectEntryData::ItemMarketSelectEntryData(QString msg)
  : msg(msg)
{}

QString ItemMarketSelectEntryData::name(bool isMoneyCurrency)
{
  // An item the player has
  if(toItem != nullptr) {
    auto itemData = toItem->toItem();
    if(itemData == nullptr)
      return "";

    return itemData->readable;
  }

  // An item the player can buy
  else if(toData != nullptr) {
    return toData->readable;
  }

  // A pokemon the player can buy
  else if(toGameCorner != nullptr) {
    return toGameCorner->name;
  }

  // Player Money
  else if(basics != nullptr) {
    return (isMoneyCurrency)
        ? "Coins"
        : "Money";
  }

  // A Text Message or some kind of error in whcih case this will suffice
  return msg;
}

int ItemMarketSelectEntryData::amount(bool isMoneyCurrency)
{
  // An item the player has
  if(toItem != nullptr) {
    return toItem->amount;
  }

  // Player Money
  else if(basics != nullptr) {
    return (isMoneyCurrency)
        ? basics->money
        : basics->coins;
  }

  return 0;
}

int ItemMarketSelectEntryData::cartAmount()
{
  return onCart;
}

void ItemMarketSelectEntryData::setCartAmount(int val)
{
  if(val < 0)
    val = 0;

  this->onCart = val;
}

bool ItemMarketSelectEntryData::canSell()
{
  // An item the player has
  if(toItem != nullptr) {
    auto itemData = toItem->toItem();
    if(itemData == nullptr)
      return false;

    return itemData->canSell();
  }

  // An item the player can buy
  else if(toData != nullptr) {
    return toData->canSell();
  }

  // Player Money
  else if(basics != nullptr) {
    return true;
  }

  return false;
}

int ItemMarketSelectEntryData::valueOne(bool isMoneyCurrency, bool isBuyMode)
{
  // An item the player has
  if(toItem != nullptr) {
    if(isMoneyCurrency && isBuyMode)
      return toItem->buyPriceOneMoney();
    else if(isMoneyCurrency && !isBuyMode)
      return toItem->sellPriceOneMoney();
    else if(!isMoneyCurrency && isBuyMode)
      return toItem->buyPriceOneCoins();
    else if(!isMoneyCurrency && !isBuyMode)
      return toItem->sellPriceOneCoins();
  }

  // An item the player can buy
  else if(toData != nullptr) {
    if(isMoneyCurrency && isBuyMode)
      return toData->buyPriceMoney();
    else if(isMoneyCurrency && !isBuyMode)
      return toData->sellPriceMoney();
    else if(!isMoneyCurrency && isBuyMode)
      return toData->buyPriceCoins();
    else if(!isMoneyCurrency && !isBuyMode)
      return toData->sellPriceCoins();
  }

  // A pokemon the player can buy
  else if(toGameCorner != nullptr) {
    if(!isMoneyCurrency && isBuyMode)
      return toGameCorner->price;
    else if(!isMoneyCurrency && !isBuyMode)
      return toGameCorner->price / 2;
  }

  // Player Money
  else if(basics != nullptr) {
    if(isMoneyCurrency && isBuyMode)
      return GameCornerDB::buyPrice;
    else if(isMoneyCurrency && !isBuyMode)
      return GameCornerDB::sellPrice;
    else if(!isMoneyCurrency && isBuyMode)
      return GameCornerDB::sellPrice;
    else if(!isMoneyCurrency && !isBuyMode)
      return GameCornerDB::buyPrice;
  }

  return 0;
}

int ItemMarketSelectEntryData::valueAll(bool isMoneyCurrency, bool isBuyMode)
{
  return valueOne(isMoneyCurrency, isBuyMode) * cartAmount();
}

int ItemMarketSelectEntryData::whichType()
{
  // An item the player has
  if(toItem != nullptr) {
    return TypePlayerItem;
  }

  // An item the player can buy
  else if(toData != nullptr) {
    return TypeStoreItem;
  }

  // A pokemon the player can buy
  else if(toGameCorner != nullptr) {
    return TypeGCPokemon;
  }

  // Player Money
  else if(basics != nullptr) {
    return TypeCurrency;
  }

  // A Text Message or some kind of error in whcih case this will suffice
  return TypeMessage;
}

ItemMarketModel::ItemMarketModel(ItemStorageBox* itemBag, ItemStorageBox* itemStorage, PlayerBasics* basics, Router* router)
  : itemBag(itemBag),
    itemStorage(itemStorage),
    router(router),
    basics(basics)
{
  // Reset if mode changed
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::reUpdateAll);

  // Reset if items changed
  connect(itemBag, &ItemStorageBox::itemsChanged, this, &ItemMarketModel::reUpdateAll);

  // Cleanup on page close
  connect(this->router, &Router::closeNonModal, this, &ItemMarketModel::pageClosing);
  connect(this->router, &Router::goHome, this, &ItemMarketModel::pageClosing);
}

int ItemMarketModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return itemListCache.size();
}

QVariant ItemMarketModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= itemListCache.size())
    return QVariant();

  // Get entry from Item List Cache
  // An entry can be one of several types
  // * An item the player has which may or may not be sellable
  // * An item the player can buy
  // * A Game Corner Pokemon
  // * Player Money / Coins
  // * A Message
  auto item = itemListCache.at(index.row());

  if(item == nullptr)
    return QVariant();

  // Now return requested information
  if (role == NameRole)
    return item->name(isMoneyCurrency);
  else if (role == AmountRole)
    return item->amount(isMoneyCurrency);
  else if (role == CartAmountRole) // Editable
    return item->cartAmount();
  else if (role == CanSellRole)
    return item->canSell();
  else if (role == ValueOneRole)
    return item->valueOne(isMoneyCurrency, isBuyMode);
  else if (role == ValueAllRole)
    return item->valueAll(isMoneyCurrency, isBuyMode);
  else if (role == BuyModeRole)
    return isBuyMode;
  else if (role == MoneyCurrencyRole)
    return isMoneyCurrency;
  else if (role == TypeRole)
    return item->whichType();

  return QVariant();
}

QHash<int, QByteArray> ItemMarketModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[NameRole] = "dataName";
  roles[AmountRole] = "dataAmount";
  roles[CartAmountRole] = "dataCartAmount";
  roles[CanSellRole] = "dataCanSell";
  roles[ValueOneRole] = "dataValueOne";
  roles[ValueAllRole] = "dataValueAll";
  roles[BuyModeRole] = "dataBuyMode";
  roles[MoneyCurrencyRole] = "dataMoneyCurrencyRole";
  roles[TypeRole] = "dataType";

  return roles;
}

bool ItemMarketModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(!index.isValid())
    return false;

  if(index.row() >= itemListCache.size())
    return false;

  auto item = itemListCache.at(index.row());

  if(item == nullptr)
    return false;

  // Now set requested information
  if (role == CartAmountRole) {
    item->setCartAmount(value.toInt());
    dataChanged(index, index);
    cartTotalChanged();
    return true;
  }

  return false;
}

int ItemMarketModel::cartTotal()
{
  int ret = 0;

  for(auto el : itemListCache) {
    if(isBuyMode)
      ret -= el->valueAll(isMoneyCurrency, isBuyMode);
    else
      ret += el->valueAll(isMoneyCurrency, isBuyMode);
  }

  return ret;
}

void ItemMarketModel::clearList()
{
  for(auto el: itemListCache)
    delete el;

  itemListCache.clear();
}

void ItemMarketModel::buildList()
{
  if(isBuyMode)
    buildMartItemList();
  else
    buildPlayerItemList();
}

void ItemMarketModel::buildPlayerItemList()
{
  clearList();

  itemListCache.append(new ItemMarketSelectEntryData("Wallet"));
  itemListCache.append(new ItemMarketSelectEntryData(basics));

  itemListCache.append(new ItemMarketSelectEntryData("Bag"));

  for(auto el: itemBag->items) {
    itemListCache.append(new ItemMarketSelectEntryData(itemBag, el));
  }

  itemListCache.append(new ItemMarketSelectEntryData("Storage"));

  for(auto el: itemStorage->items) {
    itemListCache.append(new ItemMarketSelectEntryData(itemStorage, el));
  }
}

void ItemMarketModel::buildMartItemList()
{
  clearList();

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Gather normal repeatable items and sort by name, then add into list
  QVector<ItemDBEntry*> tmp;
  QVector<GameCornerDBEntry*> tmpGC;

  itemListCache.append(new ItemMarketSelectEntryData("Wallet"));
  itemListCache.append(new ItemMarketSelectEntryData(basics));

  /////////////////////////////////////////////////

  if(!isMoneyCurrency) {
    itemListCache.append(new ItemMarketSelectEntryData("In-House Exclusives"));

    for(auto el : ItemsDB::store) {
      if(el->isGameCornerExclusive())
        tmp.append(el);
    }

    std::sort(
        tmp.begin(),
        tmp.end(),
        [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
        {
            return collator.compare(item1->readable, item2->readable) < 0;
        });

    for(auto el : tmp) {
      itemListCache.append(new ItemMarketSelectEntryData(el));
    }

    tmp.clear();

    //////////////////////////////////////////////

    for(auto el : GameCornerDB::store) {
      if(el->type == "pokemon")
        tmpGC.append(el);
    }

    std::sort(
        tmpGC.begin(),
        tmpGC.end(),
        [&collator](const GameCornerDBEntry* item1, const GameCornerDBEntry* item2)
        {
            return collator.compare(item1->name, item2->name) < 0;
        });

    for(auto el : tmpGC) {
      itemListCache.append(new ItemMarketSelectEntryData(el));
    }

    tmpGC.clear();
  }

  /////////////////////////////////////////////////

  itemListCache.append(new ItemMarketSelectEntryData("Normal Items"));

  for(auto el : ItemsDB::store) {
    if(!isMoneyCurrency) {
      if(!el->once && !el->glitch && el->canSell() && !el->isGameCornerExclusive())
        tmp.append(el);
    }
    else if(!el->once && !el->glitch && el->canSell())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemMarketSelectEntryData(el));
  }

  tmp.clear();

  ////////////////////////////////////////////////////

  itemListCache.append(new ItemMarketSelectEntryData("Special Items"));

  for(auto el : ItemsDB::store) {
    if(!isMoneyCurrency) {
      if(el->once && !el->glitch && el->canSell() && !el->isGameCornerExclusive())
        tmp.append(el);
    }
    else if(el->once && !el->glitch && el->canSell())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemMarketSelectEntryData(el));
  }

  tmp.clear();

  ////////////////////////////////////////////////////

  itemListCache.append(new ItemMarketSelectEntryData("Glitch Items"));

  for(auto el : ItemsDB::store) {
    if(!isMoneyCurrency) {
      if(el->glitch && el->canSell() && !el->isGameCornerExclusive())
        tmp.append(el);
    }
    else if(el->glitch && el->canSell())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemMarketSelectEntryData(el));
  }

  tmp.clear();
}

void ItemMarketModel::reUpdateAll()
{
  beginResetModel();
  buildList();
  endResetModel();
  cartTotalChanged();
}

void ItemMarketModel::pageClosing()
{
  clearList();
  isBuyMode = false;
  isMoneyCurrency = true;
}
