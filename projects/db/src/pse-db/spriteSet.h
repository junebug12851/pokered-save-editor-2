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
#include <QString>
#include <QVector>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct SpriteDBEntry;
struct MapDBEntry;
class QQmlEngine;

// Outdoor sprites have to be pre-loaded into memory.
// A SpriteSet is a group of up to 11 sprites kept in memory for a given
// outdoor area. Large maps may use two sets split by coordinate.

/**
 * @brief One sprite-set: the pre-loaded sprite group for an outdoor area.
 *
 * Plain-struct DB entry. A set lists up to 11 sprites (@ref spriteList resolved to
 * @ref toSprites). Large maps split into two sets by coordinate -- @ref splitAt
 * plus the @ref toSetWN / @ref toSetES neighbours; @ref isDynamic() reports the
 * split case and getSprites(x,y) returns the right set's sprites for a position.
 * @ref toMaps is a back-ref. See db.md.
 *
 * @see SpriteSetDB, AreaLoadedSprites (the save-side loaded sprites).
 */
struct DB_AUTOPORT SpriteSetDBEntry {
  SpriteSetDBEntry();                ///< Empty entry.
  SpriteSetDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                  ///< Resolve sprite list + split neighbours + maps.

  [[nodiscard]] bool isDynamic() const; ///< True if this set is split by coordinate.
  [[nodiscard]] QVector<SpriteDBEntry*> getSprites(var8 x, var8 y) const; ///< Sprites active at (x,y).

  /**
   * @brief The set the game would actually LOAD at (@p x, @p y) -- never a split set.
   *
   * A split id ($F1-$FC) is not something the console ever stores: `GetSplitMapSpriteSetID`
   * (engine/overworld/map_sprites.asm) resolves it against the dividing line and hands back one of
   * the ten real set ids, and *that* is what goes in `wSpriteSetID`. So a save can only ever hold
   * 1-10 (or 0), and anything else is a value the game did not put there.
   *
   * For a plain set this is `this`. See notes/reference/sprite-sets.md.
   */
  [[nodiscard]] const SpriteSetDBEntry* getResolvedSet(var8 x, var8 y) const;

  var8 ind = 0;   ///< Set index.
  QString split;  ///< Split descriptor.

  QVector<QString>      spriteList; ///< Sprite names in this set.
  QVector<SpriteDBEntry*> toSprites; ///< Resolved sprites (deepLink).

  std::optional<var8> splitAt; ///< Coordinate the set splits at, if dynamic.
  std::optional<var8> setWN;   ///< West/North sub-set index, if split.
  std::optional<var8> setES;   ///< East/South sub-set index, if split.

  SpriteSetDBEntry* toSetWN = nullptr; ///< Resolved West/North set (deepLink).
  SpriteSetDBEntry* toSetES = nullptr; ///< Resolved East/South set (deepLink).

  QVector<MapDBEntry*> toMaps; ///< Maps using this set (back-ref).
};

/**
 * @brief The sprite-sets database, keyed by name.
 *
 * Standard DB-singleton with a name index and a deepLink() pass. See db.md.
 *
 * @see SpriteSetDBEntry, DB.
 */
class DB_AUTOPORT SpriteSetDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of sprite-sets.

public:
  static SpriteSetDB* inst(); ///< The process-wide SpriteSetDB singleton.

  [[nodiscard]] const QVector<SpriteSetDBEntry*> getStore() const;       ///< All sprite-sets.
  [[nodiscard]] const QHash<QString, SpriteSetDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                              ///< Set count.

  Q_INVOKABLE SpriteSetDBEntry* getStoreAt(int idx) const;            ///< Set by store index (for QML).
  Q_INVOKABLE SpriteSetDBEntry* getIndAt(const QString& key) const;   ///< Set by name key (for QML).

public slots:
  void load();     ///< Load sprite-sets from JSON.
  void index();    ///< Build the name->entry index.
  void deepLink(); ///< Resolve sprites, split neighbours, and maps.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  SpriteSetDB(); ///< Private -- use inst().

  QVector<SpriteSetDBEntry*> store;       ///< The loaded sprite-sets.
  QHash<QString, SpriteSetDBEntry*> ind;  ///< Name->entry lookup.
};
