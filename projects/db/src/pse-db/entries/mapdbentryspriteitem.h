/*
  * Copyright 2020 Fairy Fox
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
#include "./mapdbentrysprite.h"
#include "../db_autoport.h"

class ItemDBEntry;
class MapsDB;

// An item that's obtained
/**
 * @brief A map sprite that is a pick-up item (type ITEM).
 *
 * Adds the @ref item it gives (resolved to @ref toItem in deepLink) to the common
 * MapDBEntrySprite base. type() returns ITEM. See db.md.
 *
 * @see MapDBEntrySprite (base), ItemDBEntry.
 */
struct DB_AUTOPORT MapDBEntrySpriteItem : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getItem READ getItem CONSTANT)        ///< Item name given.
  Q_PROPERTY(ItemDBEntry* getToItem READ getToItem CONSTANT) ///< Resolved item.

public:
  virtual SpriteType type() const; ///< Returns ITEM.

  const QString getItem() const;   ///< @see getItem property.
  ItemDBEntry* getToItem() const;  ///< @see getToItem property.

protected:
  MapDBEntrySpriteItem(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  virtual void deepLink();          ///< Resolve the item link.
  virtual void qmlRegister() const; ///< Register with QML.

  // Which Item
  QString item = ""; ///< Item name (read via getItem()).

  ItemDBEntry* toItem = nullptr; ///< Resolved item (deepLink).
  friend class MapsDB;
  friend class MapDBEntry;
};
