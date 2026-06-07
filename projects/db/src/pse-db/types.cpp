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
 * @file types.cpp
 * @brief Implementation of TypesDB and TypeDBEntry. See types.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./types.h"
#include "./util/gamedata.h"

TypeDBEntry::TypeDBEntry() {}
TypeDBEntry::TypeDBEntry(QJsonValue& data)
{
  name     = data["name"].toString();
  ind      = static_cast<var8>(data["ind"].toDouble());
  readable = data["readable"].toString();
}

TypesDB* TypesDB::inst()
{
  static TypesDB* _inst = new TypesDB;
  return _inst;
}

const QVector<TypeDBEntry*> TypesDB::getStore() const { return store; }
const QHash<QString, TypeDBEntry*> TypesDB::getInd() const { return ind; }
int TypesDB::getStoreSize() const { return store.size(); }

TypeDBEntry* TypesDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

TypeDBEntry* TypesDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void TypesDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("types");
  for (QJsonValue entry : jsonData.array())
    store.append(new TypeDBEntry(entry));
  once = true;
}

void TypesDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);
  }
  once = true;
}

void TypesDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TypesDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TypesDB>("PSE.DB.TypesDB", 1, 0, "TypesDB", "Can't instantiate in QML");
  once = true;
}

TypesDB::TypesDB()
{
  qmlRegister();
}
