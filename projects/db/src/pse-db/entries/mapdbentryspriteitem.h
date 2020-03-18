/*
  * Copyright 2020 June Hanabi
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
#ifndef MAPDBENTRYSPRITEITEM_H
#define MAPDBENTRYSPRITEITEM_H

#include "./mapdbentrysprite.h"
#include "../db_autoport.h"

class ItemDBEntry;
class MapsDB;

// An item that's obtained
struct DB_AUTOPORT MapDBEntrySpriteItem : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getItem READ getItem CONSTANT)
  Q_PROPERTY(ItemDBEntry* getToItem READ getToItem CONSTANT)

public:
  virtual SpriteType type() const;

  const QString getItem() const;
  const ItemDBEntry* getToItem() const;

protected:
  MapDBEntrySpriteItem(const QJsonValue& data, MapDBEntry* const parent);
  virtual void deepLink();
  virtual void qmlRegister() const;

  // Which Item
  QString item = "";

  ItemDBEntry* toItem = nullptr;
  friend class MapsDB;
  friend class MapDBEntry;
};

#endif // MAPDBENTRYSPRITEITEM_H
