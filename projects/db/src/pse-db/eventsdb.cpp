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
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./eventsdb.h"
#include "./entries/eventdbentry.h"
#include "./util/gamedata.h"
#include "./mapsdb.h"

EventsDB* EventsDB::inst()
{
  static EventsDB* _inst = new EventsDB;
  return _inst;
}

const QVector<EventDBEntry*> EventsDB::getStore() const
{
  return store;
}

const QHash<QString, EventDBEntry*> EventsDB::getInd() const
{
  return ind;
}

int EventsDB::getStoreSize() const
{
  return store.size();
}

const EventDBEntry* EventsDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const EventDBEntry* EventsDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}

void EventsDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Events
  auto jsonData = GameData::inst()->json("events");

  // Go through each event
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event
    auto entry = new EventDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void EventsDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }

  once = true;
}

void EventsDB::deepLink()
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

void EventsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void EventsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<EventsDB>("PSE.DB.EventsDB", 1, 0, "EventsDB", "Can't instantiate in QML");
  once = true;
}

EventsDB::EventsDB()
{
  qmlRegister();
  load();
}
