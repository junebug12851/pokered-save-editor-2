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

#include "./scripts.h"
#include "./util/gamedata.h"
#include "./maps.h"

ScriptDBEntry::ScriptDBEntry() {}
ScriptDBEntry::ScriptDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  size = data["size"].toDouble();

  if(data["skip"].isDouble())
    skip = data["skip"].toDouble();

  // If maps is given use that, otherwise use name
  if(data["maps"].isArray()) {
    for(auto mapEntry : data["maps"].toArray())
      maps.append(mapEntry.toString());
  }
  else
    maps.append(name);
}

void ScriptDBEntry::deepLink()
{
  for(auto mapEntry : maps) {
    auto map = MapsDB::ind.value(mapEntry, nullptr);
    toMaps.append(map);

#ifdef QT_DEBUG
  if(map == nullptr)
    qCritical() << "Script Entry: " << name << ", could not be deep linked to map " << mapEntry;
#endif

  if(map != nullptr)
    map->toScript = this;
  }
}

void ScriptsDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("scripts");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new ScriptDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void ScriptsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

void ScriptsDB::deepLink()
{
  for(auto entry : store) {
    entry->deepLink();
  }
}

QVector<ScriptDBEntry*> ScriptsDB::store = QVector<ScriptDBEntry*>();
QHash<QString, ScriptDBEntry*> ScriptsDB::ind = QHash<QString, ScriptDBEntry*>();
