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
#include "./gamedata.h"
#include "./maps.h"

void EventsDB::load()
{
  // Grab Events
  auto eventData = GameData::json("events");

  // Go through each event
  for(QJsonValue eventEntry : eventData->array())
  {
    // Create a new event
    auto entry = new EventDBEntry();

    // Set simple properties
    entry->name = eventEntry["name"].toString();
    entry->ind = eventEntry["ind"].toDouble();
    entry->byte = eventEntry["byte"].toDouble();
    entry->bit = eventEntry["bit"].toDouble();

    for(auto eventMap : eventEntry["maps"].toArray())
      entry->maps.append(eventMap.toString());

    // Add to array
    store.append(entry);
  }

  delete eventData;
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
    for(auto map : entry->maps)
    {
      auto tmp = MapsDB::ind.value(map, nullptr);
      entry->toMaps.append(tmp);

#ifdef QT_DEBUG
      if(tmp == nullptr)
        qCritical() << "Events: " << entry->name << ", could not be deep linked to map " << map ;
#endif

      if(tmp != nullptr)
        tmp->toEvents.append(entry);
    }
  }
}

QVector<EventDBEntry*> EventsDB::store = QVector<EventDBEntry*>();
QHash<QString, EventDBEntry*> EventsDB::ind = QHash<QString, EventDBEntry*>();
