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

#include "./items.h"
#include "./gamedata.h"
#include "./moves.h"

ItemDBEntry::ItemDBEntry() {}
ItemDBEntry::ItemDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  readable = data["readable"].toString();

  // Set simple optional properties
  if(data["once"].isBool())
    once = data["once"].toBool();

  if(data["glitch"].isBool())
    glitch = data["glitch"].toBool();

  if(data["tm"].isDouble())
    tm = data["tm"].toDouble();

  if(data["hm"].isDouble())
    hm = data["hm"].toDouble();

  if(data["price"].isDouble())
    price = data["price"].toDouble();
}

void ItemDBEntry::deepLink()
{
  if(tm && !hm)
    toMove = MovesDB::ind.value("tm" + QString::number(*tm), nullptr);
  else if(tm && hm)
    toMove = MovesDB::ind.value("hm" + QString::number(*hm), nullptr);

#ifdef QT_DEBUG
  if((tm || hm) && toMove == nullptr)
    qCritical() << "Item: " << name << ", could not be deep linked." ;
#endif
}

void ItemsDB::load()
{
  // Grab Item Data
  auto itemData = GameData::json("items");

  // Go through each item
  for(QJsonValue itemEntry : itemData->array())
  {
    // Create a new item entry
    auto entry = new ItemDBEntry(itemEntry);

    // Add to array
    store.append(entry);
  }

  delete itemData;
}

void ItemsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(entry->readable, entry);
    ind.insert(QString::number(entry->ind), entry);

    if(entry->tm)
      ind.insert("tm" + QString::number(*entry->tm), entry);
    if(entry->hm)
      ind.insert("hm" + QString::number(*entry->hm), entry);
  }
}

void ItemsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<ItemDBEntry*> ItemsDB::store = QVector<ItemDBEntry*>();
QHash<QString, ItemDBEntry*> ItemsDB::ind = QHash<QString, ItemDBEntry*>();
