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
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "itemdbentry.h"
#include "../moves.h"
#include "./gamecornerdbentry.h"
#include "../gamecornerdb.h"

ItemDBEntry::ItemDBEntry() {
  qmlRegister();
}
ItemDBEntry::ItemDBEntry(const QJsonValue& data)
{
  qmlRegister();

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
  if(tm > 0 && hm <= 0)
    toMove = MovesDB::ind.value("tm" + QString::number(tm), nullptr);
  else if(tm > 0 && hm > 0)
    toMove = MovesDB::ind.value("hm" + QString::number(hm), nullptr);

#ifdef QT_DEBUG
  if((tm || hm) && toMove == nullptr)
    qCritical() << "Item: " << name << ", could not be deep linked." ;
#endif
}

void ItemDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<ItemDBEntry>(
        "PSE.DB.ItemDBEntry", 1, 0, "ItemDBEntry", "Can't instantiate in QML");
  once = true;
}

const QVector<PokemonDBEntry*> ItemDBEntry::getToTeachPokemon() const
{
  return toTeachPokemon;
}

int ItemDBEntry::getToTeachPokemonSize() const
{
  return toTeachPokemon.size();
}

const PokemonDBEntry* ItemDBEntry::getToTeachPokemonAt(const int ind) const
{
  if(ind >= toTeachPokemon.size())
    return nullptr;

  return toTeachPokemon.at(ind);
}

void ItemDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const QVector<PokemonDBEntryEvolution*> ItemDBEntry::getToEvolvePokemon() const
{
  return toEvolvePokemon;
}

int ItemDBEntry::getToEvolvePokemonSize() const
{
  return toEvolvePokemon.size();
}

const PokemonDBEntryEvolution* ItemDBEntry::getToEvolvePokemonAt(const int ind) const
{
  if(ind >= toEvolvePokemon.size())
    return nullptr;

  return toEvolvePokemon.at(ind);
}

const QVector<MapDBEntrySpriteItem*> ItemDBEntry::getToMapSpriteItem() const
{
  return toMapSpriteItem;
}

int ItemDBEntry::getToMapSpriteItemSize() const
{
  return toMapSpriteItem.size();
}

const MapDBEntrySpriteItem* ItemDBEntry::getToMapSpriteItemAt(const int ind) const
{
  if(ind >= toMapSpriteItem.size())
    return nullptr;

  return toMapSpriteItem.at(ind);
}

const GameCornerDBEntry* ItemDBEntry::getToGameCorner() const
{
    return toGameCorner;
}

const MoveDBEntry* ItemDBEntry::getToMove() const
{
    return toMove;
}

int ItemDBEntry::getPrice() const
{
    return price;
}

int ItemDBEntry::getHm() const
{
    return hm;
}

int ItemDBEntry::getTm() const
{
    return tm;
}

const QString ItemDBEntry::getReadable() const
{
    return readable;
}

bool ItemDBEntry::getGlitch() const
{
    return glitch;
}

bool ItemDBEntry::getOnce() const
{
    return once;
}

int ItemDBEntry::getInd() const
{
    return ind;
}

const QString ItemDBEntry::getName() const
{
    return name;
}

int ItemDBEntry::buyPriceMoney() const
{
    if(price <= 0)
        return 0;

  return price;
}

int ItemDBEntry::buyPriceCoins() const
{
  // Sometimes there might be a duplicate game corner price
  // We want to pick the game-corner price over market price anytime

  int val = 0;

  // If there's a game corner price attached, always use that. It will be in
  // coins.
  if(toGameCorner != nullptr)
    val = toGameCorner->getPrice();

  // Otherwise if there's no market price or the market price is 0 then keep it
  // at 0
  else if(price <= 0)
    val = 0;

  // In all other cases, divide market price by Game Corner exchange rate to
  // produce the coins amount
  else
    val = price / GameCornerDB::inst()->getBuyPrice();

  // Return whatever value we have
  return val;
}

// These are simpler, we did all the work above, here we just divide by 2 which
// is the global sell-back ratio for anything

int ItemDBEntry::sellPriceMoney() const
{
  return buyPriceMoney() / 2;
}

int ItemDBEntry::sellPriceCoins() const
{
  return buyPriceCoins() / 2;
}

// This is a bit tricky
// We can sell an item, even if the sell price is zero. But if there's no listed
// price then we can't sell.
// Non-sellable items are considered key items and must be tossed
bool ItemDBEntry::canSell() const
{
  return price >= 0;
}

bool ItemDBEntry::isGameCornerExclusive() const
{
  return toGameCorner != nullptr;
}
