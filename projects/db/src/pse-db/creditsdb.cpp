/*
  * Copyright 2020 June Hanabi
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

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValueRef>
#include <pse-common/utility.h>

#include "./creditsdb.h"
#include "./entries/creditdbentry.h"
#include "./util/gamedata.h"
#include "./db.h"

CreditsDB* CreditsDB::inst()
{
  static CreditsDB* _inst = new CreditsDB;
  return _inst;
}

const QVector<CreditDBEntry*> CreditsDB::getStore() const
{
  return store;
}

int CreditsDB::getStoreSize() const
{
  return store.size();
}

const CreditDBEntry* CreditsDB::getStoreAt(const int ind) const
{
  if(store.size() >= ind)
    return nullptr;

  return store.at(ind);
}

void CreditsDB::load()
{
  static bool loaded = false;

  if(loaded)
    return;

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("credits");
  auto obj = jsonData.object();

  // Create a entry
  CreditDBEntry::process(obj);

  // Seal off further loading
  loaded = true;
}

void CreditsDB::engineProtect(const QQmlEngine* const engine) const
{
  Utility::engineProtectUtil(this, engine);

  for(auto el : store)
    Utility::engineProtectUtil(el, engine);
}

void CreditsDB::engineRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<CreditsDB>("PSE.DB.CreditsDB", 1, 0, "CreditsDB", "Can't instantiate in QML");
  registered = true;
}

CreditsDB::CreditsDB() {
  engineRegister();
  load();
}
