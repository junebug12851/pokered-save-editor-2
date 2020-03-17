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
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./itemsdb.h"
#include "./util/gamedata.h"
#include "./moves.h"
#include "./gamecornerdb.h"
#include "./entries/itemdbentry.h"

ItemsDB* ItemsDB::inst()
{
  static ItemsDB* _inst = new ItemsDB;
  return _inst;
}

const QVector<ItemDBEntry*> ItemsDB::getStore() const
{
  return store;
}

const QHash<QString, ItemDBEntry*> ItemsDB::getInd() const
{
  return ind;
}

int ItemsDB::getStoreSize() const
{
  return store.size();
}

const ItemDBEntry* ItemsDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const ItemDBEntry* ItemsDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}

void ItemsDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Item Data
  auto jsonData = GameData::inst()->json("items");

  // Go through each item
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new item entry
    auto entry = new ItemDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void ItemsDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(entry->readable, entry);
    ind.insert(QString::number(entry->ind), entry);

    if(entry->tm > 0)
      ind.insert("tm" + QString::number(entry->tm), entry);
    if(entry->hm > 0)
      ind.insert("hm" + QString::number(entry->hm), entry);
  }

  once = true;
}

void ItemsDB::deepLink()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    entry->deepLink();
  }

  once = true;
}

void ItemsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void ItemsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<ItemsDB>(
        "PSE.DB.ItemsDB", 1, 0, "ItemsDB", "Can't instantiate in QML");
  once = true;
}

ItemsDB::ItemsDB()
{
  qmlRegister();
  load();
}
