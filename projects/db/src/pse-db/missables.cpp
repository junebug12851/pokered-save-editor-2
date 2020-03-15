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
#include "./util/gamedata.h"
#include "./maps.h"

MissableDBEntry::MissableDBEntry() {}
MissableDBEntry::MissableDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  map = data["map"].toString();
  sprite = data["sprite"].toDouble();
  defShow = data["defVal"].toString() == "Show";
}

void MissableDBEntry::deepLink()
{
  toMap = MapsDB::ind.value(map, nullptr);

  // Deep link map sprite only if toMap is valid and sprite is a valid range
  if(toMap != nullptr && sprite < toMap->sprites.size())
  {
    toMapSprite = toMap->sprites.at(sprite);
  }

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Missables: " << name << ", could not be deep linked to map" << map;

  if(toMapSprite == nullptr &&

     // Don't warn about these errors as they're not my errors, they're
     // gen 1 errors

     // This is a valid map that refers to an extra sprite not on the map
     ((map == "Silph Co 7F" &&
     sprite != 11) ||

     // This is an invalid map with no sprites
     (map == "Unused Map F4" &&
     sprite != 1))

     )
    qCritical() << "Missables: " << name << ", could not be deep linked to map " << map << " sprite #" << sprite;
#endif
}

void MissablesDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("missables");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new MissableDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
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
    entry->deepLink();
  }
}

QVector<MissableDBEntry*> MissablesDB::store = QVector<MissableDBEntry*>();
QHash<QString, MissableDBEntry*> MissablesDB::ind =
    QHash<QString, MissableDBEntry*>();
