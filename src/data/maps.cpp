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
#include "maps.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

MapEntry::MapEntry()
{
  special = false;
  glitch = false;
}

std::optional<var8> MapEntry::height2X2()
{
  if(height)
    return *height * 2;

  return std::optional<var8>();
}

std::optional<var8> MapEntry::width2X2()
{
  if(width)
    return *width * 2;

  return std::optional<var8>();
}

void Maps::load()
{
  // Grab Map Data
  auto mapData = GameData::json("maps");

  // Go through each map
  for(QJsonValue mapEntry : mapData->array())
  {
    // Create a new map entry
    auto entry = new MapEntry();

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

    // Add to array
    maps->append(entry);
  }
}

QVector<MapEntry*>* Maps::maps = new QVector<MapEntry*>();
