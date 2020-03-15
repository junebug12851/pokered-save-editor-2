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

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValueRef>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./eventpokemondb.h"
#include "./entries/eventpokemondbentry.h"
#include "./util/gamedata.h"
#include "./pokemon.h"

EventPokemonDB* EventPokemonDB::inst()
{
  static EventPokemonDB* _inst = new EventPokemonDB;
  return _inst;
}

const QVector<EventPokemonDBEntry*> EventPokemonDB::getStore() const
{
  return store;
}

int EventPokemonDB::getStoreSize() const
{
  return store.size();
}

const EventPokemonDBEntry* EventPokemonDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

void EventPokemonDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("eventPokemon");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new EventPokemonDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void EventPokemonDB::deepLink()
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

void EventPokemonDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void EventPokemonDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<EventPokemonDB>("PSE.DB.EventPokemonDB", 1, 0, "EventPokemonDB", "Can't instantiate in QML");
  once = true;
}

EventPokemonDB::EventPokemonDB()
{
  qmlRegister();
  load();
}
