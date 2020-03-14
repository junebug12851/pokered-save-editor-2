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

#include <QDebug>

#include "./itemmarketentrymoney.h"
#include <pse-db/gamecorner.h>
#include "../../data/file/expanded/player/playerbasics.h"

ItemMarketEntryMoney::ItemMarketEntryMoney()
  : ItemMarketEntry(CompatNo, CompatEither) // Only Coins, either buy/sell
{
  finishConstruction();
  exclude = true;
}

ItemMarketEntryMoney::~ItemMarketEntryMoney() {}

QString ItemMarketEntryMoney::_name()
{
  if(!requestFilter())
    return "";

  if(*isBuyMode)
    return "Money => Coins";

  return   "Coins => Money";
}

// An exception, Money is strictly a money exchange as in your always selling
// something and therefore always have an in-stock value which has a limit.
int ItemMarketEntryMoney::_inStockCount()
{
  if(!requestFilter())
    return 0;

  if(*isBuyMode)
    return player->money;

  return player->coins;
}

bool ItemMarketEntryMoney::_canSell()
{
  return true;
}

int ItemMarketEntryMoney::_itemWorth()
{
  if(!requestFilter())
    return 0;

  if(*isBuyMode)
    return GameCornerDB::buyPrice;

  return GameCornerDB::sellPrice;
}

QString ItemMarketEntryMoney::_whichType()
{
  return type;
}

int ItemMarketEntryMoney::onCartLeft()
{
  if(!requestFilter())
    return 0;

  int ret = 0;

  // Selling money to buy coins
  if(*isBuyMode) {

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

void ItemMarketEntryMoney::checkout()
{
  if(!canCheckout() ||
     !requestFilter())
    return;

  if(*isBuyMode) {
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
  onCartChanged();
}
