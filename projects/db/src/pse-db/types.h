/*
  * Copyright 2019 Twilight
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
#include <QString>
#include <QHash>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MoveDBEntry;
struct PokemonDBEntry;
class QQmlEngine;

/**
 * @brief One elemental type: its name plus the moves and Pokemon of that type.
 *
 * Plain-struct DB entry (public fields). @ref toMoves / @ref toPokemon are
 * back-references filled in when MovesDB / PokemonDB deep-link. See db.md.
 *
 * @see TypesDB.
 */
struct DB_AUTOPORT TypeDBEntry {
  TypeDBEntry();                ///< Empty entry.
  TypeDBEntry(QJsonValue& data); ///< Build from a JSON value.

  QString name;      ///< Internal type name (key).
  var8 ind = 0;      ///< Type index/id.
  QString readable;  ///< Human-readable type name.

  QVector<MoveDBEntry*> toMoves;      ///< Moves of this type (back-ref).
  QVector<PokemonDBEntry*> toPokemon; ///< Pokemon of this type (back-ref).
};

/**
 * @brief The types database -- the type list, keyed by name.
 *
 * Standard DB-singleton with a name index (see CreditsDB / db.md).
 *
 * @see TypeDBEntry, DB.
 */
class DB_AUTOPORT TypesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of types.

public:
  static TypesDB* inst(); ///< The process-wide TypesDB singleton.

  [[nodiscard]] const QVector<TypeDBEntry*> getStore() const;       ///< All types.
  [[nodiscard]] const QHash<QString, TypeDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                          ///< Type count.

  Q_INVOKABLE TypeDBEntry* getStoreAt(int idx) const;              ///< Type by store index (for QML).
  Q_INVOKABLE TypeDBEntry* getIndAt(const QString& key) const;     ///< Type by name key (for QML).

public slots:
  void load();   ///< Load types from JSON.
  void index();  ///< Build the name->entry index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TypesDB(); ///< Private -- use inst().

  QVector<TypeDBEntry*> store;       ///< The loaded types.
  QHash<QString, TypeDBEntry*> ind;  ///< Name->entry lookup.
};
