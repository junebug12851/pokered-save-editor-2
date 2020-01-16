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

#include "./missables.h"
#include "./gamedata.h"
#include "./maps.h"

void MissablesDB::load()
{
  // Grab Event Pokemon Data
  auto missableData = GameData::json("missables");

  // Go through each event Pokemon
  for(QJsonValue missableEntry : missableData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new MissableDBEntry();

    // Set simple properties
    entry->name = missableEntry["name"].toString();
    entry->ind = missableEntry["ind"].toDouble();
    entry->map = missableEntry["map"].toString();
    entry->sprite = missableEntry["sprite"].toDouble();
    entry->defShow = missableEntry["defVal"].toString() == "Show";

    // Add to array
    store.append(entry);
  }

  delete missableData;
}

void MissablesDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    // Name is actually map + name, the name is the local name to the map
    // and is often duplicated across maps
    ind.insert(entry->map + " " + entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

void MissablesDB::deepLink()
{
  for(auto entry : store)
  {
    entry->toMap = MapsDB::ind.value(entry->map, nullptr);

    // Deep link map sprite only if toMap is valid and sprite is a valid range
    if(entry->toMap != nullptr && entry->sprite < entry->toMap->sprites.size())
    {
      entry->toMapSprite = entry->toMap->sprites.at(entry->sprite);
    }

#ifdef QT_DEBUG
    if(entry->toMap == nullptr)
      qCritical() << "Missables: " << entry->name << ", could not be deep linked to map" << entry->map;

    if(entry->toMapSprite == nullptr &&

       // Don't warn about these errors as they're not my errors, they're
       // gen 1 errors

       // This is a valid map that refers to an extra sprite not on the map
       (entry->map == "Silph Co 7F" &&
       entry->sprite != 11) &&

       // This is an invalid map with no sprites
       (entry->map == "Unused Map F4" &&
       entry->sprite != 1)

       )
      qCritical() << "Missables: " << entry->name << ", could not be deep linked to map " << entry->map << " sprite #" << entry->sprite;
#endif
  }
}

QVector<MissableDBEntry*> MissablesDB::store = QVector<MissableDBEntry*>();
QHash<QString, MissableDBEntry*> MissablesDB::ind =
    QHash<QString, MissableDBEntry*>();
