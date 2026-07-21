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
 * @file tmHm.cpp
 * @brief Implementation of TmHmsDB. See tmHm.h for the documented API.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./tmHm.h"
#include "./util/gamedata.h"
#include "./itemsdb.h"
#include "./moves.h"

TmHmsDB* TmHmsDB::inst()
{
  static TmHmsDB* _inst = new TmHmsDB;
  return _inst;
}

const QVector<QString> TmHmsDB::getStore() const { return store; }
int TmHmsDB::getStoreSize() const { return store.size(); }
const QVector<ItemDBEntry*>& TmHmsDB::getTmHmItems() const { return toTmHmItem; }
const QVector<MoveDBEntry*>& TmHmsDB::getTmHmMoves() const { return toTmHmMove; }

void TmHmsDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("tmHm");
  for (QJsonValue entry : jsonData.array())
    store.append(entry.toString());
  once = true;
}

void TmHmsDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (var8 i = 0; i < static_cast<var8>(store.size()); ++i) {
    const auto& entry = store.at(i);
    var8 tmNum = i + 1;
    auto* item = ItemsDB::inst()->getIndAt("tm" + QString::number(tmNum));
    auto* move = MovesDB::inst()->getIndAt("tm" + QString::number(tmNum));
    toTmHmItem.append(item);
    toTmHmMove.append(move);
#ifdef QT_DEBUG
    if (!item) qCritical() << "TM/HM item:" << entry << "could not be deep linked.";
    if (!move) qCritical() << "TM/HM move:" << entry << "could not be deep linked.";
#endif
  }
  once = true;
}

void TmHmsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TmHmsDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TmHmsDB>("PSE.DB.TmHmsDB", 1, 0, "TmHmsDB", "Can't instantiate in QML");
  once = true;
}

TmHmsDB::TmHmsDB()
{
  qmlRegister();
}
