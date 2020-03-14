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
#include <pse-db/items.h>
#include <pse-common/random.h>

Item::Item(SaveFileIterator* it)
{
  if(it == nullptr) {
    reset();
    return;
  }

  ind = it->getByte();
  amount = it->getByte();
  makeConnect();
}

Item::Item(var8 ind, var8 amount)
{
  load(ind, amount);
  makeConnect();
}

Item::Item(bool random)
{
  load(random);
  makeConnect();
}

Item::Item(QString name, var8 amount)
{
  load(name, amount);
  makeConnect();
}

void Item::makeConnect()
{
  connect(this, &Item::indChanged, this, &Item::itemChanged);
  connect(this, &Item::amountChanged, this, &Item::itemChanged);
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

bool Item::canSell()
{
  auto el = toItem();

  if(el == nullptr)
    return false;
  else
    return el->canSell();
}

int Item::buyPriceOneMoney()
{
  auto el = toItem();

  if(el == nullptr)
    return 0;
  else
    return el->buyPriceMoney();
}

int Item::buyPriceOneCoins()
{
  auto el = toItem();

  if(el == nullptr)
    return 0;
  else
    return el->buyPriceCoins();
}

int Item::sellPriceOneMoney()
{
  auto el = toItem();

  if(el == nullptr)
    return 0;
  else
    return el->sellPriceMoney();
}

int Item::sellPriceOneCoins()
{
  auto el = toItem();

  if(el == nullptr)
    return 0;
  else
    return el->sellPriceCoins();
}

int Item::buyPriceAllMoney()
{
  return buyPriceOneMoney() * amount;
}

int Item::buyPriceAllCoins()
{
  return buyPriceOneCoins() * amount;
}

int Item::sellPriceAllMoney()
{
  return sellPriceOneMoney() * amount;
}

int Item::sellPriceAllCoins()
{
  return  sellPriceOneCoins() * amount;
}

int Item::getAmount()
{
  return amount;
}

void Item::setAmount(int val)
{
  amount = val;
  if(amount < 1)
    amount = 1;
  else if(amount > 99)
    amount = 99;
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
