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

#include "../../../db/items.h"
#include "../../../../common/random.h"

ItemStorageBox::ItemStorageBox(bool isBag, int maxSize, SaveFile* saveFile, int offset)
  : maxSize(maxSize),
    isBag(isBag)
{
  load(saveFile, offset);
}

ItemStorageBox::~ItemStorageBox()
{
  reset();
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

int ItemStorageBox::itemsWorth()
{
  int ret = 0;

  for(auto el : items) {
    ret += el->worthAll();
  }

  return ret;
}

bool ItemStorageBox::getIsBag()
{
  return isBag;
}

bool ItemStorageBox::relocateFull()
{
  auto dest = (isBag)
      ? file->dataExpanded->storage->items
      : file->dataExpanded->player->items;

  if(dest->items.size() >= dest->itemsMax())
    return true;

  return false;
}

Item* ItemStorageBox::itemAt(int ind)
{
  return items.at(ind);
}

void ItemStorageBox::itemMove(int from, int to)
{
  if(from == to)
    return;

  // Grab and remove item
  auto eFrom = items.at(from);
  items.removeAt(from);

  // Insert it elsewhere
  items.insert(to, eFrom);

  itemMoveChange(from, to);
  itemsChanged();
}

void ItemStorageBox::itemRemove(int ind)
{
  if(items.size() <= 0)
    return;

  delete items.at(ind);
  items.removeAt(ind);
  itemRemoveChange(ind);
  itemsChanged();
}

void ItemStorageBox::itemNew()
{
  if(items.size() >= maxSize)
    return;

  items.append(new Item);
  itemInsertChange();
  itemsChanged();
}

void ItemStorageBox::relocateAll()
{
  auto dest = (isBag)
      ? file->dataExpanded->storage->items
      : file->dataExpanded->player->items;

  while(items.size() > 0 && dest->items.size() < dest->itemsMax())
    relocateOne(0);
}

void ItemStorageBox::relocateOne(int ind)
{
  auto dest = (isBag)
      ? file->dataExpanded->storage->items
      : file->dataExpanded->player->items;

  if(dest->items.size() >= dest->itemsMax())
    return;

  auto el = items.at(ind);

  items.removeAt(ind);
  itemRemoveChange(ind);
  itemsChanged();

  dest->items.append(el);
  dest->itemInsertChange();
  dest->itemsChanged();
}

void ItemStorageBox::sort()
{
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

void ItemStorageBox::reset()
{
  for(auto item : items) {
    disconnect(item, &Item::itemChanged, this, &ItemStorageBox::itemsChanged);
    delete item;
  }

  items.clear();
  itemsResetChange();
  itemsChanged();
}

void ItemStorageBox::randomize()
{
  reset();

  // Between None and half of max capacity
  var8 count = Random::rangeInclusive(0, maxSize * .5);

  // Load up random items to count
  for(var8 i = 0; i < count; i++) {
    items.append(new Item(true));
    itemInsertChange();
  }

  itemsChanged();
}
