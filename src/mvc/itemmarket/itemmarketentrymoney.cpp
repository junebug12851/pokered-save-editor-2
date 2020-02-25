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

#include "./itemmarketentrymoney.h"
#include "../../data/db/gamecorner.h"
#include "../../data/file/expanded/player/playerbasics.h"

ItemMarketEntryMoney::ItemMarketEntryMoney()
{
  finishConstruction();
}

ItemMarketEntryMoney::~ItemMarketEntryMoney() {}

bool ItemMarketEntryMoney::moneyToCoins()
{
  // Selling Coins for Money
  if(*isMoneyCurrency && *isBuyMode)
    return false;

  // Selling Money for Coins
  else if(*isMoneyCurrency && !(*isBuyMode))
    return true;

  // Selling Money for Coins
  else if(!(*isMoneyCurrency) && *isBuyMode)
    return true;

  // Selling Coins for Money
  else if(!(*isMoneyCurrency) && !(*isBuyMode))
    return false;

  return false;
}

QString ItemMarketEntryMoney::_name()
{
  if(*isMoneyCurrency)
    return "Coins";

  return "Money";
}

int ItemMarketEntryMoney::_inStockCount()
{
  if(*isMoneyCurrency)
    return player->coins;

  return player->money;
}

bool ItemMarketEntryMoney::_canSell()
{
  return true;
}

int ItemMarketEntryMoney::_itemWorth()
{
  if(moneyToCoins())
    return GameCornerDB::buyPrice;

  return GameCornerDB::sellPrice;
}

QString ItemMarketEntryMoney::_whichType()
{
  return type;
}

int ItemMarketEntryMoney::onCartLeft()
{
  if(*isMoneyCurrency)
    return player->coins - onCart;

  return player->money - onCart;
}

int ItemMarketEntryMoney::stackCount()
{
  // Applies only to buying items
  return 0;
}

void ItemMarketEntryMoney::checkout()
{
  if(!canCheckout())
    return;

  if(moneyToCoins()) {
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
