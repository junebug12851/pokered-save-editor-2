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
// With amazing help of Quicktype!!!
// https://app.quicktype.io

#include <QObject>
#include <QVector>

#include "./db_autoport.h"

// These are Pokemon you get by going to or participating in real-life events
// that were held around the world

struct EventPokemonDBEntry;
class QQmlEngine;

/**
 * @brief Database of real-world event/distribution Pokemon presets.
 *
 * As the note above says: Pokemon handed out at real-life events around the world.
 * Standard DB-singleton (no key index; accessed by store index) with a deepLink()
 * pass. See CreditsDB / db.md; the entry type is in `entries/eventpokemondbentry.h`.
 *
 * @see EventPokemonDBEntry, DB.
 */
class DB_AUTOPORT EventPokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of event Pokemon.

public:
  // Get Instance
  static EventPokemonDB* inst(); ///< The process-wide EventPokemonDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<EventPokemonDBEntry*> getStore() const; ///< All event Pokemon.
  int getStoreSize() const;                             ///< Event-Pokemon count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE EventPokemonDBEntry* getStoreAt(const int ind) const; ///< Event Pokemon by store index (for QML).

public slots:
  // QML accessible methods
  void load();     ///< Load event Pokemon from JSON.
  void deepLink(); ///< Resolve each entry's species/move links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  // Singleton Constructor
  EventPokemonDB(); ///< Private -- use inst().

  // Store
  QVector<EventPokemonDBEntry*> store; ///< The loaded event Pokemon.
};
