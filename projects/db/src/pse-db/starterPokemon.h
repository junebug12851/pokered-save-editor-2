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
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;

/**
 * @brief The curated list of "good starter" species, for the randomizer.
 *
 * A name list (@ref store) resolved to species (@ref toPokemon) in deepLink(). The
 * first three entries are the canonical in-game starters; the rest are extra
 * "startery"-feeling choices. Powers the randomizer's starter picks
 * (PokemonRandom::Random_Starters3 vs Random_Starters). See db.md.
 *
 * @see PokemonDB, PokemonBox::newPokemon().
 */
// Hand-curated list of good starter choices.
// Rules: base evolution (if one exists), non-legendary, feels "startery".
class DB_AUTOPORT StarterPokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of starter choices.

public:
  static StarterPokemonDB* inst(); ///< The process-wide StarterPokemonDB singleton.

  [[nodiscard]] int getStoreSize() const; ///< Starter-choice count.

  // First 3 entries are the canonical in-game starters.
  Q_INVOKABLE PokemonDBEntry* random3Starter() const;   ///< A random one of the 3 canonical starters.
  Q_INVOKABLE PokemonDBEntry* randomAnyStarter() const; ///< A random "startery" species.

public slots:
  void load();     ///< Load the starter list from JSON.
  void deepLink(); ///< Resolve the names to species entries.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  StarterPokemonDB(); ///< Private -- use inst().

  QVector<QString>        store;     ///< Starter species names (first 3 = canonical).
  QVector<PokemonDBEntry*> toPokemon; ///< Resolved species entries (deepLink).
};
