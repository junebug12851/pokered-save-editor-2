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
#include "./itemmarket/itemmarketentry.h"
#include "./itemmarket/itemmarketentrygcpokemon.h"
#include "./itemmarket/itemmarketentrymessage.h"
#include "./itemmarket/itemmarketentrymoney.h"
#include "./itemmarket/itemmarketentryplayeritem.h"
#include "./itemmarket/itemmarketentrystoreitem.h"
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
  // Setup Item Market Entry globals
  ItemMarketEntry::isMoneyCurrency = &isMoneyCurrency;
  ItemMarketEntry::isBuyMode = &isBuyMode;
  ItemMarketEntry::player = basics;

  // Reset if mode changed or checked out
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::checkedOut, this, &ItemMarketModel::reUpdateAll);

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
    return item->name();
  else if(role == InStockCountRole)
    return item->inStockCount();
  else if(role == CanSellRole)
    return item->canSell();
  else if(role == ItemWorthRole)
    return item->itemWorth();
  else if(role == WhichTypeRole)
    return item->whichType();
  else if(role == StackCountRole)
    return item->stackCount();
  else if(role == OnCartLeftRole)
    return item->onCartLeft();
  else if(role == CartCountRole)
    return item->getCartCount();
  else if(role == CartWorthRole)
    return item->cartWorth();
  else if(role == TotalStackCountRole)
    return item->totalStackCount();
  else if(role == TotalWorthRole)
    return item->totalWorth();
  else if(role == CanCheckoutRole)
    return item->canCheckout();
  else if(role == ValidItemRole)
    return item->requestFilter();
  else if(role == BuyModeRole)
    return isBuyMode;
  else if(role == MoneyCurrencyRole)
    return isMoneyCurrency;

  return QVariant();
}

QHash<int, QByteArray> ItemMarketModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[NameRole] = "dataName";
  roles[InStockCountRole] = "dataInStockCount";
  roles[CanSellRole] = "dataCanSell";
  roles[ItemWorthRole] = "dataItemWorth";
  roles[WhichTypeRole] = "dataWhichType";
  roles[StackCountRole] = "dataStackCount";
  roles[OnCartLeftRole] = "dataOnCartLeft";
  roles[CartCountRole] = "dataCartCount";
  roles[CartWorthRole] = "dataCartWorth";
  roles[TotalStackCountRole] = "dataTotalStackCount";
  roles[TotalWorthRole] = "dataTotalWorth";
  roles[CanCheckoutRole] = "dataCanCheckout";
  roles[ValidItemRole] = "dataValidItem";
  roles[BuyModeRole] = "dataBuyMode";
  roles[MoneyCurrencyRole] = "dataMoneyCurrency";

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
  if (role == CartCountRole) {
    item->setCartCount(value.toInt());
    dataChanged(index, index);
    cartTotalChanged();
    return true;
  }

  return false;
}

int ItemMarketModel::totalCartWorth()
{
  int ret = 0;

  if(isBuyMode)
    ret -= itemListCache.at(0)->totalWorth();
  else
    ret += itemListCache.at(0)->totalWorth();

  return ret;
}

int ItemMarketModel::totalCartCount()
{
  int ret = 0;

  for(auto el : itemListCache) {
    ret += el->onCart;
  }

  return ret;
}

void ItemMarketModel::checkout()
{
  for(auto el : itemListCache) {
    el->checkout();
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

  itemListCache.append(new ItemMarketEntryMessage("Wallet"));
  itemListCache.append(new ItemMarketEntryMoney);

  itemListCache.append(new ItemMarketEntryMessage("Bag"));

  for(auto el: itemBag->items) {
    itemListCache.append(new ItemMarketEntryPlayerItem(itemBag, el));
  }

  itemListCache.append(new ItemMarketEntryMessage("Storage"));

  for(auto el: itemStorage->items) {
    itemListCache.append(new ItemMarketEntryPlayerItem(itemStorage, el));
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

  itemListCache.append(new ItemMarketEntryMessage("Wallet"));
  itemListCache.append(new ItemMarketEntryMoney);

  /////////////////////////////////////////////////

  if(!isMoneyCurrency) {
    itemListCache.append(new ItemMarketEntryMessage("In-House Exclusives"));

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
      itemListCache.append(new ItemMarketEntryStoreItem(el, itemBag, itemStorage));
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
      itemListCache.append(new ItemMarketEntryGCPokemon(el, playerPokemon, storage));
    }

    tmpGC.clear();
  }

  /////////////////////////////////////////////////

  itemListCache.append(new ItemMarketEntryMessage("Normal Items"));

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
    itemListCache.append(new ItemMarketEntryStoreItem(el, itemBag, itemStorage));
  }

  tmp.clear();

  ////////////////////////////////////////////////////

  itemListCache.append(new ItemMarketEntryMessage("Special Items"));

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
    itemListCache.append(new ItemMarketEntryStoreItem(el, itemBag, itemStorage));
  }

  tmp.clear();

  ////////////////////////////////////////////////////

  itemListCache.append(new ItemMarketEntryMessage("Glitch Items"));

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
    itemListCache.append(new ItemMarketEntryStoreItem(el, itemBag, itemStorage));
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
    reUpdateAll();
  }
}
