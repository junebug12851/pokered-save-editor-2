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
 * @file itemmarketentryplayeritem.cpp
 * @brief Implementation of ItemMarketEntryPlayerItem. See itemmarketentryplayeritem.h.
 */

#include "./itemmarketentryplayeritem.h"
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/player/playerbasics.h>

ItemMarketEntryPlayerItem::ItemMarketEntryPlayerItem(ItemStorageBox* toBox,
                                                     Item* toItem)
  : ItemMarketEntry(CompatEither, CompatEither), // Any currency; selling, but coexists
                                                 // with buys in the unified cart, so it
                                                 // no longer mode-gates on the buy flag.
    toBox(toBox),
    toItem(toItem)
{
  finishConstruction();
}

ItemMarketEntryPlayerItem::~ItemMarketEntryPlayerItem() {}

QString ItemMarketEntryPlayerItem::_name()
{
  if(!toItem || !requestFilter())   // toItem may have been freed (stale entry)
    return "";

  auto itemData = toItem->toItem();
  if(itemData == nullptr)
    return "";

  return itemData->getReadable();
}

int ItemMarketEntryPlayerItem::_inStockCount()
{
  if(!toItem || !requestFilter())
    return 0;

  return toItem->amount;
}

bool ItemMarketEntryPlayerItem::_canSell()
{
  if(!toItem || !requestFilter())
    return false;

  auto itemData = toItem->toItem();
  if(itemData == nullptr)
    return false;

  return itemData->canSell();
}

int ItemMarketEntryPlayerItem::_itemWorth()
{
  // A stale entry (left in the static instances registry by a torn-down model) can
  // outlive its Item; QPointer nulls on free, so bail before touching it. This is the
  // exact dangling read AddressSanitizer caught at itemmarketentryplayeritem.cpp:79.
  if(!toItem || !requestFilter())
    return 0;

  // Sell item to money
  if(*isMoneyCurrency)
    return toItem->sellPriceOneMoney();

  // Sell item to coins
  else if(!(*isMoneyCurrency))
    return toItem->sellPriceOneCoins();

  return 0;
}

QString ItemMarketEntryPlayerItem::_whichType()
{
  return type;
}

int ItemMarketEntryPlayerItem::onCartLeft()
{
  if(!toItem || !requestFilter())
    return 0;

  // Maximum items left that can be sold
  int maxItems = toItem->amount - onCart;

  // Maximum money left that can be obtained from a sell
  int maxMoney = (*isMoneyCurrency)
      ? 999999
      : 9999;

  maxMoney -= moneyLeftover();

  int maxMoneyItems = (itemWorth() == 0)
      ? INT_MAX
      : maxMoney / itemWorth();

  // Return smallest of the numbers
  return qMin(maxItems, maxMoneyItems);
}

int ItemMarketEntryPlayerItem::stackCount()
{
  // Applies only to buying items
  return 0;
}

void ItemMarketEntryPlayerItem::checkout()
{
  if(!toItem || !toBox ||
     !requestFilter() ||
     !canCheckout())
    return;

  // Subtract amount owed
  toItem->amount -= onCart;
  toItem->amountChanged();

  // Remove if empty
  if(toItem->amount <= 0)
    toBox->itemRemove(toBox->items.indexOf(toItem));

  // Sell item to money
  if(*isMoneyCurrency) {
    player->money += cartWorth();
    player->moneyChanged();
  }

  // Sell item to coins
  else {
    player->coins += cartWorth();
    player->coinsChanged();
  }
}
