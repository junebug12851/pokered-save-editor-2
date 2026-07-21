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
#include <QVector>
#include <QHash>

#include "./db_autoport.h"

class MissableDBEntry;
class QQmlEngine;

/**
 * @brief The missables database -- metadata for the missable-sprite flags, keyed by name.
 *
 * The DB-side companion to the save's WorldMissables bitfield: names/describes each
 * missable sprite. Standard DB-singleton with a name index and deepLink(). See
 * CreditsDB / db.md; the entry type is in `entries/missabledbentry.h`.
 *
 * @see MissableDBEntry, WorldMissables (the save-side flags), DB.
 */
class DB_AUTOPORT MissablesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of missable definitions.

public:
  // Get Instance
  static MissablesDB* inst(); ///< The process-wide MissablesDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<MissableDBEntry*> getStore() const;       ///< All missable definitions.
  const QHash<QString, MissableDBEntry*> getInd() const;  ///< Name->entry index.
  int getStoreSize() const;                               ///< Missable count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE MissableDBEntry* getStoreAt(const int ind) const;   ///< Missable by store index (for QML).
  Q_INVOKABLE MissableDBEntry* getIndAt(const QString val) const; ///< Missable by name key (for QML).

public slots:
  void load();     ///< Load missables from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve each missable's cross-DB links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  // Singleton Constructor
  MissablesDB(); ///< Private -- use inst().

  QVector<MissableDBEntry*> store;       ///< The loaded missables.
  QHash<QString, MissableDBEntry*> ind;  ///< Name->entry lookup.
};
