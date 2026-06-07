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
 * @file tileset.cpp
 * @brief Implementation of TilesetDB and TilesetDBEntry. See tileset.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./tileset.h"
#include "./util/gamedata.h"

TilesetDBEntry::TilesetDBEntry() {}
TilesetDBEntry::TilesetDBEntry(QJsonValue& data)
{
  name      = data["name"].toString();
  type      = data["type"].toString();
  nameAlias = data["nameAlias"].toString();
  typeAlias = data["typeAlias"].toString();

  ind      = static_cast<var8>(data["ind"].toDouble());
  grass    = static_cast<var8>(data["grass"].toDouble());
  bank     = static_cast<var8>(data["bank"].toDouble());
  blockPtr = static_cast<var16>(data["blockPtr"].toDouble());
  gfxPtr   = static_cast<var16>(data["gfxPtr"].toDouble());
  collPtr  = static_cast<var16>(data["collPtr"].toDouble());

  QJsonValue talkArr = data["talk"].toArray();
  for (var8 i = 0; i < talkCount; ++i)
    talk[i] = static_cast<var8>(talkArr[i].toDouble());
}

TilesetType TilesetDBEntry::typeAsEnum() const
{
  if (type == "Outdoor") return TilesetType::OUTDOOR;
  if (type == "Cave")    return TilesetType::CAVE;
  return TilesetType::INDOOR;
}

TilesetDB* TilesetDB::inst()
{
  static TilesetDB* _inst = new TilesetDB;
  return _inst;
}

const QVector<TilesetDBEntry*> TilesetDB::getStore() const { return store; }
const QHash<QString, TilesetDBEntry*> TilesetDB::getInd() const { return ind; }
int TilesetDB::getStoreSize() const { return store.size(); }

TilesetDBEntry* TilesetDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

TilesetDBEntry* TilesetDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void TilesetDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("tileset");
  for (QJsonValue entry : jsonData.array())
    store.append(new TilesetDBEntry(entry));
  once = true;
}

void TilesetDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(entry->nameAlias, entry);
  }
  once = true;
}

void TilesetDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TilesetDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TilesetDB>("PSE.DB.TilesetDB", 1, 0, "TilesetDB", "Can't instantiate in QML");
  once = true;
}

TilesetDB::TilesetDB()
{
  qmlRegister();
}
