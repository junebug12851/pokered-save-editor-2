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
 * @file itemmarketmodel.cpp
 * @brief Implementation of ItemMarketModel -- the buy/sell market + cart logic.
 *        See itemmarketmodel.h.
 */

#include <algorithm>
#include <QCollator>
#include <QSet>

#include "./itemmarketmodel.h"
#include "./itemmarket/itemmarketentry.h"
#include "./itemmarket/itemmarketentrygcpokemon.h"
#include "./itemmarket/itemmarketentrymessage.h"
#include "./itemmarket/itemmarketentrymoney.h"
#include "./itemmarket/itemmarketentryplayeritem.h"
#include "./itemmarket/itemmarketentrystoreitem.h"
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-db/entries/gamecornerdbentry.h>
#include "../bridge/router.h"
#include <pse-db/gamecornerdb.h>
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

  // Rebuild on a currency or exchange-mode change (a different cart) or on a data
  // reset. NOT on isBuyMode: Buy/Sell only filters the left VIEW now (the cart holds
  // both), so toggling it must NOT rebuild/clear the cart -- the view proxy just
  // re-filters off isBuyModeChanged.
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::reUpdateAll);
  connect(this, &ItemMarketModel::isExchangeModeChanged, this, &ItemMarketModel::reUpdateAll);
  connect(file, &SaveFile::dataExpandedChanged, this, &ItemMarketModel::reUpdateAll);

  // Cleanup and reset on page open
  connect(this->router, &Router::openNonModal, this, &ItemMarketModel::pageOpening);

  // Any mode change
  connect(this, &ItemMarketModel::isBuyModeChanged, this, &ItemMarketModel::isAnyChanged);
  connect(this, &ItemMarketModel::isMoneyCurrencyChanged, this, &ItemMarketModel::isAnyChanged);
  connect(this, &ItemMarketModel::isExchangeModeChanged, this, &ItemMarketModel::isAnyChanged);

  connect(this, &ItemMarketModel::reUpdateValues, this, &ItemMarketModel::onReUpdateValues);

  // Build the list up front so the model is in a valid, populated state from
  // construction. Otherwise the cache stays empty until the first dataExpandedChanged
  // or Pokemart page-open, and a QML binding that reads an aggregate accessor before
  // then (the order in which the navigation signal reaches this model vs. the screen's
  // StackView push is not guaranteed) would hit an empty list. reUpdateAll on
  // dataExpandedChanged and pageOpening still refresh it for the current save/mode.
  buildList();
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
  else if(role == ViewTagRole)
    return item->viewTag;
  else if(role == CartSignRole)
    return item->cartSignVal;
  else if(role == MoneyDirRole) {
    auto money = qobject_cast<ItemMarketEntryMoney*>(item);
    return money ? money->forceDir : -1;
  }

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
  roles[ViewTagRole] = "dataViewTag";
  roles[CartSignRole] = "dataCartSign";
  roles[MoneyDirRole] = "dataMoneyDir";

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
  // Aggregate carried on entry 0; an empty list is an empty cart -> zero worth.
  if(itemListCache.isEmpty())
    return 0;
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
  // Every entry exposes the same model-wide leftover, so entry 0 is the proxy; with
  // no entries the cart is empty, so nothing is spent and all the currency remains.
  if(itemListCache.isEmpty())
    return moneyStart();
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
  // Aggregate carried on entry 0; with no entries there is nothing to check out.
  if(itemListCache.isEmpty())
    return false;
  return itemListCache.at(0)->canAnyCheckout();
}

int ItemMarketModel::exchangeMoneyStart()
{
  return basics->money;
}

int ItemMarketModel::exchangeCoinsStart()
{
  return basics->coins;
}

// Money on hand after applying every carted swap -- computed by mirroring
// ItemMarketEntryMoney::checkout() exactly (buy spends onCart money + gains coins;
// sell gains cartWorth money), so the preview can never disagree with the commit.
int ItemMarketModel::exchangeMoneyAfter()
{
  int m = basics->money;

  for(auto el : itemListCache) {
    auto money = qobject_cast<ItemMarketEntryMoney*>(el);
    if(money != nullptr)
      m += money->moneyDelta();   // signed: -cost buying, +gain selling
  }

  return m;
}

int ItemMarketModel::exchangeCoinsAfter()
{
  int c = basics->coins;

  for(auto el : itemListCache) {
    auto money = qobject_cast<ItemMarketEntryMoney*>(el);
    if(money != nullptr)
      c += money->coinsDelta();   // signed: +buying, -selling
  }

  return c;
}

int ItemMarketModel::exchangeBuyRate()
{
  return GameCornerDB::inst()->getBuyPrice();
}

int ItemMarketModel::exchangeSellRate()
{
  return GameCornerDB::inst()->getSellPrice();
}

// The two exchange rows are one net axis: +N = buying N coins (the Money=>Coins row),
// -N = selling N coins (the Coins=>Money row). The +Coins / +Money buttons nudge this.
int ItemMarketModel::exchangeNet()
{
  int net = 0;
  for(auto el : itemListCache) {
    auto money = qobject_cast<ItemMarketEntryMoney*>(el);
    if(money == nullptr)
      continue;
    net += money->buying() ? money->onCart : -money->onCart;
  }
  return net;
}

void ItemMarketModel::exchangeAdjust(int deltaCoins)
{
  ItemMarketEntryMoney* buyRow = nullptr;   // Money => Coins
  ItemMarketEntryMoney* sellRow = nullptr;  // Coins => Money
  for(auto el : itemListCache) {
    auto money = qobject_cast<ItemMarketEntryMoney*>(el);
    if(money == nullptr)
      continue;
    if(money->buying()) buyRow = money; else sellRow = money;
  }
  if(buyRow == nullptr || sellRow == nullptr)
    return;

  // Work out the requested new net, then clamp it to what each side allows.
  const int net = buyRow->onCart - sellRow->onCart;
  int want = net + deltaCoins;

  // Start both at zero so onCartLeft() reads the full available range per side.
  buyRow->onCart = 0;
  sellRow->onCart = 0;

  if(want > 0) {
    buyRow->onCart = qMin(want, buyRow->onCartLeft());   // cap at affordable / coin-cap
  } else if(want < 0) {
    sellRow->onCart = qMin(-want, sellRow->onCartLeft()); // cap at owned / money-cap
  }

  buyRow->onCartChanged();
  sellRow->onCartChanged();
  reUpdateValues();
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

// Is this item stocked by a *reachable* Poke-Mart in Gen 1 R/B? "Buyable" means it
// appears in the union of every referenced mart inventory in pokered
// data/items/marts.asm. The two unused/unreferenced clerks (the bike shop and a
// stray spare-mart text) are deliberately excluded -- their stock is never
// purchasable in-game, and every item they list is sold elsewhere anyway.
//
// Indices are the stable game item IDs (ItemDBEntry::getInd), so this needs no JSON
// data. A priced item that is NOT here is sellable-only ("Unbuyable Items"); a buyer
// can BUY *and* SELL anything that is here ("Normal Items").
bool ItemMarketModel::buyableInMart(ItemDBEntry* el) const
{
  static const QSet<int> kMartStock = {
    4,   // POKE BALL
    3,   // GREAT BALL
    2,   // ULTRA BALL
    20,  // POTION
    19,  // SUPER POTION
    18,  // HYPER POTION
    17,  // MAX POTION
    16,  // FULL RESTORE
    11,  // ANTIDOTE
    12,  // BURN HEAL
    13,  // ICE HEAL
    14,  // AWAKENING
    15,  // PARLYZ HEAL
    52,  // FULL HEAL
    53,  // REVIVE
    29,  // ESCAPE ROPE
    30,  // REPEL
    56,  // SUPER REPEL
    57,  // MAX REPEL
    32,  // FIRE STONE
    33,  // THUNDER STONE
    34,  // WATER STONE
    47,  // LEAF STONE
    35,  // HP UP
    36,  // PROTEIN
    37,  // IRON
    38,  // CARBOS
    39,  // CALCIUM
    46,  // X ACCURACY
    55,  // GUARD SPEC
    58,  // DIRE HIT
    65,  // X ATTACK
    66,  // X DEFEND
    67,  // X SPEED
    68,  // X SPECIAL
    51,  // POKE DOLL
    201, // TM01 MEGA PUNCH    (Celadon Dept. TM counter)
    202, // TM02 RAZOR WIND
    205, // TM05 MEGA KICK
    207, // TM07 HORN DRILL
    209, // TM09 TAKE DOWN
    217, // TM17 SUBMISSION
    232, // TM32 DOUBLE TEAM
    233, // TM33 REFLECT
    237, // TM37 EGG BOMB
  };

  return kMartStock.contains(el->getInd());
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
  clearList();

  // Claim the aggregate sweep for THIS model's list. The entry-level totals
  // (totalWorth / canAnyCheckout / totalStackCount) iterate activeList, so point it
  // at our live cache -- never the global cross-model registry (use-after-free).
  ItemMarketEntry::activeList = &itemListCache;

  // Exchange is its own (separate-currency) list. Otherwise the cart is a single
  // currency holding BOTH the sell rows (your items) and the buy rows (the store);
  // the Buy/Sell strip only filters the left VIEW (see marketViewModel), it does
  // not split the cart -- so both are built into one list here.
  if(isExchangeMode) {
    buildExchangeList();
  } else {
    buildPlayerItemList(); // sell rows (tagged ViewSell)
    buildMartItemList();   // buy rows (tagged ViewBuy)
  }

  reUpdateValues();
}

// The money<->coins exchange as its own list: both swap directions at once
// (Money=>Coins and Coins=>Money), each a fixed-direction money row. Pulled out of
// the buy/sell lists so those stay pure item lists. (Caller clears the list.)
void ItemMarketModel::buildExchangeList()
{
  itemListCache.append(new ItemMarketEntryMessage("Coin Exchange"));
  itemListCache.append(new ItemMarketEntryMoney(ItemMarketEntryMoney::DirToCoins));
  itemListCache.append(new ItemMarketEntryMoney(ItemMarketEntryMoney::DirToMoney));
}

// Sell rows (your bag + storage items). Caller clears the list; the whole appended
// range is tagged ViewSell so the Buy/Sell strip can filter the left view.
void ItemMarketModel::buildPlayerItemList()
{
  const int from = itemListCache.size();

  itemListCache.append(new ItemMarketEntryMessage("Bag"));

  for(auto el: itemBag->items) {
    itemListCache.append(new ItemMarketEntryPlayerItem(itemBag, el));
  }

  itemListCache.append(new ItemMarketEntryMessage("Storage"));

  for(auto el: itemStorage->items) {
    itemListCache.append(new ItemMarketEntryPlayerItem(itemStorage, el));
  }

  for(int i = from; i < itemListCache.size(); i++)
    itemListCache[i]->viewTag = ViewSell;
}

// Buy rows (the store stock for the active currency). Caller clears the list; the
// whole appended range is tagged ViewBuy.
void ItemMarketModel::buildMartItemList()
{
  const int from = itemListCache.size();

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  auto sortByName = [&collator](QVector<ItemDBEntry*>& v)
  {
    std::sort(v.begin(), v.end(),
              [&collator](const ItemDBEntry* a, const ItemDBEntry* b)
    {
      return collator.compare(a->getReadable(), b->getReadable()) < 0;
    });
  };

  auto appendItems = [this](const QVector<ItemDBEntry*>& v)
  {
    for(auto el : v)
      itemListCache.append(new ItemMarketEntryStoreItem(el, itemBag, itemStorage));
  };

  QVector<ItemDBEntry*> tmp;

  /////////////////////////////////////////////////
  // Game Corner inventory (coins only): the coins-priced Game-Corner-exclusive
  // items plus the Pokemon prizes.
  if(!isMoneyCurrency) {
    itemListCache.append(new ItemMarketEntryMessage("Inventory"));

    for(auto el : ItemsDB::inst()->getStore()) {
      if(el->isGameCornerExclusive())
        tmp.append(el);
    }
    sortByName(tmp);
    appendItems(tmp);
    tmp.clear();

    for(auto el : GameCornerDB::inst()->getStore()) {
      if(el->getType() == "pokemon")
        itemListCache.append(new ItemMarketEntryGCPokemon(el, playerPokemon, storage));
    }
  }

  /////////////////////////////////////////////////
  // Normal (non-glitch) priced items, split by real Gen-1 buyability:
  //   * Normal Items    -- stocked by a reachable mart: you can BUY *and* SELL them.
  //   * Unbuyable Items -- priced (so SELLABLE) but sold by no mart, e.g. Nugget,
  //                        Rare Candy, Max Revive, the vending-machine drinks, and
  //                        every TM that isn't on the Celadon shelf.
  // vendorListItem() is the price gate (a positive buy price in the active currency,
  // and not a Game-Corner exclusive). An item with no price in this currency lands in
  // neither list -- there is no way to buy *or* sell it, so it is shown nowhere. This
  // matches pokered data/items/prices.asm, where price 0 means "cannot be priced/sold"
  // (Master Ball, Moon Stone, the PP restoratives, key items, ...).
  QVector<ItemDBEntry*> normalItems;
  QVector<ItemDBEntry*> unbuyableItems;

  for(auto el : ItemsDB::inst()->getStore()) {
    if(el->getOnce() || el->getGlitch() || !vendorListItem(el))
      continue;
    (buyableInMart(el) ? normalItems : unbuyableItems).append(el);
  }

  itemListCache.append(new ItemMarketEntryMessage("Normal Items"));
  sortByName(normalItems);
  appendItems(normalItems);

  itemListCache.append(new ItemMarketEntryMessage("Unbuyable Items"));
  sortByName(unbuyableItems);
  appendItems(unbuyableItems);

  ////////////////////////////////////////////////////
  // Glitch items (unchanged): any glitch item that still carries a price.
  itemListCache.append(new ItemMarketEntryMessage("Glitch Items"));

  for(auto el : ItemsDB::inst()->getStore()) {
    if(el->getGlitch() && vendorListItem(el))
      tmp.append(el);
  }
  sortByName(tmp);
  appendItems(tmp);
  tmp.clear();

  for(int i = from; i < itemListCache.size(); i++)
    itemListCache[i]->viewTag = ViewBuy;
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

  buildList();
}
