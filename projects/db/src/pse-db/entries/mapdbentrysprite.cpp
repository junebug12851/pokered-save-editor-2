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
#include "mapdbentrysprite.h"

MapDBEntrySprite::MapDBEntrySprite() {}
MapDBEntrySprite::MapDBEntrySprite(QJsonValue& data, MapDBEntry* parent) :
  parent(parent)
{
  sprite = data["sprite"].toString();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  move = data["move"].toString();
  text = data["text"].toDouble();

  if(data["missable"].isDouble())
    missable = data["missable"].toDouble();

  if(data["range"].isDouble())
    range = data["range"].toDouble();
  else
    face = data["face"].toString();
}

void MapDBEntrySprite::deepLink()
{
  toSprite = SpritesDB::ind.value(sprite);

  if(missable)
    toMissable = MissablesDB::ind.value(QString::number(*missable), nullptr);

#ifdef QT_DEBUG
  if(toSprite == nullptr)
    qCritical() << "MapDBEntrySprite: Unable to deep link " + sprite + " to sprite";

  if(move == "" || text == 0 || (!range && face == ""))
    qCritical() << "Values are not correct on sprite " + sprite;

  if(missable && toMissable == nullptr)
    qCritical() << "Missable cannot be deep linked to " + QString::number(*missable);
#endif

  if(toSprite != nullptr)
    toSprite->toMaps.append(this);
}

// Called from child classes
SpriteType MapDBEntrySprite::type() {
#ifdef QT_DEBUG
    qCritical() << "MapDBEntrySprite: Parent asked what child type is";
#endif

  return SpriteType::ERROR;
}

var8 MapDBEntrySprite::adjustedX()
{
  return x + 4;
}

var8 MapDBEntrySprite::adjustedY()
{
  return y + 4;
}
