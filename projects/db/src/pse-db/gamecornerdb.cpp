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
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./gamecornerdb.h"
#include "./util/gamedata.h"
#include "./pokemon.h"
#include "./itemsdb.h"
#include "./entries/gamecornerdbentry.h"

void GameCornerDB::load()
{
  static bool once = false;
  if(once)
    return;

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

  once = true;
}

void GameCornerDB::deepLink()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    entry->deepLink();
  }

  once = true;
}

void GameCornerDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void GameCornerDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<GameCornerDB>(
        "PSE.DB.GameCornerDB", 1, 0, "GameCornerDB", "Can't instantiate in QML");
  once = true;
}

GameCornerDB::GameCornerDB()
{
  qmlRegister();
  load();
}

GameCornerDB* GameCornerDB::inst()
{
  static GameCornerDB* _inst = new GameCornerDB;
  return _inst;
}

const QVector<GameCornerDBEntry*> GameCornerDB::getStore() const
{
  return store;
}

int GameCornerDB::getStoreSize() const
{
  return store.size();
}

const GameCornerDBEntry* GameCornerDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

int GameCornerDB::getBuyPrice() const
{
    return buyPrice;
}
