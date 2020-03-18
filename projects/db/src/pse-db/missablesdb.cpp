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

#include <QJsonValue>
#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "./missablesdb.h"
#include "./util/gamedata.h"
#include "./entries/missabledbentry.h"

MissablesDB* MissablesDB::inst()
{
  static MissablesDB* _inst = new MissablesDB;
  return _inst;
}

const QVector<MissableDBEntry*> MissablesDB::getStore() const
{
  return store;
}

const QHash<QString, MissableDBEntry*> MissablesDB::getInd() const
{
  return ind;
}

int MissablesDB::getStoreSize() const
{
  return store.size();
}

const MissableDBEntry* MissablesDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const MissableDBEntry* MissablesDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}

void MissablesDB::load()
{
  static bool once = false;
  if(once)
    return;

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

  once = true;
}

void MissablesDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    // Name is actually map + name, the name is the local name to the map
    // and is often duplicated across maps
    ind.insert(entry->map + " " + entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }

  once = true;
}

void MissablesDB::deepLink()
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

void MissablesDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void MissablesDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MissablesDB>("PSE.DB.MissablesDB", 1, 0, "MissablesDB", "Can't instantiate in QML");
  once = true;
}

MissablesDB::MissablesDB()
{
  qmlRegister();
  load();
}
