/*
  * Copyright 2019 June Hanabi
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
#ifndef SPRITESET_H
#define SPRITESET_H

#include <QJsonValue>
#include <QString>
#include <QVector>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct SpriteDBEntry;
struct MapDBEntry;

// Outdoor sprites have to be pre-loaded into memory
// A Sprite set is a set of 11 sprites that are kept in memory in a given
// outdoor area. Only they can be used.
// For very large maps you can divide the map into two parts and switch between
// two sprite sets on either side of the map.

struct DB_AUTOPORT SpriteSetDBEntry {

  SpriteSetDBEntry();
  SpriteSetDBEntry(QJsonValue& data);
  void deepLink();

  // A sprite set is dynamic if it's index starts at 241 (0xF1)
  // Dynamic sprites are for large maps that need 2 sprite sets loaded on either
  // side
  bool isDynamic();

  // Get sprite list
  // Returns static list if static
  // Returns correct dynamic list if dynamic
  QVector<SpriteDBEntry*> getSprites(var8 x, var8 y);

  // Sprite Set ID
  var8 ind = 0;

  // For Dynamic Sprites, Split Horizontal or Vertical
  QString split;

  // Static sprites: Static sprite list
  QVector<QString> spriteList;

  // Static sprites: Static sprite list to sprite data
  QVector<SpriteDBEntry*> toSprites;

  // For dynamic sprites, what's the sprite set splitting point on a map
  std::optional<var8> splitAt;

  // Dynamic sprites: Set to load at West or North Division
  std::optional<var8> setWN;

  // Dynamic sprites: To static entry for WN Set
  SpriteSetDBEntry* toSetWN;

  // Dynamic sprites: Set to load at East or South Division
  std::optional<var8> setES;

  // Dynamic sprites: To static entry for ES Set
  SpriteSetDBEntry* toSetES;

  QVector<MapDBEntry*> toMaps;
};

class DB_AUTOPORT SpriteSetDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<SpriteSetDBEntry*> store;
  static QHash<QString, SpriteSetDBEntry*> ind;
};

#endif // SPRITESET_H
