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
  : ItemMarketEntry(CompatNo, CompatEither) // Only Coins, either buy/sell
{
  finishConstruction();
}

ItemMarketEntryMoney::~ItemMarketEntryMoney() {}

QString ItemMarketEntryMoney::_name()
{
  if(!requestFilter())
    return "";

  return   "Coins";
}

int ItemMarketEntryMoney::_inStockCount()
{
  if(!requestFilter())
    return 0;

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

  if(*isBuyMode)
    return 9999 - onCart;

  return player->coins - onCart;
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
