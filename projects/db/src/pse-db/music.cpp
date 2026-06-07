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
 * @file music.cpp
 * @brief Implementation of MusicDB and MusicDBEntry. See music.h for the API.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./music.h"
#include "./util/gamedata.h"

MusicDBEntry::MusicDBEntry() {}
MusicDBEntry::MusicDBEntry(QJsonValue& data)
{
  name = data["name"].toString();
  bank = static_cast<var8>(data["bank"].toDouble());
  id   = static_cast<var8>(data["id"].toDouble());
}

MusicDB* MusicDB::inst()
{
  static MusicDB* _inst = new MusicDB;
  return _inst;
}

const QVector<MusicDBEntry*> MusicDB::getStore() const { return store; }
const QHash<QString, MusicDBEntry*> MusicDB::getInd() const { return ind; }
int MusicDB::getStoreSize() const { return store.size(); }

MusicDBEntry* MusicDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

MusicDBEntry* MusicDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void MusicDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("music");
  for (QJsonValue entry : jsonData.array())
    store.append(new MusicDBEntry(entry));
  once = true;
}

void MusicDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->bank) + "_" + QString::number(entry->id), entry);
  }
  once = true;
}

void MusicDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void MusicDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<MusicDB>("PSE.DB.MusicDB", 1, 0, "MusicDB", "Can't instantiate in QML");
  once = true;
}

MusicDB::MusicDB()
{
  qmlRegister();
}
