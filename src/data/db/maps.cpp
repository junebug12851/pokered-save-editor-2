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

#include <QVector>
#include <QJsonArray>
#include <QJsonObject>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./maps.h"
#include "./tileset.h"
#include "./music.h"
#include "./gamedata.h"

MapDBEntryConnect::MapDBEntryConnect() {}
MapDBEntryConnect::MapDBEntryConnect(
    ConnectDir dir,
    MapDBEntry* fromMap,
    QJsonValue& data)
{
  // Set Direction
  this->dir = dir;

  // Save from map
  this->fromMap = fromMap;

  // Set other values from JSON
  map = data["map"].toString();
  stripMove = data["stripMove"].toDouble();
  stripOffset = data["stripOffset"].toDouble();
  flag = data["flag"].toBool(false);
}

void MapDBEntryConnect::deepLink()
{
  toMap = MapsDB::ind.value(map, nullptr);

#ifdef QT_DEBUG
    if(toMap == nullptr)
      qCritical() << "Map Connect: " << map << ", could not be deep linked to";
#endif
}

var16 MapDBEntryConnect::stripLocation()
{
  // Stop if toMap is not accessible or it doesn't have a data pointer
  // A valid toMap is required
  if(toMap == nullptr || !toMap->dataPtr || !toMap->width || !toMap->height)
    return 0;

  var16 ret = 0;

  var16 dataPtr = *toMap->dataPtr;
  var16 toWidth = *toMap->width;
  var16 toHeight = *toMap->height;

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

var16 MapDBEntryConnect::mapPos()
{
  if(fromMap == nullptr || !fromMap->height || !fromMap->width)
    return 0;

  var16 ret = 0;
  var16 fromHeight = *fromMap->height;
  var16 fromWidth = *fromMap->width;

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

var8 MapDBEntryConnect::stripSize()
{
  if(fromMap == nullptr || toMap == nullptr || !fromMap->width || !toMap->width
     || !fromMap->height || !toMap->height)
    return 0;

  var8 ret = 0;
  var8 fromWidth = *fromMap->width;
  var8 toWidth = *toMap->width;
  var8 fromHeight = *fromMap->height;
  var8 toHeight = *toMap->height;

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

svar8 MapDBEntryConnect::yAlign()
{
  if(toMap == nullptr || !toMap->height)
    return 0;

  svar8 ret;
  var8 toHeight = *toMap->height;

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

svar8 MapDBEntryConnect::xAlign()
{
  if(toMap == nullptr || !toMap->width)
    return 0;

  svar8 ret;
  var8 toWidth = *toMap->width;

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

var16 MapDBEntryConnect::window()
{
  if(toMap == nullptr || !toMap->height || !toMap->width)
    return 0;

  var16 ret;
  var8 toHeight = *toMap->height;
  var8 toWidth = *toMap->width;

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

MapDBEntry::MapDBEntry()
{
  special = false;
  glitch = false;
}

std::optional<var8> MapDBEntry::height2X2()
{
  if(height)
    return *height * 2;

  return std::optional<var8>();
}

std::optional<var8> MapDBEntry::width2X2()
{
  if(width)
    return *width * 2;

  return std::optional<var8>();
}

void MapsDB::load()
{
  // Grab Map Data
  auto mapData = GameData::json("maps");

  // Go through each map
  for(QJsonValue mapEntry : mapData->array())
  {
    // Create a new map entry
    auto entry = new MapDBEntry();

    // Set simple properties
    entry->name = mapEntry["name"].toString();
    entry->ind = mapEntry["ind"].toDouble();

    // Set simple optional properties
    if(mapEntry["special"].isBool())
      entry->special = mapEntry["special"].toBool();

    if(mapEntry["glitch"].isBool())
      entry->glitch = mapEntry["glitch"].toBool();

    if(mapEntry["bank"].isDouble())
      entry->bank = mapEntry["bank"].toDouble();

    if(mapEntry["dataPtr"].isDouble())
      entry->dataPtr = mapEntry["dataPtr"].toDouble();

    if(mapEntry["scriptPtr"].isDouble())
      entry->scriptPtr = mapEntry["scriptPtr"].toDouble();

    if(mapEntry["textPtr"].isDouble())
      entry->textPtr = mapEntry["textPtr"].toDouble();

    if(mapEntry["width"].isDouble())
      entry->width = mapEntry["width"].toDouble();

    if(mapEntry["height"].isDouble())
      entry->height = mapEntry["height"].toDouble();

    if(mapEntry["music"].isString())
      entry->music = mapEntry["music"].toString();

    if(mapEntry["tileset"].isString())
      entry->tileset = mapEntry["tileset"].toString();

    if(mapEntry["modernName"].isString())
      entry->modernName = mapEntry["modernName"].toString();

    if(mapEntry["incomplete"].isString())
      entry->incomplete = mapEntry["incomplete"].toString();

    if(mapEntry["border"].isDouble())
      entry->border = mapEntry["border"].toDouble();

    if(mapEntry["connect"].isObject())
    {
      QJsonValue conVal = mapEntry["connect"].toObject();

      if(conVal["north"].isObject()) {
        QJsonValue tmp = conVal["north"].toObject();
        entry->connect.insert(
              (var8)ConnectDir::NORTH,
              new MapDBEntryConnect(ConnectDir::NORTH, entry, tmp));
      }
      if(conVal["east"].isObject()) {
        QJsonValue tmp = conVal["east"].toObject();
        entry->connect.insert(
              (var8)ConnectDir::EAST,
              new MapDBEntryConnect(ConnectDir::EAST, entry, tmp));
      }
      if(conVal["south"].isObject()) {
        QJsonValue tmp = conVal["south"].toObject();
        entry->connect.insert(
              (var8)ConnectDir::SOUTH,
              new MapDBEntryConnect(ConnectDir::SOUTH, entry, tmp));
      }
      if(conVal["west"].isObject()) {
        QJsonValue tmp = conVal["west"].toObject();
        entry->connect.insert(
              (var8)ConnectDir::WEST,
              new MapDBEntryConnect(ConnectDir::WEST, entry, tmp));
      }
    }

    // Add to array
    store.append(entry);
  }

  delete mapData;
}

void MapsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);

    // Also insert the modern name if present
    if(entry->modernName != "")
      ind.insert(entry->modernName, entry);
  }
}

void MapsDB::deepLink()
{
  for(auto entry : store)
  {
    if(entry->music != "")
      entry->toMusic = MusicDB::ind.value(entry->music, nullptr);

    if(entry->tileset != "")
      entry->toTileset = TilesetDB::ind.value(entry->tileset, nullptr);

    if(entry->incomplete != "")
      entry->toComplete = MapsDB::ind.value(entry->incomplete, nullptr);

    if(entry->connect.contains((var8)ConnectDir::NORTH))
      entry->connect.value((var8)ConnectDir::NORTH)->deepLink();
    if(entry->connect.contains((var8)ConnectDir::EAST))
      entry->connect.value((var8)ConnectDir::EAST)->deepLink();
    if(entry->connect.contains((var8)ConnectDir::SOUTH))
      entry->connect.value((var8)ConnectDir::SOUTH)->deepLink();
    if(entry->connect.contains((var8)ConnectDir::WEST))
      entry->connect.value((var8)ConnectDir::WEST)->deepLink();

#ifdef QT_DEBUG
    if(entry->music != "" && entry->toMusic == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to music" << entry->music;

    if(entry->tileset != "" && entry->toTileset == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to tileset" << entry->tileset;

    if(entry->incomplete != "" && entry->toComplete == nullptr)
      qCritical() << "Map: " << entry->name << ", could not be deep linked to complete" << entry->incomplete;
#endif
  }
}

QVector<MapDBEntry*> MapsDB::store = QVector<MapDBEntry*>();
QHash<QString, MapDBEntry*> MapsDB::ind = QHash<QString, MapDBEntry*>();
