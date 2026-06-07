/*
  * Copyright 2020 Twilight
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
// With amazing help of Quicktype!!!
// https://app.quicktype.io

#include <QObject>
#include <QVector>

#include "./db_autoport.h"

class QQmlEngine;
struct CreditDBEntry;

/**
 * @brief The credits/attribution database -- and the canonical example of the
 *        "DB singleton" pattern every database in this layer follows.
 *
 * @par The DB-singleton convention (shared by every `*DB` class here)
 * - A `static T* inst()` singleton with a private constructor -- never `new` one.
 * - A `QVector<XxxDBEntry*> store` of entries, loaded from JSON in load() (called
 *   centrally by DB::loadAll(), never from the constructor -- see db.h).
 * - QML-facing access: a `getStore()` / `getStoreSize` pair plus an invokable
 *   `getStoreAt(ind)` (arguments can't be a property or slot, hence Q_INVOKABLE).
 * - qmlProtect()/qmlRegister() for QML ownership + type registration.
 * - The entry struct is a `friend` so it can populate the store during load.
 *
 * Read this one, and the other databases (PokemonDB, MovesDB, ItemsDB, ...) read
 * the same way -- they just carry richer entries and lookup indexes.
 *
 * @see CreditDBEntry (its entry type), DB (the aggregate),
 *      [the db system map](../../../../notes/systems/db.md).
 */
// Singleton accessible, registered to, and protected from QML
class DB_AUTOPORT CreditsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of credit entries.

public:
  /// Returns the process-wide CreditsDB singleton.
  static CreditsDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<CreditDBEntry*> getStore() const; ///< The full entry vector.
  int getStoreSize() const;                        ///< Entry count (backs @c getStoreSize).

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE CreditDBEntry* getStoreAt(const int ind) const; ///< Entry at @p ind (for QML).

public slots:
  // QML accessible methods
  void load();                                       ///< Load entries from the JSON assets.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership (anti-GC).

private slots:
  void qmlRegister() const; ///< Register this DB + its entry with the QML type system.

private:
  // Singleton Constructor
  CreditsDB(); ///< Private -- use inst().

  // Store
  QVector<CreditDBEntry*> store; ///< The loaded credit entries.

  // Allow modifications from these classes
  friend struct CreditDBEntry; ///< Lets entries populate the store during load.
};
