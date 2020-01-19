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

#include <QString>
#include <QRandomGenerator>

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
  if(saveFile == nullptr) {
    reset();
    return;
  }

  reset();

  auto it = saveFile->iterator();
  it->offsetTo(0x25C9);

  var8 amount = it->getByte();

  for(var8 i = 0; i < amount; i++)
    bagItems.append(new Item(it));

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

void PlayerItems::reset()
{
  for(auto item : bagItems)
  {
    delete item;
  }

  bagItems.clear();
}

void PlayerItems::randomize()
{
  reset();

  auto rnd = QRandomGenerator::global();

  // Essentials
  bagItems.append(new Item("TOWN MAP", 1));
  bagItems.append(new Item("POKE BALL", rnd->bounded(5, 15+1)));
  bagItems.append(new Item("POTION", rnd->bounded(5, 10+1)));
  bagItems.append(new Item("ANTIDOTE", rnd->bounded(1, 3+1)));
  bagItems.append(new Item("PARLYZ HEAL", rnd->bounded(1, 3+1)));
  bagItems.append(new Item("AWAKENING", rnd->bounded(1, 3+1)));

  // Again I have no idea where this will drop you so prepare for an escape
  // If need be, also because you only get 1 HM Slave that doesn't know dig.
  // This is your dig.
  bagItems.append(new Item("ESCAPE ROPE", rnd->bounded(1, 5+1)));

  bool giveSuperPotion = rnd->bounded(0, 5+1) >= 3;
  if(giveSuperPotion)
    bagItems.append(new Item("SUPER POTION", 1));

  bool giveGreatBall = rnd->bounded(0, 5+1) >= 3;
  if(giveGreatBall)
    bagItems.append(new Item("GREAT BALL", 1));
}
