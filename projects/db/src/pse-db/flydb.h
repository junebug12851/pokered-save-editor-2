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
#include <QHash>
#include <QJsonValue>
#include <QVector>

#include "./db_autoport.h"

class FlyDBEntry;
class QQmlEngine;

/**
 * @brief The fly-destinations database -- where Fly can take you, keyed by name.
 *
 * Standard DB-singleton with a name index and a deepLink() pass (resolving each
 * destination to its map). See CreditsDB / db.md for the shared pattern; the entry
 * type lives in `entries/flydbentry.h`.
 *
 * @see FlyDBEntry, DB.
 */
class DB_AUTOPORT FlyDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of fly destinations.

public:
  // Get Instance
  static FlyDB* inst(); ///< The process-wide FlyDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<FlyDBEntry*> getStore() const;       ///< All fly destinations.
  const QHash<QString, FlyDBEntry*> getInd() const;  ///< Name->entry index.
  int getStoreSize() const;                          ///< Destination count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE FlyDBEntry* getStoreAt(const int ind) const;   ///< Destination by store index (for QML).
  Q_INVOKABLE FlyDBEntry* getIndAt(const QString val) const; ///< Destination by name key (for QML).

public slots:
  void load();     ///< Load destinations from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve each destination's map link.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

public:
  FlyDB(); ///< (Public here, but obtain the singleton via inst().)

  QVector<FlyDBEntry*> store;       ///< The loaded destinations.
  QHash<QString, FlyDBEntry*> ind;  ///< Name->entry lookup.
};
