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

#include <QObject>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "db.h"

#include "./util/gamedata.h"
#include "creditsdb.h"

const DB* DB::inst()
{
  static DB* _inst = new DB;
  return _inst;
}

GameData* DB::json()
{
  return GameData::inst();
}

CreditsDB* DB::credits()
{
  return CreditsDB::inst();
}

DB::DB()
{
  // Init Resources
  initRes();

  // Register to QML
  engineRegister();

  // Load, Index, and Deep Link All
  loadAll();
  indexAll();
  deepLinkAll();
}

void DB::initRes() const
{
  Q_INIT_RESOURCE(db);
}

void DB::engineRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<DB>("PSE.DB", 1, 0, "DB", "Can't instantiate in QML");

  registered = true;
}

void DB::loadAll() const
{
  CreditsDB::inst()->load();
}

void DB::indexAll() const
{
  //
}

void DB::deepLinkAll() const
{
  //
}

void DB::engineProtect(const QQmlEngine* const engine) const
{
  Utility::engineProtectUtil(this, engine);
  GameData::inst()->engineProtect(engine);
  CreditsDB::inst()->engineProtect(engine);
}

void DB::engineHook(QQmlContext* const context)
{
  context->setContextProperty("pseDB", this);
}
