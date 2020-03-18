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

#include <QJsonValue>
#include <QDebug>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./mapdbentrysprite.h"
#include "../sprites.h"
#include "../missables.h"

MapDBEntrySprite::MapDBEntrySprite() {
  qmlRegister();
}

MapDBEntrySprite::MapDBEntrySprite(const QJsonValue& data, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();

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

  if(missable >= 0)
    toMissable = MissablesDB::ind.value(QString::number(missable), nullptr);

#ifdef QT_DEBUG
  if(toSprite == nullptr)
    qCritical() << "MapDBEntrySprite: Unable to deep link " + sprite + " to sprite";

  if(move == "" || text == 0 || (!range && face == ""))
    qCritical() << "Values are not correct on sprite " + sprite;

  if(missable >= 0 && toMissable == nullptr)
    qCritical() << "Missable cannot be deep linked to " + QString::number(missable);
#endif

  if(toSprite != nullptr)
    toSprite->toMaps.append(this);
}

void MapDBEntrySprite::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntrySprite>(
        "PSE.DB.MapDBEntrySprite", 1, 0, "MapDBEntrySprite", "Can't instantiate in QML");
  once = true;
}

const MapDBEntry* MapDBEntrySprite::getParent() const
{
  return parent;
}

void MapDBEntrySprite::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const SpriteDBEntry* MapDBEntrySprite::getToSprite() const
{
    return toSprite;
}

const MissableDBEntry* MapDBEntrySprite::getToMissable() const
{
    return toMissable;
}

int MapDBEntrySprite::getMissable() const
{
    return missable;
}

const QString MapDBEntrySprite::getFace() const
{
    return face;
}

int MapDBEntrySprite::getRange() const
{
    return range;
}

int MapDBEntrySprite::getText() const
{
    return text;
}

const QString MapDBEntrySprite::getMove() const
{
    return move;
}

int MapDBEntrySprite::getY() const
{
    return y;
}

int MapDBEntrySprite::getX() const
{
    return x;
}

const QString MapDBEntrySprite::getSprite() const
{
    return sprite;
}

// Called from child classes
MapDBEntrySprite::SpriteType MapDBEntrySprite::type() const {
#ifdef QT_DEBUG
    qCritical() << "MapDBEntrySprite: Parent asked what child type is";
#endif

  return SpriteType::ERROR;
}

int MapDBEntrySprite::adjustedX() const
{
  return x + 4;
}

int MapDBEntrySprite::adjustedY() const
{
  return y + 4;
}
