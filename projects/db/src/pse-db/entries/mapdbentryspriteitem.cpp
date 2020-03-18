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

#include <QDebug>
#include <QJsonValue>
#include "mapdbentryspriteitem.h"
#include "../itemsdb.h"
#include "./itemdbentry.h"

MapDBEntrySpriteItem::MapDBEntrySpriteItem(const QJsonValue& data,
                                           MapDBEntry* const parent) :
  MapDBEntrySprite(data, parent)
{
  item = data["item"].toString();
}

void MapDBEntrySpriteItem::deepLink()
{
  // There are 2 exceptions in the game where the item name will literally be
  // "0". Daisy walking, and Town Map in Rival's house. My only guess is that
  // they wanted to script an item rather than automate giving you an item but
  // it's quite weird because Daisy Walking is an item. Another guess is this
  // is an early in-development code before item automation was programmed in.
  MapDBEntrySprite::deepLink();

  if(item == "0")
    return;

  toItem = ItemsDB::inst()->getInd().value(item);

#ifdef QT_DEBUG
  if(toItem == nullptr)
    qCritical() << "MapDBEntrySpriteItem: Unable to deep link " + item + " to item";
#endif

  if(toItem != nullptr)
    toItem->toMapSpriteItem.append(this);
}

const ItemDBEntry* MapDBEntrySpriteItem::getToItem() const
{
    return toItem;
}

const QString MapDBEntrySpriteItem::getItem() const
{
    return item;
}

MapDBEntrySpriteItem::SpriteType MapDBEntrySpriteItem::type() const
{
    return SpriteType::ITEM;
}
