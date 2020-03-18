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
#include "mapdbentrywildmon.h"

MapDBEntryWildMon::MapDBEntryWildMon() {}
MapDBEntryWildMon::MapDBEntryWildMon(QJsonValue& value, MapDBEntry* parent) :
  parent(parent)
{
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
