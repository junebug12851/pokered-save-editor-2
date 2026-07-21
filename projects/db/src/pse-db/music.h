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

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntry;
class QQmlEngine;

/**
 * @brief One music track: its name and bank/id, plus the maps that use it.
 *
 * Plain-struct DB entry (public fields). @ref toMaps is a back-reference filled in
 * when MapsDB deep-links. See db.md for the entry convention.
 *
 * @see MusicDB.
 */
struct DB_AUTOPORT MusicDBEntry {
  MusicDBEntry();                ///< Empty entry.
  MusicDBEntry(QJsonValue& data); ///< Build from a JSON value.

  QString name;     ///< Track name (key).
  var8 bank = 0;    ///< Audio bank.
  var8 id   = 0;    ///< Track id within the bank.

  QVector<MapDBEntry*> toMaps; ///< Maps that play this track (back-ref, set by MapsDB).
};

/**
 * @brief The music database -- every track, keyed by name.
 *
 * Standard DB-singleton with a name index (see CreditsDB / db.md).
 *
 * @see MusicDBEntry, DB.
 */
class DB_AUTOPORT MusicDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of tracks.

public:
  static MusicDB* inst(); ///< The process-wide MusicDB singleton.

  [[nodiscard]] const QVector<MusicDBEntry*> getStore() const;       ///< All tracks.
  [[nodiscard]] const QHash<QString, MusicDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                           ///< Track count.

  Q_INVOKABLE MusicDBEntry* getStoreAt(int idx) const;              ///< Track by store index (for QML).
  Q_INVOKABLE MusicDBEntry* getIndAt(const QString& key) const;     ///< Track by name key (for QML).

public slots:
  void load();   ///< Load tracks from JSON.
  void index();  ///< Build the name->entry index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  MusicDB(); ///< Private -- use inst().

  QVector<MusicDBEntry*> store;       ///< The loaded tracks.
  QHash<QString, MusicDBEntry*> ind;  ///< Name->entry lookup.
};
