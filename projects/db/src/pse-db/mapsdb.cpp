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

#include <QJsonArray>
#include <pse-common/utility.h>
#include <QQmlEngine>

#include "./mapsdb.h"
#include "./util/gamedata.h"
#include "./util/mapsearch.h"
#include "./entries/mapdbentry.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

MapsDB* MapsDB::inst()
{
  static MapsDB* _inst = new MapsDB;
  return _inst;
}

const QVector<MapDBEntry*> MapsDB::getStore() const
{
  return store;
}

const QHash<QString, MapDBEntry*> MapsDB::getInd() const
{
  return ind;
}

int MapsDB::getStoreSize() const
{
  return store.size();
}

void MapsDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Map Data
  auto jsonData = GameData::inst()->json("maps");

  // Go through each map
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new map entry
    auto entry = new MapDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void MapsDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);

    // Also insert the modern name if present
    if(entry->modernName != "")
      ind.insert(entry->modernName, entry);
  }

  once = true;
}

void MapsDB::deepLink()
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

void MapsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void MapsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapsDB>(
        "PSE.DB.MapsDB", 1, 0, "MapsDB", "Can't instantiate in QML");
  once = true;
}

MapsDB::MapsDB()
{
  qmlRegister();
  load();
}

MapSearch* MapsDB::searchRaw() const
{
  return new MapSearch();
}

const QScopedPointer<const MapSearch, QScopedPointerDeleteLater> MapsDB::search() const
{
  return QScopedPointer<const MapSearch, QScopedPointerDeleteLater>(
        new MapSearch());
}

const MapDBEntry* MapsDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const MapDBEntry* MapsDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}
