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

#include "./events.h"
#include "./util/gamedata.h"
#include "./maps.h"

EventDBEntry::EventDBEntry() {}
EventDBEntry::EventDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  byte = data["byte"].toDouble();
  bit = data["bit"].toDouble();

  for(auto eventMap : data["maps"].toArray())
    maps.append(eventMap.toString());
}

void EventDBEntry::deepLink()
{
  for(auto map : maps)
  {
    auto tmp = MapsDB::ind.value(map, nullptr);
    toMaps.append(tmp);

#ifdef QT_DEBUG
    if(tmp == nullptr)
      qCritical() << "Events: " << name << ", could not be deep linked to map " << map ;
#endif

    if(tmp != nullptr)
      tmp->toEvents.append(this);
  }
}

void EventsDB::load()
{
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
}

void EventsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

void EventsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<EventDBEntry*> EventsDB::store = QVector<EventDBEntry*>();
QHash<QString, EventDBEntry*> EventsDB::ind = QHash<QString, EventDBEntry*>();
