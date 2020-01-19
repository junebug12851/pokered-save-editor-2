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

#include "./itemstoragebox.h"
#include "./item.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/items.h"
#include "../../../../random.h"

ItemStorageBox::ItemStorageBox(SaveFile* saveFile)
{
  load(saveFile);
}

ItemStorageBox::~ItemStorageBox()
{
  reset();
}

void ItemStorageBox::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  auto it = saveFile->iterator()->offsetTo(0x27E7);

  for (var8 i = 0; i < toolset->getByte(0x27E6) && i < boxMaxItems; i++) {
    items.append(new Item(it));
  }

  delete it;
}

void ItemStorageBox::save(SaveFile* saveFile)
{
  // Save all box items
  auto it = saveFile->iterator()->offsetTo(0x27E6);
  it->setByte(items.size());
  for (var8 i = 0; i < items.size() && i < boxMaxItems; i++) {
    it->setByte(items.at(i)->ind);
    it->setByte(items.at(i)->amount);
  }
  it->setByte(0xFF);
  delete it;
}

void ItemStorageBox::reset()
{
  for(auto item : items)
    delete item;

  items.clear();
}

void ItemStorageBox::randomize()
{
  reset();

  // Between None and half of max capacity
  var8 count = Random::rangeInclusive(0, boxMaxItems * .5);

  // Load up random items to count
  for(var8 i = 0; i < count; i++)
    items.append(new Item(true));
}
