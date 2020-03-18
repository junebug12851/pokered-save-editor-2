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
#ifndef MAPDBENTRYWILDMON_H
#define MAPDBENTRYWILDMON_H


// Wild Pokemon Entry
struct DB_AUTOPORT MapDBEntryWildMon
{
  MapDBEntryWildMon();
  MapDBEntryWildMon(QJsonValue& value, MapDBEntry* parent);
  void deepLink();

  QString name;
  var8 level = 0;

  PokemonDBEntry* toPokemon = nullptr;
  MapDBEntry* parent = nullptr;
};

#endif // MAPDBENTRYWILDMON_H
