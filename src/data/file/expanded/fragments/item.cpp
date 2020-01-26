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
#include "./item.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/items.h"
#include "../../../../random.h"

Item::Item(SaveFileIterator* it)
{
  if(it == nullptr) {
    reset();
    return;
  }

  ind = it->getByte();
  amount = it->getByte();
}

Item::Item(var8 ind, var8 amount)
{
  load(ind, amount);
}

Item::Item(bool random)
{
  load(random);
}

Item::Item(QString name, var8 amount)
{
  load(name, amount);
}

Item::~Item() {}

void Item::save(SaveFileIterator* it)
{
  it->setByte(ind);
  it->setByte(amount);
}

void Item::reset()
{
  ind = 0;
  indChanged();

  amount = 0;
  amountChanged();
}

void Item::randomize()
{
  auto tmp = ItemsDB::store.at(Random::rangeExclusive(0, ItemsDB::store.size()));

  // No Glitch or Items only gotten once in the game
  while(tmp->glitch || tmp->once)
    tmp = ItemsDB::store.at(Random::rangeExclusive(0, ItemsDB::store.size()));

  ind = tmp->ind;
  indChanged();

  amount = Random::rangeInclusive(1, 5); // Between 1 and 5 of them
  amountChanged();
}

ItemDBEntry* Item::toItem()
{
  auto tmp = ItemsDB::ind.value(QString::number(ind), nullptr);
  return tmp;
}

void Item::load(int ind, int amount)
{
  this->ind = ind;
  this->amount = amount;
}

void Item::load(bool random)
{
  if(!random) {
    reset();
    return;
  }

  randomize();
}

void Item::load(QString name, int amount)
{
  auto tmp = ItemsDB::ind.value(name, nullptr);
  if(tmp != nullptr)
    ind = tmp->ind;

  this->amount = amount;
}
