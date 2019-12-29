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
#include "items.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

ItemEntry::ItemEntry()
{
  once = false;
  glitch = false;
}

void Items::load()
{
  // Grab Item Data
  auto itemData = GameData::json("items");

  // Go through each item
  for(QJsonValue itemEntry : itemData->array())
  {
    // Create a new item entry
    auto entry = new ItemEntry();

    // Set simple properties
    entry->name = itemEntry["name"].toString();
    entry->ind = itemEntry["ind"].toDouble();

    // Set simple optional properties
    if(itemEntry["once"].isBool())
      entry->once = itemEntry["once"].toBool();

    if(itemEntry["glitch"].isBool())
      entry->glitch = itemEntry["glitch"].toBool();

    if(itemEntry["tm"].isDouble())
      entry->tm = itemEntry["tm"].toDouble();

    if(itemEntry["hm"].isDouble())
      entry->hm = itemEntry["hm"].toDouble();

    // Add to array
    items->append(entry);
  }
}

QVector<ItemEntry*>* Items::items = new QVector<ItemEntry*>();
