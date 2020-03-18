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

#include <QDebug>
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "mapdbentrywildmon.h"
#include "../pokemon.h"

MapDBEntryWildMon::MapDBEntryWildMon() {
  qmlRegister();
}

MapDBEntryWildMon::MapDBEntryWildMon(const QJsonValue& value, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();

  name = value["name"].toString();
  level = value["level"].toDouble();
}

void MapDBEntryWildMon::deepLink()
{
  toPokemon = PokemonDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
  // Stop here if toMap is nullptr
    if(toPokemon == nullptr) {
      qCritical() << "Wild Pokemon Entry: " << name << ", could not be deep linked";
      return;
    }
#endif

    if(toPokemon != nullptr)
      toPokemon->toWildMonMaps.append(this);
}

void MapDBEntryWildMon::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntryWildMon>(
        "PSE.DB.MapDBEntryWildMon", 1, 0, "MapDBEntryWildMon", "Can't instantiate in QML");
  once = true;
}

const MapDBEntry* MapDBEntryWildMon::getParent() const
{
  return parent;
}

void MapDBEntryWildMon::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const PokemonDBEntry* MapDBEntryWildMon::getToPokemon() const
{
    return toPokemon;
}

int MapDBEntryWildMon::getLevel() const
{
    return level;
}

const QString MapDBEntryWildMon::getName() const
{
    return name;
}
