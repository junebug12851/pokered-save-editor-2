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

#include "./itemmarketentryplayeritem.h"
#include "../../data/db/items.h"
#include "../../data/file/expanded/fragments/item.h"
#include "../../data/file/expanded/fragments/itemstoragebox.h"
#include "../../data/file/expanded/player/playerbasics.h"

ItemMarketEntryPlayerItem::ItemMarketEntryPlayerItem(ItemStorageBox* toBox,
                                                     Item* toItem)
  : ItemMarketEntry(CompatEither, CompatNo), // Any currency, only selling
    toBox(toBox),
    toItem(toItem)
{

}

ItemMarketEntryPlayerItem::~ItemMarketEntryPlayerItem()
{

}

QString ItemMarketEntryPlayerItem::_name()
{
  if(!requestFilter())
    return "";

  auto itemData = toItem->toItem();
  if(itemData == nullptr)
    return "";

  return itemData->readable;
}

int ItemMarketEntryPlayerItem::_inStockCount()
{
  if(!requestFilter())
    return 0;

  return toItem->amount;
}

bool ItemMarketEntryPlayerItem::_canSell()
{
  if(!requestFilter())
    return false;

  auto itemData = toItem->toItem();
  if(itemData == nullptr)
    return false;

  return itemData->canSell();
}

int ItemMarketEntryPlayerItem::_itemWorth()
{
  if(!requestFilter())
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

int ItemMarketEntryPlayerItem::onCartMax()
{
  if(!requestFilter())
    return 0;

  return toItem->amount;
}

int ItemMarketEntryPlayerItem::stackCount()
{
  // Applies only to buying items
  return 0;
}

void ItemMarketEntryPlayerItem::checkout()
{
  if(!requestFilter() ||
     !canCheckout() ||
     onCart == 0)
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

  onCart = 0;
  onCartChanged();
}