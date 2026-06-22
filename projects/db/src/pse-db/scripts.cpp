/*
  * Copyright 2019 Twilight
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

/**
 * @file scripts.cpp
 * @brief Implementation of ScriptsDB and ScriptDBEntry. See scripts.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./scripts.h"
#include "./util/gamedata.h"
#include "./mapsdb.h"
#include "./entries/mapdbentry.h"

ScriptDBEntry::ScriptDBEntry() {}
ScriptDBEntry::ScriptDBEntry(QJsonValue& data)
{
  name = data["name"].toString();
  ind  = static_cast<var8>(data["ind"].toDouble());
  size = static_cast<var8>(data["size"].toDouble());

  if (data["skip"].isDouble())
    skip = static_cast<var8>(data["skip"].toDouble());

  if (data["maps"].isArray()) {
    for (const auto& mapEntry : data["maps"].toArray())
      maps.append(mapEntry.toString());
  } else {
    maps.append(name);
  }
}

void ScriptDBEntry::deepLink()
{
  for (const auto& mapName : maps) {
    auto* map = MapsDB::inst()->getIndAt(mapName);
    toMaps.append(map);
#ifdef QT_DEBUG
    if (!map)
      qCritical() << "Script Entry:" << name << "could not deep-link to map" << mapName;
#endif
    if (map)
      map->toScript = this;
  }
}

ScriptsDB* ScriptsDB::inst()
{
  static ScriptsDB* _inst = new ScriptsDB;
  return _inst;
}

const QVector<ScriptDBEntry*> ScriptsDB::getStore() const { return store; }
const QHash<QString, ScriptDBEntry*> ScriptsDB::getInd() const { return ind; }
int ScriptsDB::getStoreSize() const { return store.size(); }

ScriptDBEntry* ScriptsDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

ScriptDBEntry* ScriptsDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void ScriptsDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("scripts");
  for (QJsonValue entry : jsonData.array())
    store.append(new ScriptDBEntry(entry));
  once = true;
}

void ScriptsDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
  once = true;
}

void ScriptsDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    entry->deepLink();
  once = true;
}

void ScriptsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void ScriptsDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<ScriptsDB>("PSE.DB.ScriptsDB", 1, 0, "ScriptsDB", "Can't instantiate in QML");
  once = true;
}

ScriptsDB::ScriptsDB()
{
  qmlRegister();
}
