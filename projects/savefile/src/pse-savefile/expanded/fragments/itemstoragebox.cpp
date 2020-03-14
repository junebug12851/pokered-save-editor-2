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

#include <algorithm>
#include <QCollator>

#include "./itemstoragebox.h"
#include "./item.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

#include "../savefileexpanded.h"
#include "../player/player.h"
#include "../storage.h"

#include <pse-db/items.h>
#include <pse-common/random.h>

ItemStorageBox::ItemStorageBox(bool isBag, int maxSize, SaveFile* saveFile, int offset)
  : maxSize(maxSize),
    isBag(isBag)
{
  load(saveFile, offset);
}

ItemStorageBox::~ItemStorageBox()
{
  for(auto item : items)
    item->deleteLater();
}

int ItemStorageBox::itemsCount()
{
  return items.size();
}

int ItemStorageBox::itemsCountBulk()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->amount;
  }

  return ret;
}

int ItemStorageBox::itemsMax()
{
  return maxSize;
}

bool ItemStorageBox::getIsBag()
{
  return isBag;
}

bool ItemStorageBox::relocateFull()
{
  auto dest = destBox();

  if(dest->items.size() >= dest->itemsMax())
    return true;

  return false;
}

Item* ItemStorageBox::itemAt(int ind)
{
  if(ind >= items.size())
    return nullptr;

  return items.at(ind);
}

void ItemStorageBox::randomizeStorage()
{
  // Between None and 3/4 of max capacity
  var8 count = Random::rangeInclusive(0, maxSize * .75);

  // Create that many random items
  for(var8 i = 0; i < count; i++) {
    itemNew();
  }
}

void ItemStorageBox::randomizeBag()
{
  // Essentials
  items.append(new Item("TOWN MAP", 1));
  itemInsertChange();

  items.append(new Item("POKE BALL", Random::rangeInclusive(5, 15)));
  itemInsertChange();

  items.append(new Item("POTION", Random::rangeInclusive(5, 10)));
  itemInsertChange();

  items.append(new Item("ANTIDOTE", Random::rangeInclusive(1, 3)));
  itemInsertChange();

  items.append(new Item("PARLYZ HEAL", Random::rangeInclusive(1, 3)));
  itemInsertChange();

  items.append(new Item("AWAKENING", Random::rangeInclusive(1, 3)));
  itemInsertChange();

  // Again I have no idea where this will drop you so prepare for an escape
  // If need be, also because you only get 1 HM Slave that doesn't know dig.
  // This is your dig.
  items.append(new Item("ESCAPE ROPE", Random::rangeInclusive(1, 5)));
  itemInsertChange();

  // 25% chance of having these items
  bool giveSuperPotion = Random::chanceSuccess(25);
  if(giveSuperPotion) {
    items.append(new Item("SUPER POTION", 1));
    itemInsertChange();
  }

  bool giveGreatBall = Random::chanceSuccess(25);
  if(giveGreatBall) {
    items.append(new Item("GREAT BALL", 1));
    itemInsertChange();
  }

  // Up to 5 more completely random items
  var8 count = Random::rangeInclusive(0, 5);

  // Create that many random items
  for(var8 i = 0; i < count; i++) {
    itemNew();
  }
}

bool ItemStorageBox::itemMove(int from, int to)
{
  if(items.size() <= 0 ||
     from == to ||
     from >= items.size() ||
     from < 0 ||
     to >= items.size() ||
     to < 0)
    return false;

  // Grab and remove item
  auto eFrom = items.at(from);
  items.removeAt(from);

  // Insert it elsewhere
  items.insert(to, eFrom);

  itemMoveChange(from, to);
  itemsChanged();

  return true;
}

void ItemStorageBox::itemRemove(int ind)
{
  if(items.size() <= 0 ||
     ind < 0 ||
     ind >= items.size())
    return;

  items.at(ind)->deleteLater();
  items.removeAt(ind);
  itemRemoveChange(ind);
  itemsChanged();
}

void ItemStorageBox::itemNew()
{
  if(items.size() >= maxSize)
    return;

  items.append(new Item(true));
  itemInsertChange();
  itemsChanged();
}

bool ItemStorageBox::relocateAll()
{
  auto dest = destBox();

  bool ret = true;

  while(items.size() > 0 && dest->items.size() < dest->itemsMax()) {
    if(!relocateOne(0))
      ret = false;
  }

  return ret;
}

bool ItemStorageBox::relocateOne(int ind)
{
  auto dest = destBox();

  if(items.size() <= 0 ||
     ind < 0 ||
     ind >= items.size() ||
     dest->items.size() >= dest->itemsMax())
    return false;

  auto el = items.at(ind);
  beforeItemRelocate(el);

  items.removeAt(ind);
  itemRemoveChange(ind);
  itemsChanged();

  dest->items.append(el);
  dest->itemInsertChange();
  dest->itemsChanged();

  return true;
}

void ItemStorageBox::sort()
{
  if(items.size() <= 0)
    return;

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  std::sort(
        items.begin(),
        items.end(),
        [&collator](Item* item1, Item* item2)
  {
    if(item1->toItem() == nullptr || item2->toItem() == nullptr)
      return collator.compare("", "") < 0;

    return collator.compare(item1->toItem()->readable, item2->toItem()->readable) < 0;
  });

  itemsResetChange();
  itemsChanged();
}

void ItemStorageBox::load(SaveFile* saveFile, int offset)
{
  reset();

  this->file = saveFile;

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  auto it = saveFile->iterator()->offsetTo(offset+1);

  for (var8 i = 0; i < toolset->getByte(offset) && i < maxSize; i++) {
    auto item = new Item(it);
    items.append(item);

    connect(item, &Item::itemChanged, this, &ItemStorageBox::itemsChanged);
    itemInsertChange();
  }

  itemsChanged();

  delete it;
}

void ItemStorageBox::save(SaveFile* saveFile, int offset)
{
  // Save all box items
  auto it = saveFile->iterator()->offsetTo(offset);
  it->setByte(items.size());
  for (var8 i = 0; i < items.size() && i < maxSize; i++) {
    it->setByte(items.at(i)->ind);
    it->setByte(items.at(i)->amount);
  }
  it->setByte(0xFF);
  delete it;
}

ItemStorageBox* ItemStorageBox::destBox()
{
  return (isBag)
      ? file->dataExpanded->storage->items
      : file->dataExpanded->player->items;
}

int ItemStorageBox::itemsAllBuyMoney()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->buyPriceAllMoney();
  }

  return ret;
}

int ItemStorageBox::itemsAllBuyCoins()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->buyPriceAllCoins();
  }

  return ret;
}

int ItemStorageBox::itemsAllSellMoney()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->sellPriceAllMoney();
  }

  return ret;
}

int ItemStorageBox::itemsAllSellCoins()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->sellPriceAllCoins();
  }

  return ret;
}

void ItemStorageBox::reset()
{
  for(auto item : items) {
    item->deleteLater();
  }

  items.clear();
  itemsResetChange();
  itemsChanged();
}

void ItemStorageBox::randomize()
{
  reset();

  if(isBag)
    randomizeBag();
  else
    randomizeStorage();

  itemsChanged();
}
