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
#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct TypeDBEntry;
struct ItemDBEntry;
struct PokemonDBEntryMove;
struct PokemonDBEntry;
class QQmlEngine;

/**
 * @brief One move's static data (type, power, accuracy, PP, TM/HM), with links.
 *
 * A "plain struct" DB entry (the other entry style -- contrast the
 * QObject-getter style of CreditDBEntry): fields are public and read directly.
 * The `std::optional` fields are absent for moves that don't have them. The `to*`
 * pointers are resolved in deepLink() once all DBs are loaded -- @ref toType /
 * @ref toItem forward, and the `toPokemon*` vectors are back-references filled in
 * during PokemonDB's deep-link.
 *
 * @see MovesDB (its database), db.md for the entry/deepLink convention.
 */
struct DB_AUTOPORT MoveDBEntry {
  MoveDBEntry();                ///< Empty entry.
  MoveDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();              ///< Resolve cross-DB links (type, item) after load.

  QString name;          ///< Internal move name (key).
  var8 ind = 0;          ///< Move index/id.
  bool glitch = false;   ///< Whether this is a glitch move.
  QString type;          ///< Type name (resolved to @ref toType).
  QString readable;      ///< Human-readable display name.

  std::optional<var8> power;    ///< Base power, if any.
  std::optional<var8> accuracy; ///< Accuracy, if any.
  std::optional<var8> pp;       ///< Base PP, if any.
  std::optional<var8> tm;       ///< TM number teaching this move, if any.
  std::optional<var8> hm;       ///< HM number teaching this move, if any.

  TypeDBEntry* toType  = nullptr; ///< Resolved type entry (deepLink).
  ItemDBEntry* toItem  = nullptr; ///< Resolved TM/HM item entry (deepLink).

  // Back-references populated during PokemonDB deep-link
  QVector<struct PokemonDBEntryMove*> toPokemonLearned;  ///< Mons that learn this by level-up.
  QVector<struct PokemonDBEntry*>     toPokemonInitial;  ///< Mons that start with this move.
  QVector<struct PokemonDBEntry*>     toPokemonTmHm;      ///< Mons that can learn it via TM/HM.
};

/**
 * @brief The moves database -- every move, keyed by name.
 *
 * Standard DB-singleton (see CreditsDB) plus a key index: @ref index() builds a
 * name->entry hash so `getIndAt("tackle")` works, and @ref deepLink() resolves
 * each move's cross-references. See db.md.
 *
 * @see MoveDBEntry, DB.
 */
class DB_AUTOPORT MovesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of moves.

public:
  static MovesDB* inst(); ///< The process-wide MovesDB singleton.

  [[nodiscard]] const QVector<MoveDBEntry*> getStore() const;       ///< All moves, in load order.
  [[nodiscard]] const QHash<QString, MoveDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                          ///< Move count.

  Q_INVOKABLE MoveDBEntry* getStoreAt(int idx) const;              ///< Move by store index (for QML).
  Q_INVOKABLE MoveDBEntry* getIndAt(const QString& key) const;     ///< Move by name key (for QML).

public slots:
  void load();       ///< Load moves from JSON.
  void index();      ///< Build the name->entry index.
  void deepLink();   ///< Resolve every move's cross-DB links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  MovesDB(); ///< Private -- use inst().

  QVector<MoveDBEntry*> store;       ///< The loaded moves.
  QHash<QString, MoveDBEntry*> ind;  ///< Name->entry lookup.
};
