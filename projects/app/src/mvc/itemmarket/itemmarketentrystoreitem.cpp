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

#include "./itemmarketentrystoreitem.h"
#include <pse-db/items.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/player/playerbasics.h>

ItemMarketEntryStoreItem::ItemMarketEntryStoreItem(
    ItemDBEntry* data,
    ItemStorageBox* toBag,
    ItemStorageBox* toBox)
  : ItemMarketEntry(CompatEither, CompatYes), // Any currency, only buying
    data(data),
    toBag(toBag),
    toBox(toBox)
{
  finishConstruction();
}

ItemMarketEntryStoreItem::~ItemMarketEntryStoreItem() {}

StackReturn ItemMarketEntryStoreItem::calculateStacks()
{
  if(!requestFilter())
    return StackReturn();

  // Temp copy of on cart, we don't want to change the actual onCart
  int _onCart = onCart;

  // Start off with no counted stacks. This represents the number of full new
  // stacks required by the cart items.
  int stacks = 0;

  // and no counted partial stacks. This represents the space left on the stack
  // as required by the cart items.
  // If there are 25 stack items left, all 25, or maybe 20, 15, or 5 may be
  // required. This is the required amount that can be given to a partial
  // stack before moving to new full stacks if need be.
  int stackPartialBag = 0;
  int stackPartialBox = 0;

  // See if there's already an existing stack
  Item* currentStackBag = nullptr;
  Item* currentStackBox = nullptr;

  // Basically look for first occurence in both the bag and box that have a
  // stack of less than 99 (Indeed a true partial stack)
  for(auto el : toBag->items) {
    if(el->ind == data->ind && el->amount < 99) {
      currentStackBag = el;
      break;
    }
  }

  for(auto el : toBox->items) {
    if(el->ind == data->ind && el->amount < 99) {
      currentStackBox = el;
      break;
    }
  }

  // Add what we can to the current stack if there is one
  // Start with bag. This proceeds only if there is actually a partial stack
  if(currentStackBag != nullptr) {
    // We get the space leftover in the partial stack
    int totalStackLeft = 99 - currentStackBag->amount;

    // If the cart amount is bigger than the partial stack free space, just
    // assign the rest of the free space.
    // (A partial stack of 99 or a full partial stack)
    if(_onCart > totalStackLeft) {
      stackPartialBag = totalStackLeft;
      _onCart -= totalStackLeft;
    }

    // If the entire cart can be placed onto the partial stack then things are
    // simple, place the whole cart and empty the cart out.
    else {
      stackPartialBag = _onCart;
      _onCart = 0;
    }
  }

  // Partial stacks are always the highest priority and the bag comes first.
  // Here, we do the same thing but for the box next, filling up the partial
  // stack there, if any.
  if(currentStackBox != nullptr && _onCart > 0) {
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

  // Finish the rest off in seperate stacks. We must now calculate how many
  // more stacks we require. We do this only if there are items left in the
  // cart.
  while(_onCart > 0) {
    // We immidiately increase the full stack count
    stacks++;

    // If there is more than 99 items on the cart still, we subtract 99 and
    // continue on. Otherwise we count the cart as empty and in need of no more
    // stacks.
    if(_onCart > 99) {
      _onCart -= 99;
    }
    else {
      _onCart = 0;
    }
  }

  // We return the results which is largely helpful to a number of methods
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
    return data->buyPriceMoney();

  // Buy item for coins
  else if(!(*isMoneyCurrency) && *isBuyMode)
    return data->buyPriceCoins();

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

  // Final value to return, a representation of items left that can go onto the
  // cart
  int ret = 0;

  // Calculate stacks others take up
  int totalStackFromOthers = totalStackCount() - stk.full;

  // Calculate inventory space used and free combined from both bag and box
  int combinedBoxSpace = toBox->itemsMax() + toBag->itemsMax();
  int combinedBoxUsed = toBox->itemsCount() + toBag->itemsCount();

  // Stack space left before requested transaction
  int stackSpaceBefore = combinedBoxSpace - combinedBoxUsed;

  // Stack space after requested transactions
  int stackSpaceLeftAfter = stackSpaceBefore - totalStackFromOthers - stk.full;

  // Calculate whole stack items remaining. There are 99 items max per stack
  // This is the whole new stacks converted to item count.
  ret = stackSpaceLeftAfter * 99;

  // Now figure out partial stacks which we calculate only if there is a partial
  // stack. The stack represents number of additional used space. We determine
  // the rest available.
  // 99 (Max) - 20 (On Cart) - 50 (Already there) = 29 items left in partial
  // or
  // 99 (Max) - 5 (On Cart) - 94 (Already there) = 0 Items Left in partial
  if(stk.partialElBag != nullptr) {
    ret += 99 - stk.partialBag - stk.partialElBag->amount;

    if((99 - stk.partialBag - stk.partialElBag->amount) < 0)
      qDebug() << "Negative Amount" << (99 - stk.partialBag - stk.partialElBag->amount);
  }

  if(stk.partialElBox != nullptr) {
    ret += 99 - stk.partialBox - stk.partialElBox->amount;

    if((99 - stk.partialBox - stk.partialElBox->amount) < 0)
      qDebug() << "Negative Amount" << (99 - stk.partialBox - stk.partialElBox->amount);
  }

  // Ret Now contains the maximum items possible left but it doesn't take
  // into consideration how much money is left. We do a quick calculation for
  // that
  int maxAmountFromMoney = moneyLeftover() / itemWorth();

  // Return the smallest of the calculations
  return qMin(ret, maxAmountFromMoney);
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
