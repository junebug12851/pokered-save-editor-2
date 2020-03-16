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

#include <QVector>
#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "../eventsdb.h"
#include "../util/gamedata.h"
#include "../maps.h"

#include "eventdbentry.h"

EventDBEntry::EventDBEntry() {
  qmlRegister();
}
EventDBEntry::EventDBEntry(QJsonValue& data)
{
  qmlRegister();
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

void EventDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<EventDBEntry>("PSE.DB.EventDBEntry", 1, 0, "EventDBEntry", "Can't instantiate in QML");
  once = true;
}

const QVector<MapDBEntry*> EventDBEntry::getToMaps() const
{
  return toMaps;
}

int EventDBEntry::getToMapsSize() const
{
  return toMaps.size();
}

const MapDBEntry* EventDBEntry::getToMapAt(int ind) const
{
  if(ind >= toMaps.size())
    return nullptr;

  return toMaps.at(ind);
}

void EventDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const QVector<QString> EventDBEntry::getMaps() const
{
  return maps;
}

int EventDBEntry::getMapsSize() const
{
  return maps.size();
}

const QString EventDBEntry::getMapAt(int ind) const
{
  if(ind >= maps.size())
    return nullptr;

  return maps.at(ind);
}

int EventDBEntry::getBit() const
{
    return bit;
}

int EventDBEntry::getByte() const
{
    return byte;
}

int EventDBEntry::getInd() const
{
    return ind;
}

const QString EventDBEntry::getName() const
{
    return name;
}
