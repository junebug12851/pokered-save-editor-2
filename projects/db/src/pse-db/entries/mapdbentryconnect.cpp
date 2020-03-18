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
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "./mapdbentryconnect.h"
#include "../mapsdb.h"
#include "./mapdbentry.h"

MapDBEntryConnect::MapDBEntryConnect() {
  qmlRegister();
}
MapDBEntryConnect::MapDBEntryConnect(const ConnectDir dir,
    MapDBEntry* const fromMap,
    const QJsonValue& data)
{
  qmlRegister();
  // Set Direction
  this->dir = dir;

  // Save from map
  this->fromMap = fromMap;
  parent = fromMap;

  // Set other values from JSON
  map = data["map"].toString();
  stripMove = data["stripMove"].toDouble();
  stripOffset = data["stripOffset"].toDouble();
  flag = data["flag"].toBool(false);
}

void MapDBEntryConnect::deepLink()
{
  toMap = MapsDB::inst()->getInd().value(map, nullptr);

#ifdef QT_DEBUG
    if(toMap == nullptr)
      qCritical() << "Map Connect: " << map << ", could not be deep linked to";
#endif
}

void MapDBEntryConnect::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntryConnect>(
        "PSE.DB.MapDBEntryConnect", 1, 0, "MapDBEntryConnect", "Can't instantiate in QML");
  once = true;
}

const MapDBEntry* MapDBEntryConnect::getParent() const
{
  return parent;
}

void MapDBEntryConnect::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const MapDBEntry* MapDBEntryConnect::getFromMap() const
{
    return fromMap;
}

const MapDBEntry* MapDBEntryConnect::getToMap() const
{
    return toMap;
}

bool MapDBEntryConnect::getFlag() const
{
    return flag;
}

int MapDBEntryConnect::getStripOffset() const
{
    return stripOffset;
}

int MapDBEntryConnect::getStripMove() const
{
    return stripMove;
}

MapDBEntryConnect::ConnectDir MapDBEntryConnect::getDir() const
{
  return dir;
}

const QString MapDBEntryConnect::getMap() const
{
    return map;
}

int MapDBEntryConnect::stripLocation() const
{
    // Stop if toMap is not accessible or it doesn't have a data pointer
  // A valid toMap is required
  if(toMap == nullptr ||
     toMap->getDataPtr() <= 0 ||
     toMap->getWidth() <= 0 ||
     toMap->getHeight() <= 0)
    return 0;

  int ret = 0;

  int dataPtr = toMap->getDataPtr();
  int toWidth = toMap->getWidth();
  int toHeight = toMap->getHeight();

  // These can vary based on direction
  if(dir == ConnectDir::NORTH) {
    ret = dataPtr + (toWidth * (toHeight - 3)) + stripOffset;
  }
  else if(dir == ConnectDir::SOUTH) {
    ret = dataPtr + stripOffset;
  }
  else if(dir == ConnectDir::WEST) {
    ret = dataPtr + (toWidth * stripOffset) + toWidth - 3;
  }
  else {
    ret = dataPtr + (toWidth * stripOffset);
  }

  return ret;
}

int MapDBEntryConnect::mapPos() const
{
  if(fromMap == nullptr ||
     fromMap->getHeight() <= 0 ||
     fromMap->getWidth() <= 0)
    return 0;

  int ret = 0;
  int fromHeight = fromMap->getHeight();
  int fromWidth = fromMap->getWidth();

  if(dir == ConnectDir::NORTH) {
    ret = worldMapPtr + 3 + stripMove;
  }
  else if(dir == ConnectDir::SOUTH) {
    ret = worldMapPtr + 3 + (fromHeight + 3) * (fromWidth + 6) + stripMove;
  }
  else if(dir == ConnectDir::WEST) {
    ret = worldMapPtr + (fromWidth + 6) * (stripMove + 3);
  }
  else {
    ret = worldMapPtr - 3 + (fromWidth + 6) * (stripMove + 4);
  }

  return ret;
}

int MapDBEntryConnect::stripSize() const
{
  if(fromMap == nullptr ||
     toMap == nullptr ||
     fromMap->getWidth() <= 0 ||
     toMap->getWidth() <= 0 ||
     fromMap->getHeight() <= 0 ||
     toMap->getHeight() <= 0)
    return 0;

  int ret = 0;
  int fromWidth = fromMap->getWidth();
  int toWidth = toMap->getWidth();
  int fromHeight = fromMap->getHeight();
  int toHeight = toMap->getHeight();

  if(dir == ConnectDir::NORTH) {
    if(fromWidth < toWidth)
      ret = fromWidth - stripMove + 3;
    else
      ret = toWidth - stripOffset;
  }
  else if(dir == ConnectDir::SOUTH) {
    if(fromWidth < toWidth) {
      if(flag)
        ret = fromWidth - stripMove + 3;
      else
        ret = fromWidth - stripMove;
    }
    else {
      ret = toWidth - stripOffset;
    }
  }
  else if(dir == ConnectDir::WEST) {
    if(fromHeight < toHeight)
      ret = fromHeight - stripMove + 3;
    else
      ret = toHeight - stripOffset;
  }
  else {
    if(fromHeight < toHeight) {
      if(flag)
        ret = fromHeight - stripMove + 3;
      else
        ret = fromHeight - stripMove;
    }
    else {
      ret = toHeight - stripOffset;
    }
  }

  return ret;
}

int MapDBEntryConnect::yAlign() const
{
  if(toMap == nullptr ||
     toMap->getHeight() <= 0)
    return 0;

  int ret;
  int toHeight = toMap->getHeight();

  if(dir == ConnectDir::NORTH) {
    ret = (toHeight * 2) - 1;
  }
  else if(dir == ConnectDir::SOUTH) {
    ret = 0;
  }
  else if(dir == ConnectDir::WEST) {
    ret = (stripMove - stripOffset) * -2;
  }
  else {
    ret = (stripMove - stripOffset) * -2;
  }

  return ret;
}

int MapDBEntryConnect::xAlign() const
{
  if(toMap == nullptr ||
     toMap->getWidth())
    return 0;

  int ret;
  int toWidth = toMap->getWidth();

  if(dir == ConnectDir::NORTH) {
    ret = (stripMove - stripOffset) * -2;
  }
  else if(dir == ConnectDir::SOUTH) {
    ret = (stripMove - stripOffset) * -2;
  }
  else if(dir == ConnectDir::WEST) {
    ret = (toWidth * 2) - 1;
  }
  else {
    ret = 0;
  }

  return ret;
}

int MapDBEntryConnect::window() const
{
  if(toMap == nullptr ||
     toMap->getHeight() <= 0 ||
     toMap->getWidth() <= 0)
    return 0;

  int ret;
  int toHeight = toMap->getHeight();
  int toWidth = toMap->getWidth();

  if(dir == ConnectDir::NORTH) {
    ret = worldMapPtr + 1 + (toHeight * (toWidth + 6));
  }
  else if(dir == ConnectDir::SOUTH) {
    ret = worldMapPtr + 7 + toWidth;
  }
  else if(dir == ConnectDir::WEST) {
    ret = worldMapPtr + 6 + (2 * toWidth);
  }
  else {
    ret = worldMapPtr + 7 + toWidth;
  }

  return ret;
}
