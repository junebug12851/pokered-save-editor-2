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

#include <QObject>
#include <QJsonValue>
#include "../db_autoport.h"

class QQmlEngine;
class MapsDB;
class MapDBEntry;
class PokemonDBEntry;

// Wild Pokemon Entry
struct DB_AUTOPORT MapDBEntryWildMon : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT)
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)

public:
  const QString getName() const;
  int getLevel() const;
  const PokemonDBEntry* getToPokemon() const;
  const MapDBEntry* getParent() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntryWildMon();
  MapDBEntryWildMon(const QJsonValue& value, MapDBEntry* const parent);
  void deepLink();
  void qmlRegister() const;

  QString name = "";
  int level = 0;

  PokemonDBEntry* toPokemon = nullptr;
  MapDBEntry* parent = nullptr;

  friend class MapsDB;
  friend class MapDBEntry;
};

#endif // MAPDBENTRYWILDMON_H
