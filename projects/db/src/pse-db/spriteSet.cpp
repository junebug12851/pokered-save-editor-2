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
 * @file spriteSet.cpp
 * @brief Implementation of SpriteSetDB and SpriteSetDBEntry (incl. split sets).
 *        See spriteSet.h for the documented API.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./spriteSet.h"
#include "./util/gamedata.h"
#include "./sprites.h"

SpriteSetDBEntry::SpriteSetDBEntry() {}
SpriteSetDBEntry::SpriteSetDBEntry(QJsonValue& data)
{
  ind = static_cast<var8>(data["ind"].toDouble());

  if (data["split"].isString())
    split = data["split"].toString();
  if (data["splitAt"].isDouble())
    splitAt = static_cast<var8>(data["splitAt"].toDouble());
  if (data["setWN"].isDouble())
    setWN = static_cast<var8>(data["setWN"].toDouble());
  if (data["setES"].isDouble())
    setES = static_cast<var8>(data["setES"].toDouble());

  if (data["sprites"].isArray())
    for (const QJsonValue& s : data["sprites"].toArray())
      spriteList.append(s.toString());
}

void SpriteSetDBEntry::deepLink()
{
  for (const auto& name : spriteList) {
    auto* s = SpritesDB::inst()->getIndAt(name);
    toSprites.append(s);
#ifdef QT_DEBUG
    if (!s) qCritical() << "SpriteSetDB: sprite" << name << "could not be deep linked.";
#endif
  }
  if (setWN) {
    toSetWN = const_cast<SpriteSetDBEntry*>(
      SpriteSetDB::inst()->getIndAt(QString::number(*setWN)));
#ifdef QT_DEBUG
    if (!toSetWN) qCritical() << "SpriteSetDB: setWN" << *setWN << "could not be deep linked.";
#endif
  }
  if (setES) {
    toSetES = const_cast<SpriteSetDBEntry*>(
      SpriteSetDB::inst()->getIndAt(QString::number(*setES)));
#ifdef QT_DEBUG
    if (!toSetES) qCritical() << "SpriteSetDB: setES" << *setES << "could not be deep linked.";
#endif
  }
}

bool SpriteSetDBEntry::isDynamic() const { return ind >= 0xF1; }

QVector<SpriteDBEntry*> SpriteSetDBEntry::getSprites(var8 x, var8 y) const
{
  if (!isDynamic()) return toSprites;
  if (split == "horz")
    return (y < splitAt) ? toSetWN->toSprites : toSetES->toSprites;
  return (x < splitAt) ? toSetWN->toSprites : toSetES->toSprites;
}

SpriteSetDB* SpriteSetDB::inst()
{
  static SpriteSetDB* _inst = new SpriteSetDB;
  return _inst;
}

const QVector<SpriteSetDBEntry*> SpriteSetDB::getStore() const { return store; }
const QHash<QString, SpriteSetDBEntry*> SpriteSetDB::getInd() const { return ind; }
int SpriteSetDB::getStoreSize() const { return store.size(); }

SpriteSetDBEntry* SpriteSetDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

SpriteSetDBEntry* SpriteSetDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void SpriteSetDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("spriteSet");
  for (QJsonValue entry : jsonData.array())
    store.append(new SpriteSetDBEntry(entry));
  once = true;
}

void SpriteSetDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    ind.insert(QString::number(entry->ind), entry);
  once = true;
}

void SpriteSetDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    entry->deepLink();
  once = true;
}

void SpriteSetDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void SpriteSetDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<SpriteSetDB>("PSE.DB.SpriteSetDB", 1, 0, "SpriteSetDB", "Can't instantiate in QML");
  once = true;
}

SpriteSetDB::SpriteSetDB()
{
  qmlRegister();
}
