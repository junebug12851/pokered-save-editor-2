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

struct MapDBEntrySprite;
class QQmlEngine;

/**
 * @brief One sprite definition: its name/picture-id and the maps that use it.
 *
 * Plain-struct DB entry. @ref toMaps is a back-reference of every map-sprite that
 * uses this picture. See db.md.
 *
 * @see SpritesDB, SpriteData (the save-side sprite), SpriteSetDB.
 */
struct DB_AUTOPORT SpriteDBEntry {
  SpriteDBEntry();                ///< Empty entry.
  SpriteDBEntry(QJsonValue& data); ///< Build from a JSON value.

  QString name;  ///< Sprite name (key).
  var8 ind = 0;  ///< Picture id.

  /// Which shelf of the Characters bar this picture lives on: `Story` (the unique named
  /// cast) / `Trainers` / `Townsfolk` / `Pokemon` / `Objects`. A **curation** -- it groups
  /// the ARTWORK, which is what a person is picking from a rail; it says nothing about what
  /// role an `object_event` gives the sprite. Defaults to `Townsfolk`.
  QString group;

  QVector<MapDBEntrySprite*> toMaps; ///< Map-sprites using this picture (back-ref).
};

/**
 * @brief The sprites database, keyed by name.
 *
 * Standard DB-singleton with a name index (see CreditsDB / db.md).
 *
 * @see SpriteDBEntry, DB.
 */
class DB_AUTOPORT SpritesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of sprites.

public:
  static SpritesDB* inst(); ///< The process-wide SpritesDB singleton.

  [[nodiscard]] const QVector<SpriteDBEntry*> getStore() const;       ///< All sprites.
  [[nodiscard]] const QHash<QString, SpriteDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                            ///< Sprite count.

  Q_INVOKABLE SpriteDBEntry* getStoreAt(int idx) const;              ///< Sprite by store index (for QML).
  Q_INVOKABLE SpriteDBEntry* getIndAt(const QString& key) const;     ///< Sprite by name key (for QML).

public slots:
  void load();   ///< Load sprites from JSON.
  void index();  ///< Build the name->entry index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  SpritesDB(); ///< Private -- use inst().

  QVector<SpriteDBEntry*> store;       ///< The loaded sprites.
  QHash<QString, SpriteDBEntry*> ind;  ///< Name->entry lookup.
};
