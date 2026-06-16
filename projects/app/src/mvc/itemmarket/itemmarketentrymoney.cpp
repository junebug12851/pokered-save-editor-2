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
 * @file itemmarketentrymoney.cpp
 * @brief Implementation of ItemMarketEntryMoney. See itemmarketentrymoney.h.
 */

#include <QDebug>

#include "./itemmarketentrymoney.h"
#include <pse-db/gamecornerdb.h>
#include <pse-savefile/expanded/player/playerbasics.h>

// Compatibility is Either/Either: these rows are only ever built into the dedicated
// Exchange list now (never the buy/sell item lists), so they always pass the filter;
// the swap direction comes from forceDir, not the global buy/sell flag.
ItemMarketEntryMoney::ItemMarketEntryMoney(int forceDir)
  : ItemMarketEntry(CompatEither, CompatEither),
    forceDir(forceDir)
{
  finishConstruction();
  exclude = true;
}

ItemMarketEntryMoney::~ItemMarketEntryMoney() {}

bool ItemMarketEntryMoney::buying() const
{
  if(forceDir != DirGlobal)
    return forceDir == DirToCoins;
  return *isBuyMode;
}

QString ItemMarketEntryMoney::_name()
{
  if(buying())
    return tr("Money => Coins");

  return tr("Coins => Money");
}

// An exception, Money is strictly a money exchange as in your always selling
// something and therefore always have an in-stock value which has a limit.
int ItemMarketEntryMoney::_inStockCount()
{
  if(buying())
    return player->money;

  return player->coins;
}

bool ItemMarketEntryMoney::_canSell()
{
  return true;
}

int ItemMarketEntryMoney::_itemWorth()
{
  if(buying())
    return GameCornerDB::inst()->getBuyPrice();

  return GameCornerDB::inst()->getSellPrice();
}

QString ItemMarketEntryMoney::_whichType()
{
  return type;
}

int ItemMarketEntryMoney::onCartLeft()
{
  int ret = 0;

  // Selling money to buy coins
  if(buying()) {

    //How much coins can we get
    int coinsLeft = (9999 - player->coins) / itemWorth();

    // How much coins can the player buy
    int coinsBuyable = player->money - onCart; /// itemWorth();

    // Return the smaller of the two
    ret = qMin(coinsLeft - onCart, coinsBuyable);
  }

  // Selling coins to buy money
  else {
    //How much money can we get
    int moneyLeft = (999999 - player->money)  / itemWorth();

    // How much money can the player buy
    int moneyBuyable = player->coins - onCart; /// itemWorth();

    // Return the smaller of the two
    ret = qMin(moneyLeft - onCart, moneyBuyable);
  }

  return ret;
}

int ItemMarketEntryMoney::stackCount()
{
  // Applies only to buying items
  return 0;
}

// Exchange-aware affordability. The base canCheckout() leans on the model-wide
// moneyLeftover(), which is single-currency and excludes money rows -- meaningless
// for a swap. Gate directly on the currency actually being spent instead.
bool ItemMarketEntryMoney::canCheckout()
{
  if(onCart <= 0)
    return false;
  if(onCartLeft() < 0)        // destination would overflow its cap
    return false;

  // buying() spends money (Money=>Coins); else spends coins (Coins=>Money).
  return buying() ? (player->money >= onCart)
                  : (player->coins >= onCart);
}

void ItemMarketEntryMoney::checkout()
{
  if(!canCheckout())
    return;

  if(buying()) {
    player->money -= onCart;
    player->coins += cartWorth();
  }
  else {
    player->money += cartWorth();
    player->coins -= onCart;
  }

  player->moneyChanged();
  player->coinsChanged();

  onCart = 0;
}
