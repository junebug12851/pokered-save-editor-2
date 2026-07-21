/*
  * Copyright 2019 Fairy Fox
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
 * @file sprites.cpp
 * @brief Implementation of SpritesDB and SpriteDBEntry. See sprites.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./sprites.h"
#include "./util/gamedata.h"

SpriteDBEntry::SpriteDBEntry() {}
SpriteDBEntry::SpriteDBEntry(QJsonValue& data)
{
  name = data["name"].toString();
  ind  = static_cast<var8>(data["ind"].toDouble());

  // A sprite with no group is a townsperson -- so an entry added to the JSON without one
  // still lands somewhere sensible in the Characters bar rather than vanishing.
  group = data["group"].toString();
  if(group.isEmpty())
    group = "Townsfolk";
}

SpritesDB* SpritesDB::inst()
{
  static SpritesDB* _inst = new SpritesDB;
  return _inst;
}

const QVector<SpriteDBEntry*> SpritesDB::getStore() const { return store; }
const QHash<QString, SpriteDBEntry*> SpritesDB::getInd() const { return ind; }
int SpritesDB::getStoreSize() const { return store.size(); }

SpriteDBEntry* SpritesDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

SpriteDBEntry* SpritesDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void SpritesDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("sprites");
  for (QJsonValue entry : jsonData.array())
    store.append(new SpriteDBEntry(entry));
  once = true;
}

void SpritesDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
  once = true;
}

void SpritesDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void SpritesDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<SpritesDB>("PSE.DB.SpritesDB", 1, 0, "SpritesDB", "Can't instantiate in QML");
  once = true;
}

SpritesDB::SpritesDB()
{
  qmlRegister();
}
