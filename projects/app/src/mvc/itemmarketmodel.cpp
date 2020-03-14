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
#include <pse-db/items.h>
#include "../bridge/router.h"
#include <pse-db/gamecorner.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>

ItemMarketModel::ItemMarketModel(ItemStorageBox* itemBag,
                                 ItemStorageBox* itemStorage,
                                 PlayerBasics* basics,
                                 Router* router,
                                 PlayerPokemon* playerPokemon,
                                 Storage* storage,
                                 SaveFile* file)
  : itemBag(itemBag),
    itemStorage(itemStorage),
    router(router),
    basics(basics),
    playerPokemon(playerPokemon),
    storage(storage),
    file(file)
{
  // Setup Item Market Entry globals
  ItemMarketEntry::isMoneyCurrency = &isMoneyCurrency;
  ItemMarketEntry::isBuyMode = &isBuyMode;
  ItemMarketEntry::player = basics;

  // Reset if mode changed or expanded data has been reset or changed in any way
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::reUpdateAll);
  connect(file, &SaveFile::dataExpandedChanged, this, &ItemMarketModel::reUpdateAll);

  // Cleanup and reset on page open
  connect(this->router, &Router::openNonModal, this, &ItemMarketModel::pageOpening);

  // Any mode change
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::isAnyChanged);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::isAnyChanged);

  connect(this, &ItemMarketModel::reUpdateValues, this, &ItemMarketModel::onReUpdateValues);
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
  else if(role == ExcludeItemRole)
    return item->exclude;
  else if(role == ExcludeItemRole)
    return item->moneyLeftover();
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
  roles[ExcludeItemRole] = "dataExcludeItem";
  roles[MoneyLeftRole] = "dataMoneyLeft";
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
    reUpdateValues();
    return true;
  }

  return false;
}

int ItemMarketModel::totalCartWorth()
{
  return itemListCache.at(0)->totalWorth();
}

int ItemMarketModel::totalCartCount()
{
  int ret = 0;

  for(auto el : itemListCache) {

    // Money exchanging is something else entirely, it can't be mixed in with
    // everything else because they're 2 very different things. They're properly
    // part of one transaction and all in one cart, but they're added
    // differently.
    if(el->exclude)
      continue;

    ret += el->onCart;
  }

  return ret;
}

int ItemMarketModel::whichMode()
{
  if(isBuyMode && isMoneyCurrency)
    return SelBuyMoney;
  else if(isBuyMode && !isMoneyCurrency)
    return SelBuyCoins;
  else if(!isBuyMode && isMoneyCurrency)
    return SelSellMoney;
  else if(!isBuyMode && !isMoneyCurrency)
    return SelSellCoins;

  return 0;
}

int ItemMarketModel::moneyStart()
{
  if(isMoneyCurrency)
    return basics->money;
  else
    return basics->coins;
}

int ItemMarketModel::moneyLeftover()
{
  return itemListCache.at(0)->moneyLeftover();
}

bool ItemMarketModel::anyNotEnoughSpace()
{
  int ret = false;

  for(auto el : itemListCache) {

    if(el->exclude)
      continue;

    if(el->onCartLeft() < 0) {
      ret = true;
      break;
    }
  }

  return ret;
}

bool ItemMarketModel::canAnyCheckout()
{
  return itemListCache.at(0)->canAnyCheckout();
}

void ItemMarketModel::onReUpdateValues()
{
  dataChanged(index(0), index(itemListCache.size() - 1));
}

bool ItemMarketModel::vendorListItem(ItemDBEntry* el)
{
  if(isMoneyCurrency)
    return el->canSell() && (el->buyPriceMoney() > 0) && !el->isGameCornerExclusive();

  return el->canSell() && (el->buyPriceCoins() > 0) && !el->isGameCornerExclusive();
}

void ItemMarketModel::checkout()
{
  // Perform checkout
  for(auto el : itemListCache) {
    el->checkout();
  }

  // Refresh/Reset the list
  reUpdateAll();

  // Annouce changes and update other values
  reUpdateValues();
}

void ItemMarketModel::clearList()
{
  for(auto el: itemListCache)
    el->deleteLater();

  itemListCache.clear();
}

void ItemMarketModel::buildList()
{
  if(isBuyMode)
    buildMartItemList();
  else
    buildPlayerItemList();

  reUpdateValues();
}

void ItemMarketModel::buildPlayerItemList()
{
  clearList();

  if(!isMoneyCurrency) {
    itemListCache.append(new ItemMarketEntryMessage("Money Exchange"));
    itemListCache.append(new ItemMarketEntryMoney);
  }

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

  if(!isMoneyCurrency) {
    itemListCache.append(new ItemMarketEntryMessage("Money Exchange"));
    itemListCache.append(new ItemMarketEntryMoney);
  }

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Gather normal repeatable items and sort by name, then add into list
  QVector<ItemDBEntry*> tmp;
  QVector<GameCornerDBEntry*> tmpGC;

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
        itemListCache.append(new ItemMarketEntryGCPokemon(el, playerPokemon, storage));
    }
  }

  /////////////////////////////////////////////////

  itemListCache.append(new ItemMarketEntryMessage("Normal Items"));

  for(auto el : ItemsDB::store) {
    if(!el->once && !el->glitch && vendorListItem(el))
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
    if(el->glitch && vendorListItem(el))
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
}

void ItemMarketModel::pageOpening(QString path)
{
  // Do nothing unless Trainer Mart Screen
  if(path != "qrc:/ui/app/screens/non-modal/Pokemart.qml")
    return;

  // Re-Build List to account for changes
  isBuyMode = false;
  isBuyModeChanged();

  isMoneyCurrency = true;
  isMoneyCurrencyChanged();

  //reUpdateAll();
}
