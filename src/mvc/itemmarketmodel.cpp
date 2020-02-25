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
#include "../bridge/router.h"
#include "../data/db/gamecorner.h"
#include "../data/file/expanded/storage.h"
#include "../data/file/expanded/player/playerpokemon.h"
#include "../data/file/expanded/fragments/item.h"
#include "../data/file/expanded/fragments/pokemonbox.h"
#include "../data/file/expanded/fragments/pokemonparty.h"
#include "../data/file/expanded/fragments/pokemonstorageset.h"
#include "../data/file/expanded/fragments/pokemonstoragebox.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"
#include "../data/file/expanded/player/playerbasics.h"

ItemMarketSelectEntryData::ItemMarketSelectEntryData(PlayerBasics* data)
  : basics(data)
{}

QString ItemMarketSelectEntryData::name(bool isMoneyCurrency)
{
  // Player Money
  else if(basics != nullptr) {
    return (isMoneyCurrency)
        ? "Coins"
        : "Money";
  }

  return "";
}

int ItemMarketSelectEntryData::amount(bool isMoneyCurrency)
{
  // Player Money
  if(basics != nullptr) {
    return (isMoneyCurrency)
        ? basics->money
        : basics->coins;
  }

  return 0;
}

bool ItemMarketSelectEntryData::canSell()
{
  // Player Money
  else if(basics != nullptr) {
    return true;
  }

  return false;
}

int ItemMarketSelectEntryData::valueOne(bool isMoneyCurrency, bool isBuyMode)
{
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

int ItemMarketSelectEntryData::whichType()
{
  // Player Money
  else if(basics != nullptr) {
    return TypeCurrency;
  }
}

ItemMarketModel::ItemMarketModel(ItemStorageBox* itemBag,
                                 ItemStorageBox* itemStorage,
                                 PlayerBasics* basics,
                                 Router* router,
                                 PlayerPokemon* playerPokemon,
                                 Storage* storage)
  : itemBag(itemBag),
    itemStorage(itemStorage),
    router(router),
    basics(basics),
    playerPokemon(playerPokemon),
    storage(storage)
{
  // Reset if mode changed
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::reUpdateAll);

  // Reset if items changed such as file/sav data changing while page is open
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

int ItemMarketModel::cartCount()
{
  int ret = 0;

  for(auto el : itemListCache) {
    ret += el->onCart;
  }

  return ret;
}

void ItemMarketModel::checkout()
{
  // Do nothing if cart is empty
  if(cartCount() == 0)
    return;

  // Process cart items
  for(auto el : itemListCache) {

    int whichType = el->whichType();

    // Buying or Selling Currency
    if(whichType == ItemMarketSelectEntryData::TypeCurrency) {
      // Sell Coins to Buy Money
      if(isBuyMode && isMoneyCurrency) {
        basics->money += el->valueAll(isMoneyCurrency, isBuyMode);
        basics->coins -= el->valueAll(isMoneyCurrency, isBuyMode);
      }

      // Sell Money to Buy Coins
      else if(isBuyMode && !isMoneyCurrency) {
        basics->money -= el->valueAll(isMoneyCurrency, isBuyMode);
        basics->coins += el->valueAll(isMoneyCurrency, isBuyMode);
      }

      // Buy Coins from selling money
      else if(!isBuyMode && isMoneyCurrency) {
        basics->money -= el->valueAll(isMoneyCurrency, isBuyMode);
        basics->coins += el->valueAll(isMoneyCurrency, isBuyMode);
      }

      // Buy Money from selling coins
      else if(!isBuyMode && !isMoneyCurrency) {
        basics->money += el->valueAll(isMoneyCurrency, isBuyMode);
        basics->coins -= el->valueAll(isMoneyCurrency, isBuyMode);
      }
    }
  }
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

  cartTotalChanged();
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
  if(isBuyMode != false || isMoneyCurrency != true) {
    isBuyMode = false;
    isMoneyCurrency = true;
    buildList();
  }
}
