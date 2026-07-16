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
#include <QVector>
#include <QString>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntry;
class QQmlEngine;

/**
 * @brief One map-script definition: its id/size and which maps use it.
 *
 * Plain-struct DB entry. @ref maps is the list of map names that share this
 * script; @ref toMaps resolves them in deepLink(). @ref skip is an optional
 * skip-value. See db.md for the entry convention.
 *
 * @see ScriptsDB, WorldScripts (the save-side per-map script progress).
 */
struct DB_AUTOPORT ScriptDBEntry {
  ScriptDBEntry();                ///< Empty entry.
  ScriptDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();              ///< Resolve the @ref maps names to entries.

  QString name;  ///< Script name (key).
  var8 ind  = 0; ///< Script index.
  var8 size = 0; ///< Script size.
  QString desc;  ///< What this map's script progression tracks (import_storage_meta.py).
  int steps = 0; ///< Named step count — the legal dropdown range; beyond it is the jp-hl gun.

  QVector<QString> maps;     ///< Map names using this script.
  std::optional<var8> skip;  ///< Optional skip value.

  QVector<MapDBEntry*> toMaps; ///< Resolved map entries (deepLink).
};

/**
 * @brief The map-scripts database, keyed by name.
 *
 * Standard DB-singleton with a name index and a deepLink() pass. See db.md.
 *
 * @see ScriptDBEntry, DB.
 */
class DB_AUTOPORT ScriptsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of scripts.

public:
  static ScriptsDB* inst(); ///< The process-wide ScriptsDB singleton.

  [[nodiscard]] const QVector<ScriptDBEntry*> getStore() const;       ///< All scripts.
  [[nodiscard]] const QHash<QString, ScriptDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                            ///< Script count.

  Q_INVOKABLE ScriptDBEntry* getStoreAt(int idx) const;              ///< Script by store index (for QML).
  Q_INVOKABLE ScriptDBEntry* getIndAt(const QString& key) const;     ///< Script by name key (for QML).

public slots:
  void load();     ///< Load scripts from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve each script's map links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  ScriptsDB(); ///< Private -- use inst().

  QVector<ScriptDBEntry*> store;       ///< The loaded scripts.
  QHash<QString, ScriptDBEntry*> ind;  ///< Name->entry lookup.
};
