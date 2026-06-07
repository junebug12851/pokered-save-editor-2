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
 * @file moves.cpp
 * @brief Implementation of MovesDB and MoveDBEntry. See moves.h for the API.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./moves.h"
#include "./types.h"
#include "./itemsdb.h"
#include "./util/gamedata.h"

MoveDBEntry::MoveDBEntry() {}
MoveDBEntry::MoveDBEntry(QJsonValue& data)
{
  name     = data["name"].toString();
  ind      = static_cast<var8>(data["ind"].toDouble());
  readable = data["readable"].toString();

  if (data["glitch"].isBool())    glitch   = data["glitch"].toBool();
  if (data["type"].isString())    type     = data["type"].toString();
  if (data["power"].isDouble())   power    = static_cast<var8>(data["power"].toDouble());
  if (data["accuracy"].isDouble()) accuracy = static_cast<var8>(data["accuracy"].toDouble());
  if (data["pp"].isDouble())      pp       = static_cast<var8>(data["pp"].toDouble());
  if (data["tm"].isDouble())      tm       = static_cast<var8>(data["tm"].toDouble());
  if (data["hm"].isDouble())      hm       = static_cast<var8>(data["hm"].toDouble());
}

void MoveDBEntry::deepLink()
{
  if (!type.isEmpty())
    toType = TypesDB::inst()->getIndAt(type);

  if (tm && !hm)
    toItem = ItemsDB::inst()->getIndAt("tm" + QString::number(*tm));
  else if (hm)
    toItem = ItemsDB::inst()->getIndAt("hm" + QString::number(*hm));

#ifdef QT_DEBUG
  if (!type.isEmpty() && !toType)
    qCritical() << "Move type:" << type << "could not be deep linked.";
  if ((tm || hm) && !toItem)
    qCritical() << "Move:" << name << "TM/HM item could not be deep linked.";
#endif

  if (toType)
    toType->toMoves.append(this);
}

MovesDB* MovesDB::inst()
{
  static MovesDB* _inst = new MovesDB;
  return _inst;
}

const QVector<MoveDBEntry*> MovesDB::getStore() const { return store; }
const QHash<QString, MoveDBEntry*> MovesDB::getInd() const { return ind; }
int MovesDB::getStoreSize() const { return store.size(); }

MoveDBEntry* MovesDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

MoveDBEntry* MovesDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void MovesDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("moves");
  for (QJsonValue entry : jsonData.array())
    store.append(new MoveDBEntry(entry));
  once = true;
}

void MovesDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);
    if (entry->tm) ind.insert("tm" + QString::number(*entry->tm), entry);
    if (entry->hm) ind.insert("hm" + QString::number(*entry->hm), entry);
  }
  once = true;
}

void MovesDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    entry->deepLink();
  once = true;
}

void MovesDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void MovesDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<MovesDB>("PSE.DB.MovesDB", 1, 0, "MovesDB", "Can't instantiate in QML");
  once = true;
}

MovesDB::MovesDB()
{
  qmlRegister();
}
