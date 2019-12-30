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
#include "./moves.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

ItemEntry::ItemEntry()
{
  once = false;
  glitch = false;

  toMove = nullptr;
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

void Items::index()
{
  for(auto entry : *items)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);

    if(entry->tm)
      ind->insert("tm" + QString::number(*entry->tm), entry);
    if(entry->hm)
      ind->insert("hm" + QString::number(*entry->hm), entry);
  }
}

void Items::deepLink()
{
  for(auto entry : *items)
  {
    if(entry->tm && !entry->hm)
      entry->toMove = Moves::ind->value("tm" + QString::number(*entry->tm), nullptr);
    else if(entry->tm && entry->hm)
      entry->toMove = Moves::ind->value("hm" + QString::number(*entry->hm), nullptr);

#ifdef QT_DEBUG
    if((entry->tm || entry->hm) && entry->toMove == nullptr)
      qCritical() << "Item: " << entry->name << ", could not be deep linked." ;
#endif
  }
}

QVector<ItemEntry*>* Items::items = new QVector<ItemEntry*>();
QHash<QString, ItemEntry*>* Items::ind = new QHash<QString, ItemEntry*>();
