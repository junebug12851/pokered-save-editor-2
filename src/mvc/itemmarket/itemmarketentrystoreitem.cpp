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

#include "./itemmarketentrystoreitem.h"
#include "../../data/db/items.h"
#include "../../data/file/expanded/fragments/item.h"
#include "../../data/file/expanded/fragments/itemstoragebox.h"
#include "../../data/file/expanded/player/playerbasics.h"

ItemMarketEntryStoreItem::ItemMarketEntryStoreItem(
    ItemDBEntry* data,
    ItemStorageBox* toBag,
    ItemStorageBox* toBox)
  : ItemMarketEntry(CompatEither, CompatYes), // Any currency, only buying
    data(data),
    toBag(toBag),
    toBox(toBox)
{

}

ItemMarketEntryStoreItem::~ItemMarketEntryStoreItem()
{

}

StackReturn ItemMarketEntryStoreItem::calculateStacks()
{
  if(!requestFilter())
    return StackReturn();

  // Temp copy of on cart
  int _onCart = onCart;

  int stacks = 0;

  int stackPartialBag = 0;
  int stackPartialBox = 0;

  // See if there's already an existing stack
  Item* currentStackBag = nullptr;
  Item* currentStackBox = nullptr;

  for(auto el : toBag->items) {
    if(el->ind == data->ind) {
      currentStackBag = el;
      break;
    }
  }

  for(auto el : toBox->items) {
    if(el->ind == data->ind) {
      currentStackBox = el;
      break;
    }
  }

  // Add what we can to the current stack if there is one
  // Start with bag
  if(currentStackBag != nullptr) {
    int totalStackLeft = 99 - currentStackBag->amount;
    if(_onCart > totalStackLeft) {
      stackPartialBag = totalStackLeft;
      _onCart -= totalStackLeft;
    }
    else {
      stackPartialBag = _onCart;
      _onCart = 0;
    }
  }

  // Add what we can to the current stack if there is one
  // End with box
  if(currentStackBox != nullptr) {
    int totalStackLeft = 99 - currentStackBox->amount;
    if(_onCart > totalStackLeft) {
      stackPartialBox = totalStackLeft;
      _onCart -= totalStackLeft;
    }
    else {
      stackPartialBox = _onCart;
      _onCart = 0;
    }
  }

  // Finish the rest off in seperate stacks
  while(_onCart > 0) {
    stacks++;
    if(_onCart > 99) {
      _onCart -= 99;
    }
    else {
      _onCart = 0;
    }
  }

  StackReturn stk;
  stk.full = stacks;
  stk.partialBag = stackPartialBag;
  stk.partialElBag = currentStackBag;
  stk.partialBox = stackPartialBox;
  stk.partialElBox = currentStackBox;

  return stk;
}

QString ItemMarketEntryStoreItem::_name()
{
  if(!requestFilter())
    return "";

  return data->readable;
}

int ItemMarketEntryStoreItem::_inStockCount()
{
  // There's no merchant inventory limit
  return 0;
}

bool ItemMarketEntryStoreItem::_canSell()
{
  // There's nothing a merchant can't sell
  return true;
}

int ItemMarketEntryStoreItem::_itemWorth()
{
  if(!requestFilter())
    return 0;

  // Buy item for money
  if(*isMoneyCurrency && *isBuyMode)
    return data->sellPriceMoney();

  // Buy item for coins
  else if(!(*isMoneyCurrency) && *isBuyMode)
    return data->sellPriceCoins();

  return 0;
}

QString ItemMarketEntryStoreItem::_whichType()
{
  return type;
}

// This is a bit tricky because we have to see how many the player can buy if
// they already have the item and, regardless, how many stacks they can
// accumulate
int ItemMarketEntryStoreItem::onCartLeft()
{
  if(!requestFilter())
    return 0;

  // Calculate stacks this item takes up
  auto stk = calculateStacks();

  // Calculate stacks others take up
  int totalStackFromOthers = totalStackCount() - stk.full;

  // Calculate partial stacks left
  int partialStackBagLeft = stk.partialBag;
  if(stk.partialElBag != nullptr)
    partialStackBagLeft += stk.partialElBag->amount;

  partialStackBagLeft = 99 - partialStackBagLeft;

  int partialStackBoxLeft = stk.partialBox;
  if(stk.partialElBag != nullptr)
    partialStackBoxLeft += stk.partialElBox->amount;

  partialStackBoxLeft = 99 - partialStackBoxLeft;

  // Calculate stack spaces used and free
  int combinedBoxSpace = toBox->itemsMax() + toBag->itemsMax();
  int combinedBoxUsed = toBox->itemsCount() + toBag->itemsCount();

  // Stack space left
  int stackSpaceLeft = combinedBoxSpace - combinedBoxUsed - totalStackFromOthers;

  // Calculate items remaining and return that
  return ((stackSpaceLeft - stk.full) * 99) + (99 - partialStackBagLeft) + (99 - partialStackBoxLeft);
}

int ItemMarketEntryStoreItem::stackCount()
{
  if(!requestFilter())
    return 0;

  return calculateStacks().full;
}

void ItemMarketEntryStoreItem::checkout()
{
  if(!requestFilter() ||
     !canCheckout())
    return;

  // Copy checkout money to take or give as we're going to be modifying onCart
  int origCartWorth = cartWorth();

  // Calculate stack information
  auto stk = calculateStacks();

  // Add what we can to the current stack if there is one
  if(stk.partialElBag != nullptr) {
    stk.partialElBag->amount += stk.partialBag;
    stk.partialElBag->amountChanged();
  }

  if(stk.partialElBox != nullptr) {
    stk.partialElBox->amount += stk.partialBox;
    stk.partialElBox->amountChanged();
  }

  onCart -= stk.partialBag;
  onCart -= stk.partialBox;

  // Stack item's we've completed used as an offset for box if needed
  int stkFullUsed = 0;

  // Create stacks for the rest. Start with the bag
  // Stop when we've processed all the stack items or when the cart is empty
  // or when the bag fills up
  for(int i = 0; i < stk.full && onCart > 0 && (toBag->itemsCount() < toBag->itemsMax()); i++) {
    if(onCart > 99) {
      toBag->items.append(new Item(data->ind, 99));
      toBag->itemInsertChange();
      toBag->itemsChanged();
      onCart -= 99;
    }
    else {
      toBag->items.append(new Item(data->ind, onCart));
      toBag->itemInsertChange();
      toBag->itemsChanged();
      onCart = 0;
    }

    stkFullUsed++;
  }

  stk.full -= stkFullUsed;

  // Create stacks for the rest. Continue with box
  // Stop when we've processed all the stack items or when the cart is empty
  // or when the box fills up
  for(int i = 0; i < stk.full && onCart > 0 && (toBox->itemsCount() < toBox->itemsMax()); i++) {
    if(onCart > 99) {
      toBox->items.append(new Item(data->ind, 99));
      toBox->itemInsertChange();
      toBox->itemsChanged();
      onCart -= 99;
    }
    else {
      toBox->items.append(new Item(data->ind, onCart));
      toBox->itemInsertChange();
      toBox->itemsChanged();
      onCart = 0;
    }
  }

  // Remove players money
  if(*isMoneyCurrency) {
    player->money -= origCartWorth;
    player->moneyChanged();
  }

  // Remove players coins
  else {
    player->coins -= origCartWorth;
    player->coinsChanged();
  }

  onCartChanged();
}
