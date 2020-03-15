/*
  * Copyright 2019 June Hanabi
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

#include <QVector>
#include <QJsonArray>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./items.h"
#include "./util/gamedata.h"
#include "./moves.h"
#include "./gamecorner.h"

ItemDBEntry::ItemDBEntry() {}
ItemDBEntry::ItemDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  readable = data["readable"].toString();

  // Set simple optional properties
  if(data["once"].isBool())
    once = data["once"].toBool();

  if(data["glitch"].isBool())
    glitch = data["glitch"].toBool();

  if(data["tm"].isDouble())
    tm = data["tm"].toDouble();

  if(data["hm"].isDouble())
    hm = data["hm"].toDouble();

  if(data["price"].isDouble())
    price = data["price"].toDouble();
}

void ItemDBEntry::deepLink()
{
  if(tm && !hm)
    toMove = MovesDB::ind.value("tm" + QString::number(*tm), nullptr);
  else if(tm && hm)
    toMove = MovesDB::ind.value("hm" + QString::number(*hm), nullptr);

#ifdef QT_DEBUG
  if((tm || hm) && toMove == nullptr)
    qCritical() << "Item: " << name << ", could not be deep linked." ;
#endif
}

int ItemDBEntry::buyPriceMoney()
{
  if(!price)
    return 0;

  return *price;
}

int ItemDBEntry::buyPriceCoins()
{
  // Sometimes there might be a duplicate game corner price
  // We want to pick the game-corner price over market price anytime

  int val = 0;

  // If there's a game corner price attached, always use that. It will be in
  // coins.
  if(toGameCorner != nullptr)
    val = toGameCorner->price;

  // Otherwise if there's no market price or the market price is 0 then keep it
  // at 0
  else if(!price || (*price) == 0)
    val = 0;

  // In all other cases, divide market price by Game Corner exchange rate to
  // produce the coins amount
  else
    val = (*price) / GameCornerDB::buyPrice;

  // Return whatever value we have
  return val;
}

// These are simpler, we did all the work above, here we just divide by 2 which
// is the global sell-back ratio for anything

int ItemDBEntry::sellPriceMoney()
{
  int val = buyPriceMoney();
  if(val == 0)
    return 0;

  return val / 2;
}

int ItemDBEntry::sellPriceCoins()
{
  int val = buyPriceCoins();
  if(val == 0)
    return 0;

  return val / 2;
}

// This is a bit tricky
// We can sell an item, even if the sell price is zero. But if there's no listed
// price then we can't sell.
// Non-sellable items are considered key items and must be tossed
bool ItemDBEntry::canSell()
{
  return price.has_value();
}

bool ItemDBEntry::isGameCornerExclusive()
{
  return toGameCorner != nullptr;
}

void ItemsDB::load()
{
  // Grab Item Data
  auto jsonData = GameData::inst()->json("items");

  // Go through each item
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new item entry
    auto entry = new ItemDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void ItemsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(entry->readable, entry);
    ind.insert(QString::number(entry->ind), entry);

    if(entry->tm)
      ind.insert("tm" + QString::number(*entry->tm), entry);
    if(entry->hm)
      ind.insert("hm" + QString::number(*entry->hm), entry);
  }
}

void ItemsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<ItemDBEntry*> ItemsDB::store = QVector<ItemDBEntry*>();
QHash<QString, ItemDBEntry*> ItemsDB::ind = QHash<QString, ItemDBEntry*>();
