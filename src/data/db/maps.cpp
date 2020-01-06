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

#include "./maps.h"
#include "./tileset.h"
#include "./music.h"
#include "./gamedata.h"

MapDBEntry::MapDBEntry()
{
  special = false;
  glitch = false;
}

std::optional<var8> MapDBEntry::height2X2()
{
  if(height)
    return *height * 2;

  return std::optional<var8>();
}

std::optional<var8> MapDBEntry::width2X2()
{
  if(width)
    return *width * 2;

  return std::optional<var8>();
}

void MapsDB::load()
{
  // Grab Map Data
  auto mapData = GameData::json("maps");

  // Go through each map
  for(QJsonValue mapEntry : mapData->array())
  {
    // Create a new map entry
    auto entry = new MapDBEntry();

    // Set simple properties
    entry->name = mapEntry["name"].toString();
    entry->ind = mapEntry["ind"].toDouble();

    // Set simple optional properties
    if(mapEntry["special"].isBool())
      entry->special = mapEntry["special"].toBool();

    if(mapEntry["glitch"].isBool())
      entry->glitch = mapEntry["glitch"].toBool();

    if(mapEntry["bank"].isDouble())
      entry->bank = mapEntry["bank"].toDouble();

    if(mapEntry["dataPtr"].isDouble())
      entry->dataPtr = mapEntry["dataPtr"].toDouble();

    if(mapEntry["scriptPtr"].isDouble())
      entry->scriptPtr = mapEntry["scriptPtr"].toDouble();

    if(mapEntry["textPtr"].isDouble())
      entry->textPtr = mapEntry["textPtr"].toDouble();

    if(mapEntry["width"].isDouble())
      entry->width = mapEntry["width"].toDouble();

    if(mapEntry["height"].isDouble())
      entry->height = mapEntry["height"].toDouble();

    if(mapEntry["music"].isString())
      entry->music = mapEntry["music"].toString();

    if(mapEntry["tileset"].isString())
      entry->tileset = mapEntry["tileset"].toString();

    if(mapEntry["modernName"].isString())
      entry->modernName = mapEntry["modernName"].toString();

    if(mapEntry["incomplete"].isString())
      entry->incomplete = mapEntry["incomplete"].toString();

    // Add to array
    store.append(entry);
  }

  delete mapData;
}

void MapsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);

    // Also insert the modern name if present
    if(entry->modernName != "")
      ind.insert(entry->modernName, entry);
  }
}

void MapsDB::deepLink()
{
  for(auto entry : store)
  {
    if(entry->music != "")
      entry->toMusic = MusicDB::ind.value(entry->music, nullptr);

    if(entry->tileset != "")
      entry->toTileset = TilesetDB::ind.value(entry->tileset, nullptr);

    if(entry->incomplete != "")
      entry->toComplete = MapsDB::ind.value(entry->incomplete, nullptr);

#ifdef QT_DEBUG
    if(entry->music != "" && entry->toMusic == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to music" << entry->music;

    if(entry->tileset != "" && entry->toTileset == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to tileset" << entry->tileset;

    if(entry->incomplete != "" && entry->toComplete == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to complete" << entry->incomplete;
#endif
  }
}

QVector<MapDBEntry*> MapsDB::store = QVector<MapDBEntry*>();
QHash<QString, MapDBEntry*> MapsDB::ind = QHash<QString, MapDBEntry*>();
