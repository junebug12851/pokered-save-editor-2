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

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntry;
class QQmlEngine;

/// The three tileset behaviour categories.
enum class TilesetType { INDOOR = 0, CAVE, OUTDOOR };

constexpr var8 talkCount = 3; ///< Number of "talk-over" tile slots per tileset.

/**
 * @brief One tileset definition: its type, graphics/block/collision pointers, etc.
 *
 * Plain-struct DB entry. Carries the canonical tileset data the save's
 * AreaTileset mirrors (bank/pointers, grass tile, talk-over tiles). @ref typeAsEnum
 * converts the @ref type string to @ref TilesetType. @ref toMaps lists the maps
 * using this tileset (back-ref). See db.md.
 *
 * @see TilesetDB, AreaTileset (the save-side tileset).
 */
struct DB_AUTOPORT TilesetDBEntry {
  TilesetDBEntry();                ///< Empty entry.
  TilesetDBEntry(QJsonValue& data); ///< Build from a JSON value.

  QString name;       ///< Internal tileset name (key).
  QString type;       ///< Behaviour type as a string.
  QString nameAlias;  ///< Friendly name.
  QString typeAlias;  ///< Friendly type label.

  TilesetType typeAsEnum() const; ///< @ref type parsed to the TilesetType enum.

  var8 ind      = 0;          ///< Tileset index.
  var8 talk[3]  = {0, 0, 0};  ///< Talk-over tile ids.
  var8 grass    = 0;          ///< Grass tile id.
  var8 bank     = 0;          ///< Bank holding GFX + blocks.
  var16 blockPtr = 0;         ///< Blocks pointer.
  var16 gfxPtr   = 0;         ///< Graphics pointer.
  var16 collPtr  = 0;         ///< Collision pointer.

  QVector<MapDBEntry*> toMaps; ///< Maps using this tileset (back-ref).
};

/**
 * @brief The tilesets database, keyed by name.
 *
 * Standard DB-singleton with a name index (see CreditsDB / db.md).
 *
 * @see TilesetDBEntry, DB.
 */
class DB_AUTOPORT TilesetDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of tilesets.

public:
  static TilesetDB* inst(); ///< The process-wide TilesetDB singleton.

  [[nodiscard]] const QVector<TilesetDBEntry*> getStore() const;       ///< All tilesets.
  [[nodiscard]] const QHash<QString, TilesetDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                            ///< Tileset count.

  Q_INVOKABLE TilesetDBEntry* getStoreAt(int idx) const;            ///< Tileset by store index (for QML).
  Q_INVOKABLE TilesetDBEntry* getIndAt(const QString& key) const;   ///< Tileset by name key (for QML).

public slots:
  void load();   ///< Load tilesets from JSON.
  void index();  ///< Build the name->entry index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TilesetDB(); ///< Private -- use inst().

  QVector<TilesetDBEntry*> store;       ///< The loaded tilesets.
  QHash<QString, TilesetDBEntry*> ind;  ///< Name->entry lookup.
};
