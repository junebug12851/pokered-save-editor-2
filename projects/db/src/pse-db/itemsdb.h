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
#include <QVector>
#include <QHash>

#include "./db_autoport.h"

class ItemDBEntry;
class QQmlEngine;

/**
 * @brief The items database -- every item (with prices), keyed by name.
 *
 * Standard DB-singleton with a name index and a deepLink() pass. See CreditsDB /
 * db.md for the shared pattern; the entry type (with its buy/sell pricing) lives
 * in `entries/itemdbentry.h`.
 *
 * @see ItemDBEntry, DB.
 */
class DB_AUTOPORT ItemsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of items.

public:
  // Get Instance
  static ItemsDB* inst(); ///< The process-wide ItemsDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<ItemDBEntry*> getStore() const;       ///< All items.
  const QHash<QString, ItemDBEntry*> getInd() const;  ///< Name->entry index.
  int getStoreSize() const;                           ///< Item count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE ItemDBEntry* getStoreAt(const int ind) const;   ///< Item by store index (for QML).
  Q_INVOKABLE ItemDBEntry* getIndAt(const QString val) const; ///< Item by name key (for QML).

public slots:
  void load();     ///< Load items from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve each item's cross-DB links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  ItemsDB(); ///< Private -- use inst().

  QVector<ItemDBEntry*> store;       ///< The loaded items.
  QHash<QString, ItemDBEntry*> ind;  ///< Name->entry lookup.
};
