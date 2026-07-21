/*
  * Copyright 2020 Fairy Fox
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
#pragma once
#include <QObject>
#include <QJsonValue>
#include "../db_autoport.h"

class QQmlEngine;
class MapsDB;
class MapDBEntry;
class PokemonDBEntry;

// Wild Pokemon Entry
/**
 * @brief One wild-encounter slot in a map's encounter table: species + level.
 *
 * The DB counterpart to the save's AreaPokemonWild. deepLink() resolves
 * @ref toPokemon. A leaf of MapDBEntry (in its red/blue/water tables). See db.md.
 *
 * @see MapDBEntry (parent), AreaPokemonWild (the save-side wild slot).
 */
struct DB_AUTOPORT MapDBEntryWildMon : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)         ///< Species name.
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)           ///< Encounter level.
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT) ///< Resolved species.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT) ///< Owning map.

public:
  const QString getName() const;   ///< @see getName property.
  int getLevel() const;            ///< @see getLevel property.
  PokemonDBEntry* getToPokemon() const; ///< @see getToPokemon property.
  MapDBEntry* getParent() const;   ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntryWildMon(); ///< Empty entry.
  MapDBEntryWildMon(const QJsonValue& value, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  void deepLink();          ///< Resolve the species link.
  void qmlRegister() const; ///< Register with QML.

  QString name = ""; ///< Species name.
  int level = 0;     ///< Encounter level.

  PokemonDBEntry* toPokemon = nullptr; ///< Resolved species (deepLink).
  MapDBEntry* parent = nullptr;        ///< Owning map.

  friend class MapsDB;
  friend class MapDBEntry;
};
