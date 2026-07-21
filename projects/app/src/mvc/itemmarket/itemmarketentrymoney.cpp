/*
  * Copyright 2020 Fairy Fox
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
 *
 * Rate semantics (the fix): the traded unit is ONE COIN, and `onCart` is the number
 * of coins. A coin costs GameCornerDB::getBuyPrice() money to BUY (~20 = ¥20/coin,
 * straight from the game data) and returns getSellPrice() money when SOLD (the
 * project's half-back policy). So buying N coins costs 20·N money; selling N coins
 * returns 10·N money. (The earlier code multiplied coins by the rate, which inverted
 * it -- 20 coins per ¥1.)
 */

#include <QDebug>

#include "./itemmarketentrymoney.h"
#include <pse-db/gamecornerdb.h>
#include <pse-savefile/expanded/player/playerbasics.h>

// Compatibility is Either/Either: these rows are only ever built into the dedicated
// Exchange list now, so they always pass the filter; the swap direction comes from
// forceDir, not the global buy/sell flag.
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

// Always "selling" something, so it always has an in-stock value: how many of the
// SOURCE currency you hold (money when buying coins, coins when selling them).
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

// The per-coin rate: cost to buy a coin, or money returned for selling one.
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

int ItemMarketEntryMoney::moneyDelta() const
{
  const int rate = buying() ? GameCornerDB::inst()->getBuyPrice()
                            : GameCornerDB::inst()->getSellPrice();
  // Buying coins spends money; selling coins gains money.
  return (buying() ? -rate : rate) * onCart;
}

int ItemMarketEntryMoney::coinsDelta() const
{
  // Buying gains coins; selling spends them.
  return (buying() ? 1 : -1) * onCart;
}

// How many MORE coins can be added to the cart, bounded by both currencies' caps and
// by what the player can afford / actually owns.
int ItemMarketEntryMoney::onCartLeft()
{
  const int rate = buying() ? GameCornerDB::inst()->getBuyPrice()
                            : GameCornerDB::inst()->getSellPrice();
  if(rate <= 0)
    return 0;

  int maxCoins = 0;
  if(buying()) {
    const int coinCapRoom = 9999 - player->coins;       // coins fit under 9,999
    const int affordable  = player->money / rate;       // coins you can pay for
    maxCoins = qMin(coinCapRoom, affordable);
  } else {
    const int coinsOwned   = player->coins;             // coins you actually have
    const int moneyCapRoom = (999999 - player->money) / rate; // money fits under 999,999
    maxCoins = qMin(coinsOwned, moneyCapRoom);
  }

  return maxCoins - onCart;
}

int ItemMarketEntryMoney::stackCount()
{
  // Applies only to buying items
  return 0;
}

// Exchange-aware affordability: gate on the currency actually being spent.
bool ItemMarketEntryMoney::canCheckout()
{
  if(onCart <= 0)
    return false;
  if(onCartLeft() < 0)
    return false;

  const int rate = GameCornerDB::inst()->getBuyPrice();
  return buying() ? (player->money >= rate * onCart)   // can pay for the coins
                  : (player->coins >= onCart);          // owns the coins to sell
}

void ItemMarketEntryMoney::checkout()
{
  if(!canCheckout())
    return;

  player->money += moneyDelta();
  player->coins += coinsDelta();

  player->moneyChanged();
  player->coinsChanged();

  onCart = 0;
}
