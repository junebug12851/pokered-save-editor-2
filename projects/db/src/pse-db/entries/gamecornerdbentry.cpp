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

#include "../pokemon.h"
#include "../itemsdb.h"
#include "../gamecornerdb.h"
#include "gamecornerdbentry.h"

GameCornerDBEntry::GameCornerDBEntry() {
  qmlRegister();
}
GameCornerDBEntry::GameCornerDBEntry(const QJsonValue& data)
{
  qmlRegister();

  name = data["name"].toString();
  type = data["type"].toString();

  if(type != "pokemon")
    price = data["price"].toDouble();

  if(type == "money") {
    GameCornerDB::inst()->buyPrice = price;
  }
}

void GameCornerDBEntry::deepLink() {
  if(type != "pokemon" &&
     type != "tm")
    return;

  if(type == "pokemon") {
    toPokemon = PokemonDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
    if(toPokemon == nullptr)
      qCritical() << "Game Corner: " << name << ", could not be deep linked." ;
#endif

    if(toPokemon == nullptr)
      return;

    // Cross-Deep Link
    toPokemon->toGameCorner.append(this);
  }
  else if(type == "tm") {
    toItem = ItemsDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
    if(toItem == nullptr)
      qCritical() << "Game Corner: " << name << ", could not be deep linked." ;
#endif

    if(toItem == nullptr)
      return;

    // Cross-Deep Link
    toItem->toGameCorner = this;
  }
}

void GameCornerDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<GameCornerDBEntry>(
        "PSE.DB.GameCornerDBEntry", 1, 0, "GameCornerDBEntry", "Can't instantiate in QML");
  once = true;
}

const ItemDBEntry* GameCornerDBEntry::getToItem() const
{
  return toItem;
}

void GameCornerDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const PokemonDBEntry* GameCornerDBEntry::getToPokemon() const
{
    return toPokemon;
}

int GameCornerDBEntry::getLevel() const
{
    return level;
}

int GameCornerDBEntry::getPrice() const
{
    return price;
}

const QString GameCornerDBEntry::getType() const
{
    return type;
}

const QString GameCornerDBEntry::getName() const
{
    return name;
}
