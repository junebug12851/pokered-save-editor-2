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
 * @file trainers.cpp
 * @brief Implementation of TrainersDB and TrainerDBEntry. See trainers.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./trainers.h"
#include "./util/gamedata.h"

TrainerDBEntry::TrainerDBEntry() {}
TrainerDBEntry::TrainerDBEntry(QJsonValue& data)
{
  name = data["name"].toString();
  ind  = static_cast<var8>(data["ind"].toDouble());
  if (data["unused"].isBool()) unused = data["unused"].toBool();
  if (data["opp"].isBool())    opp    = data["opp"].toBool();
}

TrainersDB* TrainersDB::inst()
{
  static TrainersDB* _inst = new TrainersDB;
  return _inst;
}

const QVector<TrainerDBEntry*> TrainersDB::getStore() const { return store; }
const QHash<QString, TrainerDBEntry*> TrainersDB::getInd() const { return ind; }
int TrainersDB::getStoreSize() const { return store.size(); }

TrainerDBEntry* TrainersDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

TrainerDBEntry* TrainersDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void TrainersDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("trainers");
  for (QJsonValue entry : jsonData.array())
    store.append(new TrainerDBEntry(entry));
  once = true;
}

void TrainersDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->opp ? "Opp" + entry->name : entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
  once = true;
}

void TrainersDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TrainersDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TrainersDB>("PSE.DB.TrainersDB", 1, 0, "TrainersDB", "Can't instantiate in QML");
  once = true;
}

TrainersDB::TrainersDB()
{
  qmlRegister();
}
