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

struct ItemDBEntry;
struct MoveDBEntry;
class QQmlEngine;

/**
 * @brief The TM/HM database -- the ordered list of TM/HM moves and their items.
 *
 * Slightly different shape from the other DBs: the @ref store is just a vector of
 * move-name strings (TM/HM order), and deepLink() resolves two parallel vectors --
 * @ref getTmHmItems (the item for each TM/HM) and @ref getTmHmMoves (the move).
 * No per-entry struct. See db.md for the singleton/deepLink convention.
 *
 * @see ItemsDB, MovesDB.
 */
// All TMs and HMs. Internally HMs are TMs starting at TM 51.
class DB_AUTOPORT TmHmsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of TMs+HMs.

public:
  static TmHmsDB* inst(); ///< The process-wide TmHmsDB singleton.

  [[nodiscard]] const QVector<QString> getStore() const;             ///< Move names in TM/HM order.
  [[nodiscard]] int getStoreSize() const;                           ///< TM+HM count.
  [[nodiscard]] const QVector<ItemDBEntry*>& getTmHmItems() const;  ///< The item for each TM/HM (parallel to store).
  [[nodiscard]] const QVector<MoveDBEntry*>& getTmHmMoves() const;  ///< The move for each TM/HM (parallel to store).

public slots:
  void load();     ///< Load the TM/HM list from JSON.
  void deepLink(); ///< Resolve the parallel item/move vectors.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TmHmsDB(); ///< Private -- use inst().

  QVector<QString>    store;      ///< Move names, TM/HM order (HMs start at TM 51).
  QVector<ItemDBEntry*> toTmHmItem; ///< Resolved item per TM/HM (deepLink).
  QVector<MoveDBEntry*> toTmHmMove; ///< Resolved move per TM/HM (deepLink).
};
