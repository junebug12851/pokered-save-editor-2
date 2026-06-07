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
#include <QString>
#include <QHash>
#include <QJsonValue>

#include "./db_autoport.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

class EventDBEntry;
class QQmlEngine;

/**
 * @brief The story-events database -- metadata for the 508 event flags, keyed by name.
 *
 * The DB-side companion to the save's WorldEvents bitfield: it names and describes
 * each known event flag. Standard DB-singleton with a name index and deepLink().
 * See CreditsDB / db.md; the entry type is in `entries/eventdbentry.h`.
 *
 * @see EventDBEntry, WorldEvents (the save-side flags), DB.
 */
class DB_AUTOPORT EventsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of event definitions.

public:
  // Get Instance
  static EventsDB* inst(); ///< The process-wide EventsDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<EventDBEntry*> getStore() const;       ///< All event definitions.
  const QHash<QString, EventDBEntry*> getInd() const;  ///< Name->entry index.
  int getStoreSize() const;                            ///< Event count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE EventDBEntry* getStoreAt(const int ind) const;   ///< Event by store index (for QML).
  Q_INVOKABLE EventDBEntry* getIndAt(const QString val) const; ///< Event by name key (for QML).

public slots:
  void load();     ///< Load events from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve each event's cross-DB links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  // Singleton Constructor
  EventsDB(); ///< Private -- use inst().

  QVector<EventDBEntry*> store;       ///< The loaded events.
  QHash<QString, EventDBEntry*> ind;  ///< Name->entry lookup.
};
