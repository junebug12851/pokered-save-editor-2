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
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;

/**
 * @brief One in-game (NPC) trade definition: what you give and get.
 *
 * Plain-struct DB entry. @ref toGive / @ref toGet resolve the species in
 * deepLink(). See db.md for the entry convention.
 *
 * @see TradesDB.
 */
struct DB_AUTOPORT TradeDBEntry {
  TradeDBEntry();                ///< Empty entry.
  TradeDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();              ///< Resolve the give/get species links.

  QString give;        ///< Species name you give (resolved to @ref toGive).
  QString get;         ///< Species name you get (resolved to @ref toGet).
  var8    textId   = 0; ///< Trade dialogue text id.
  QString nickname;    ///< Nickname the received mon comes with.
  bool    unused   = false; ///< Whether this trade slot is unused.

  PokemonDBEntry* toGive = nullptr; ///< Resolved species you give (deepLink).
  PokemonDBEntry* toGet  = nullptr; ///< Resolved species you get (deepLink).
};

/**
 * @brief The in-game trades database.
 *
 * Standard DB-singleton (no key index; trades are accessed by store index). See
 * db.md.
 *
 * @see TradeDBEntry, DB.
 */
class DB_AUTOPORT TradesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of trades.

public:
  static TradesDB* inst(); ///< The process-wide TradesDB singleton.

  [[nodiscard]] const QVector<TradeDBEntry*> getStore() const; ///< All trades.
  [[nodiscard]] int getStoreSize() const;                      ///< Trade count.

  Q_INVOKABLE TradeDBEntry* getStoreAt(int idx) const;         ///< Trade by store index (for QML).

public slots:
  void load();     ///< Load trades from JSON.
  void deepLink(); ///< Resolve each trade's species links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TradesDB(); ///< Private -- use inst().

  QVector<TradeDBEntry*> store; ///< The loaded trades.
};
