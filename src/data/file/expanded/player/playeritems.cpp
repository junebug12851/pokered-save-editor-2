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
#include "./playeritems.h"
#include "../fragments/item.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/items.h"
#include "../../../../common/random.h"

#include <QString>

PlayerItems::PlayerItems(SaveFile* saveFile)
{
  load(saveFile);
}

PlayerItems::~PlayerItems()
{
  reset();
}

void PlayerItems::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr) {
    return;
  }

  auto it = saveFile->iterator();
  it->offsetTo(0x25C9);

  var8 amount = it->getByte();

  for(var8 i = 0; i < amount; i++) {
    bagItems.append(new Item(it));
    bagItemInsertChange();
  }

  bagItemsChanged();

  delete it;
}

void PlayerItems::save(SaveFile* saveFile)
{
  auto it = saveFile->iterator();
  it->offsetTo(0x25C9);

  it->setByte(bagItems.size());

  for(var8 i = 0; i < bagItems.size(); i++) {
    bagItems.at(i)->save(it);
  }

  it->setByte(0xFF);

  delete it;
}

int PlayerItems::bagItemCount()
{
  return bagItems.size();
}

int PlayerItems::bagItemMax()
{
  return maxBagItems;
}

Item* PlayerItems::bagItemAt(int ind)
{
  return bagItems.at(ind);
}

void PlayerItems::bagItemMove(int from, int to)
{
  if(from == to)
    return;

  // Grab and remove item
  auto eFrom = bagItems.at(from);
  bagItems.removeAt(from);

  // Insert it elsewhere
  bagItems.insert(to, eFrom);

  bagItemMoveChange(from, to);
  bagItemsChanged();
}

void PlayerItems::bagItemRemove(int ind)
{
  if(bagItems.size() <= 0)
    return;

  delete bagItems.at(ind);
  bagItems.removeAt(ind);

  bagItemRemoveChange(ind);
  bagItemsChanged();
}

void PlayerItems::bagItemNew()
{
  if(bagItems.size() >= maxBagItems)
    return;

  bagItems.append(new Item);

  bagItemInsertChange();
  bagItemsChanged();
}

void PlayerItems::reset()
{
  for(auto item : bagItems)
  {
    delete item;
  }

  bagItems.clear();

  bagItemResetChange();
  bagItemsChanged();
}

void PlayerItems::randomize()
{
  reset();

  // Essentials
  bagItems.append(new Item("TOWN MAP", 1));
  bagItemInsertChange();

  bagItems.append(new Item("POKE BALL", Random::rangeInclusive(5, 15)));
  bagItemInsertChange();

  bagItems.append(new Item("POTION", Random::rangeInclusive(5, 10)));
  bagItemInsertChange();

  bagItems.append(new Item("ANTIDOTE", Random::rangeInclusive(1, 3)));
  bagItemInsertChange();

  bagItems.append(new Item("PARLYZ HEAL", Random::rangeInclusive(1, 3)));
  bagItemInsertChange();

  bagItems.append(new Item("AWAKENING", Random::rangeInclusive(1, 3)));
  bagItemInsertChange();

  // Again I have no idea where this will drop you so prepare for an escape
  // If need be, also because you only get 1 HM Slave that doesn't know dig.
  // This is your dig.
  bagItems.append(new Item("ESCAPE ROPE", Random::rangeInclusive(1, 5)));
  bagItemInsertChange();

  // 25% chance of having these items
  bool giveSuperPotion = Random::chanceSuccess(25);
  if(giveSuperPotion) {
    bagItems.append(new Item("SUPER POTION", 1));
    bagItemInsertChange();
  }

  bool giveGreatBall = Random::chanceSuccess(25);
  if(giveGreatBall) {
    bagItems.append(new Item("GREAT BALL", 1));
    bagItemInsertChange();
  }

  bagItemsChanged();
}
