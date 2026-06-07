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

class MapDBEntry;
class QQmlEngine;
class MapSearch;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

/**
 * @brief The maps database -- every map and its full layout, keyed by name.
 *
 * The largest entry family in the project: each MapDBEntry (in `entries/`) carries
 * a map's size, connections, warps, signs, sprites, tileset, music, and wild
 * encounters. MapsDB is otherwise a standard DB-singleton with a name index and a
 * deepLink() pass, plus a MapSearch finder (like FontsDB) -- @ref search /
 * @ref searchRaw differ only in ownership (C++ smart-pointer vs QML-managed raw).
 *
 * @see MapDBEntry (and the MapDBEntry* family), MapSearch, Area (the save-side map).
 */
class DB_AUTOPORT MapsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)    ///< Number of maps.
  Q_PROPERTY(MapSearch* search READ searchRaw STORED false)  ///< A fresh map finder (QML-managed; see note).

public:
  // Get Instance
  static MapsDB* inst(); ///< The process-wide MapsDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<MapDBEntry*> getStore() const;       ///< All maps.
  const QHash<QString, MapDBEntry*> getInd() const;  ///< Name->map index.
  int getStoreSize() const;                          ///< Map count.

  MapSearch* searchRaw() const; ///< Raw finder for QML (QML-managed ownership).
  QScopedPointer<MapSearch, QScopedPointerDeleteLater> search() const; ///< C++-owned finder (smart pointer).

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE MapDBEntry* getStoreAt(const int ind) const;   ///< Map by store index (for QML).
  Q_INVOKABLE MapDBEntry* getIndAt(const QString val) const; ///< Map by name key (for QML).

public slots:
  void load();     ///< Load maps from JSON.
  void index();    ///< Build the name->map index.
  void deepLink(); ///< Resolve every map's warps/sprites/connections/etc.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  MapsDB(); ///< Private -- use inst().

  QVector<MapDBEntry*> store;       ///< The loaded maps.
  QHash<QString, MapDBEntry*> ind;  ///< Name->map lookup.
};
