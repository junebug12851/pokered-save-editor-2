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
#include "playeritems.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/items.h"

#include <QString>
#include <QRandomGenerator>

BagItem::BagItem(var8 id, var8 amount)
{
  this->id = id;
  this->amount = amount;
}

ItemEntry* BagItem::toItem()
{
  return Items::ind->value(QString::number(id), nullptr);
}

PlayerItems::PlayerItems(SaveFile* saveFile)
{
  bagItems = new QVector<BagItem*>();

  load(saveFile);
}

PlayerItems::~PlayerItems()
{
  for(auto item : *bagItems)
  {
    delete item;
  }

  delete bagItems;
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
    bagItems->append(new BagItem(it->getByte(), it->getByte()));
}

void PlayerItems::save(SaveFile* saveFile)
{
  auto it = saveFile->iterator();
  it->offsetTo(0x25C9);

  it->setByte(bagItems->size());

  for(var8 i = 0; i < bagItems->size(); i++) {
    it->setByte(bagItems->at(i)->id);
    it->setByte(bagItems->at(i)->amount);
  }

  it->setByte(0xFF);
}

void PlayerItems::reset()
{
  for(auto item : *bagItems)
  {
    delete item;
  }

  bagItems->clear();
}

void PlayerItems::randomize()
{
  reset();

  auto rnd = QRandomGenerator::global();

  bagItems->append(new BagItem(
                     Items::ind->value("TOWN MAP")->ind, 1));

  bagItems->append(new BagItem(
                     Items::ind->value("POKE BALL")->ind,
                     rnd->bounded(5, 15+1)));

  bagItems->append(new BagItem(
                     Items::ind->value("POTION")->ind,
                     rnd->bounded(5, 10+1)));

  bagItems->append(new BagItem(
                     Items::ind->value("ANTIDOTE")->ind,
                     rnd->bounded(1, 3+1)));

  bagItems->append(new BagItem(
                     Items::ind->value("PARLYZ HEAL")->ind,
                     rnd->bounded(1, 3+1)));

  bagItems->append(new BagItem(
                     Items::ind->value("AWAKENING")->ind,
                     rnd->bounded(1, 3+1)));

  bool giveSuperPotion = rnd->bounded(0, 5+1) >= 3;
  if(giveSuperPotion)
    bagItems->append(new BagItem(
                       Items::ind->value("SUPER POTION")->ind, 1));

  bool giveGreatBall = rnd->bounded(0, 5+1) >= 3;
  if(giveGreatBall)
    bagItems->append(new BagItem(
                       Items::ind->value("GREAT BALL")->ind, 1));
}
