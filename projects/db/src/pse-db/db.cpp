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
#include "./eventpokemondb.h"
#include "./eventsdb.h"
#include "./examples.h"
#include "./names.h"
#include "./flydb.h"

DB* DB::inst()
{
  static DB* _inst = new DB;
  return _inst;
}

GameData* DB::json() const
{
  return GameData::inst();
}

CreditsDB* DB::credits() const
{
  return CreditsDB::inst();
}

EventPokemonDB* DB::eventPokemon() const
{
  return EventPokemonDB::inst();
}

EventsDB* DB::events() const
{
  return EventsDB::inst();
}

Examples* DB::examples() const
{
  return Examples::inst();
}

Names* DB::names() const
{
  return Names::inst();
}

FlyDB* DB::fly() const
{
  return FlyDB::inst();
}

DB::DB()
{
  // Init Resources
  initRes();

  // Register to QML
  qmlRegister();

  // Load, Index, and Deep Link All
  loadAll();
  indexAll();
  deepLinkAll();
}

void DB::initRes() const
{
  Q_INIT_RESOURCE(db);
}

void DB::qmlRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<DB>("PSE.DB", 1, 0, "DB", "Can't instantiate in QML");

  registered = true;
}

void DB::loadAll() const
{
  Examples::inst();
  Names::inst();

  CreditsDB::inst()->load();
  EventPokemonDB::inst()->load();
  EventsDB::inst()->load();
  FlyDB::inst()->load();
}

void DB::indexAll() const
{
  EventsDB::inst()->index();
  FlyDB::inst()->index();
}

void DB::deepLinkAll() const
{
  EventPokemonDB::inst()->deepLink();
  EventsDB::inst()->deepLink();
  FlyDB::inst()->deepLink();
}

void DB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  GameData::inst()->qmlProtect(engine);
  CreditsDB::inst()->qmlProtect(engine);
  EventPokemonDB::inst()->qmlProtect(engine);
  EventsDB::inst()->qmlProtect(engine);
  Examples::inst()->qmlProtect(engine);
  FlyDB::inst()->qmlProtect(engine);
}

void DB::qmlHook(QQmlContext* const context) const
{
  context->setContextProperty("pseDB", const_cast<DB*>(this));
}
