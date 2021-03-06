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

#include "./flydb.h"
#include "./util/gamedata.h"
#include "./mapsdb.h"
#include "./entries/flydbentry.h"

FlyDB* FlyDB::inst()
{
  static FlyDB* _inst = new FlyDB;
  return _inst;
}

const QVector<FlyDBEntry*> FlyDB::getStore() const
{
  return store;
}

const QHash<QString, FlyDBEntry*> FlyDB::getInd() const
{
  return ind;
}

int FlyDB::getStoreSize() const
{
  return store.size();
}

const FlyDBEntry* FlyDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const FlyDBEntry* FlyDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}

void FlyDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("fly");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new FlyDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void FlyDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }

  once = true;
}

void FlyDB::deepLink()
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

void FlyDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void FlyDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<FlyDB>("PSE.DB.FlyDB", 1, 0, "FlyDB", "Can't instantiate in QML");
  once = true;
}

FlyDB::FlyDB()
{
  qmlRegister();
  load();
}
