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

#include <QJsonArray>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./gamecorner.h"
#include "./util/gamedata.h"
#include "./pokemon.h"
#include "./items.h"

GameCornerDBEntry::GameCornerDBEntry() {}
GameCornerDBEntry::GameCornerDBEntry(QJsonValue& data)
{
  name = data["name"].toString();
  type = data["type"].toString();

  if(type != "pokemon")
    price = data["price"].toDouble();

  if(type == "money") {
    GameCornerDB::buyPrice = price;
    GameCornerDB::sellPrice = price / 2;
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

void GameCornerDB::load()
{
  // Grab Item Data
  auto jsonData = GameData::inst()->json("gameCorner");

  // Go through each item
  for(QJsonValue jsonEntry : jsonData.array())
  {
    if(jsonEntry["options"].isArray()) {
      for(QJsonValue option : jsonEntry["options"].toArray()) {

        // Create a new item entry
        auto entry = new GameCornerDBEntry(jsonEntry);
        entry->level = option["level"].toDouble();
        entry->price = option["price"].toDouble();

        // Add to array
        store.append(entry);
      }
    }
    else {
      auto entry = new GameCornerDBEntry(jsonEntry);

      // Add to array
      store.append(entry);
    }
  }
}

void GameCornerDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

int GameCornerDB::buyPrice = 0;
int GameCornerDB::sellPrice = 0;
QVector<GameCornerDBEntry*> GameCornerDB::store = QVector<GameCornerDBEntry*>();
