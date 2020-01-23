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
#include <QRandomGenerator>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./maps.h"
#include "./tileset.h"
#include "./music.h"
#include "./gamedata.h"
#include "./sprites.h"
#include "./items.h"
#include "./pokemon.h"
#include "./trainers.h"
#include "./spriteSet.h"
#include "./missables.h"
#include "./mapsearch.h"

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
  parent = fromMap;

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

MapDBEntrySpriteNPC::MapDBEntrySpriteNPC() {}
MapDBEntrySpriteNPC::MapDBEntrySpriteNPC(QJsonValue& data, MapDBEntry* parent) :
  MapDBEntrySprite(data, parent)
{}

SpriteType MapDBEntrySpriteNPC::type()
{
  return SpriteType::NPC;
}

MapDBEntrySpriteItem::MapDBEntrySpriteItem() {}
MapDBEntrySpriteItem::MapDBEntrySpriteItem(QJsonValue& data, MapDBEntry* parent) :
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
  if(item == "0")
    return;

  MapDBEntrySprite::deepLink();
  toItem = ItemsDB::ind.value(item);

#ifdef QT_DEBUG
  if(toItem == nullptr)
    qCritical() << "MapDBEntrySpriteItem: Unable to deep link " + item + " to item";
#endif

  if(toItem == nullptr)
    toItem->toMapSpriteItem.append(this);
}

SpriteType MapDBEntrySpriteItem::type()
{
  return SpriteType::ITEM;
}

MapDBEntrySpritePokemon::MapDBEntrySpritePokemon() {}
MapDBEntrySpritePokemon::MapDBEntrySpritePokemon(QJsonValue& data, MapDBEntry* parent) :
  MapDBEntrySprite(data, parent)
{
  pokemon = data["pokemon"].toString();
  level = data["level"].toDouble();
}

void MapDBEntrySpritePokemon::deepLink()
{
  toPokemon = PokemonDB::ind.value(pokemon);

#ifdef QT_DEBUG
  if(toPokemon == nullptr)
    qCritical() << "MapDBEntrySpritePokemon: Unable to deep link " + pokemon + " to pokemon";
#endif

  if(toPokemon != nullptr)
    toPokemon->toMapSpritePokemon = this;
}

SpriteType MapDBEntrySpritePokemon::type()
{
  return SpriteType::POKEMON;
}

MapDBEntrySpriteTrainer::MapDBEntrySpriteTrainer() {}
MapDBEntrySpriteTrainer::MapDBEntrySpriteTrainer(QJsonValue& data, MapDBEntry* parent) :
  MapDBEntrySprite(data, parent)
{
  trainerClass = data["class"].toString();
  team = data["team"].toDouble();
}

void MapDBEntrySpriteTrainer::deepLink()
{
  MapDBEntrySprite::deepLink();
  toTrainer = TrainersDB::ind.value("Opp"+trainerClass);

#ifdef QT_DEBUG
  if(toTrainer == nullptr)
    qCritical() << "MapDBEntrySpriteTrainer: Unable to deep link " + trainerClass + " to trainer";
#endif

  if(toTrainer != nullptr)
    toTrainer->tpMapSpriteTrainers.append(this);
}

SpriteType MapDBEntrySpriteTrainer::type()
{
  return SpriteType::TRAINER;
}

MapDBEntryWarpOut::MapDBEntryWarpOut() {}
MapDBEntryWarpOut::MapDBEntryWarpOut(QJsonValue& data, MapDBEntry* parent) :
  parent(parent)
{
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  warp = data["toWarp"].toDouble();
  map = data["toMap"].toString();
  glitch = data["glitch"].toBool(false);
}

void MapDBEntryWarpOut::deepLink()
{
  toMap = MapsDB::ind.value(map, nullptr);

#ifdef QT_DEBUG
  // Stop here if toMap is nullptr
    if(toMap == nullptr) {
      qCritical() << "Map Warp Out entry: " << map << ", could not be deep linked to map";
      return;
    }
#endif

    // Stop here if this is a special warp to simply return to the last map
    if(map == "Last Map")
      return;

    // Also stop here if this is the silph co elevator which warps to an invalid
    // map. Why would the elevator do this? No idea.
    if(map == "237")
        return;

    // Deep link to the destination warp coordinates
    // This will immidiately crash if toMap isn't set
    toWarp = toMap->warpIn.at(warp);

    toWarp->toConnectingWarps.append(this);
}

MapDBEntryWarpIn::MapDBEntryWarpIn() {}
MapDBEntryWarpIn::MapDBEntryWarpIn(QJsonValue& data, MapDBEntry* parent) :
  parent(parent)
{
  x = data["x"].toDouble();
  y = data["y"].toDouble();
}

MapDBEntrySign::MapDBEntrySign() {}
MapDBEntrySign::MapDBEntrySign(QJsonValue& data, MapDBEntry* parent) :
  parent(parent)
{
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  textID = data["text"].toDouble();
}

MapDBEntryWildMon::MapDBEntryWildMon() {}
MapDBEntryWildMon::MapDBEntryWildMon(QJsonValue& value, MapDBEntry* parent) :
  parent(parent)
{
  name = value["name"].toString();
  level = value["level"].toDouble();
}

void MapDBEntryWildMon::deepLink()
{
  toPokemon = PokemonDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
  // Stop here if toMap is nullptr
    if(toPokemon == nullptr) {
      qCritical() << "Wild Pokemon Entry: " << name << ", could not be deep linked";
      return;
    }
#endif

    if(toPokemon != nullptr)
      toPokemon->toWildMonMaps.append(this);
}

MapDBEntry::MapDBEntry()
{
  special = false;
  glitch = false;
}

MapDBEntry::MapDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();

  // Set simple optional properties
  if(data["special"].isBool())
    special = data["special"].toBool();

  if(data["glitch"].isBool())
    glitch = data["glitch"].toBool();

  if(data["bank"].isDouble())
    bank = data["bank"].toDouble();

  if(data["dataPtr"].isDouble())
    dataPtr = data["dataPtr"].toDouble();

  if(data["scriptPtr"].isDouble())
    scriptPtr = data["scriptPtr"].toDouble();

  if(data["textPtr"].isDouble())
    textPtr = data["textPtr"].toDouble();

  if(data["width"].isDouble())
    width = data["width"].toDouble();

  if(data["height"].isDouble())
    height = data["height"].toDouble();

  if(data["music"].isString())
    music = data["music"].toString();

  if(data["tileset"].isString())
    tileset = data["tileset"].toString();

  if(data["modernName"].isString())
    modernName = data["modernName"].toString();

  if(data["incomplete"].isString())
    incomplete = data["incomplete"].toString();

  if(data["border"].isDouble())
    border = data["border"].toDouble();

  if(data["spriteSet"].isDouble())
    spriteSet = data["spriteSet"].toDouble();

  if(data["warpOut"].isArray())
  {
    for(QJsonValue warpEntry : data["warpOut"].toArray()) {
      auto tmp = new MapDBEntryWarpOut(warpEntry, this);
      warpOut.append(tmp);
    }
  }

  if(data["warpIn"].isArray())
  {
    for(QJsonValue warpEntry : data["warpIn"].toArray()) {
      auto tmp = new MapDBEntryWarpIn(warpEntry, this);
      warpIn.append(tmp);
    }
  }

  if(data["signs"].isArray())
  {
    for(QJsonValue signEntry : data["signs"].toArray()) {
      auto tmp = new MapDBEntrySign(signEntry, this);
      signs.append(tmp);
    }
  }

  if(data["redMons"].isArray())
  {
    for(QJsonValue monEntry : data["redMons"].toArray()) {
      auto tmp = new MapDBEntryWildMon(monEntry, this);
      monsRed.append(tmp);
    }
  }

  if(data["blueMons"].isArray())
  {
    for(QJsonValue monEntry : data["blueMons"].toArray()) {
      auto tmp = new MapDBEntryWildMon(monEntry, this);
      monsBlue.append(tmp);
    }
  }

  if(data["waterMons"].isArray())
  {
    for(QJsonValue monEntry : data["waterMons"].toArray()) {
      auto tmp = new MapDBEntryWildMon(monEntry, this);
      monsWater.append(tmp);
    }
  }

  if(data["monRate"].isDouble())
    monRate = data["monRate"].toDouble();

  if(data["monRateWater"].isDouble())
    monRateWater = data["monRateWater"].toDouble();

  if(data["sprites"].isArray())
  {
    for(QJsonValue spriteEntry : data["sprites"].toArray()) {

      MapDBEntrySprite* ret;

      if(spriteEntry["item"].isString()) {
        auto tmp = new MapDBEntrySpriteItem(spriteEntry, this);
        ret = tmp;
      }
      else if(spriteEntry["class"].isString()) {
        auto tmp = new MapDBEntrySpriteTrainer(spriteEntry, this);
        ret = tmp;
      }
      else if(spriteEntry["pokemon"].isString()) {
        auto tmp = new MapDBEntrySpritePokemon(spriteEntry, this);
        ret = tmp;
      }
      else {
        auto tmp = new MapDBEntrySpriteNPC(spriteEntry, this);
        ret = tmp;
      }

      sprites.append(ret);
    }
  }

  if(data["connect"].isObject())
  {
    QJsonValue conVal = data["connect"].toObject();

    if(conVal["north"].isObject()) {
      QJsonValue tmp = conVal["north"].toObject();
      connect.insert(
            (var8)ConnectDir::NORTH,
            new MapDBEntryConnect(ConnectDir::NORTH, this, tmp));
    }
    if(conVal["east"].isObject()) {
      QJsonValue tmp = conVal["east"].toObject();
      connect.insert(
            (var8)ConnectDir::EAST,
            new MapDBEntryConnect(ConnectDir::EAST, this, tmp));
    }
    if(conVal["south"].isObject()) {
      QJsonValue tmp = conVal["south"].toObject();
      connect.insert(
            (var8)ConnectDir::SOUTH,
            new MapDBEntryConnect(ConnectDir::SOUTH, this, tmp));
    }
    if(conVal["west"].isObject()) {
      QJsonValue tmp = conVal["west"].toObject();
      connect.insert(
            (var8)ConnectDir::WEST,
            new MapDBEntryConnect(ConnectDir::WEST, this, tmp));
    }
  }
}

void MapDBEntry::deepLink()
{
  if(music != "")
    toMusic = MusicDB::ind.value(music, nullptr);

  if(tileset != "")
    toTileset = TilesetDB::ind.value(tileset, nullptr);

  if(incomplete != "")
    toComplete = MapsDB::ind.value(incomplete, nullptr);

  if(connect.contains((var8)ConnectDir::NORTH))
    connect.value((var8)ConnectDir::NORTH)->deepLink();
  if(connect.contains((var8)ConnectDir::EAST))
    connect.value((var8)ConnectDir::EAST)->deepLink();
  if(connect.contains((var8)ConnectDir::SOUTH))
    connect.value((var8)ConnectDir::SOUTH)->deepLink();
  if(connect.contains((var8)ConnectDir::WEST))
    connect.value((var8)ConnectDir::WEST)->deepLink();

  if(warpOut.size() > 0)
    for(auto warpEntry : warpOut)
      warpEntry->deepLink();

  if(sprites.size() > 0)
    for(auto spriteEntry : sprites)
      spriteEntry->deepLink();

  if(monsRed.size() > 0)
    for(auto monEntry : monsRed)
      monEntry->deepLink();

  if(monsBlue.size() > 0)
    for(auto monEntry : monsBlue)
      monEntry->deepLink();

  if(monsWater.size() > 0)
    for(auto monEntry : monsWater)
      monEntry->deepLink();

  if(spriteSet)
    toSpriteSet =
        SpriteSetDB::ind.value(QString::number(*spriteSet), nullptr);

#ifdef QT_DEBUG
  if(music != "" && toMusic == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to music" << music;

  if(tileset != "" && toTileset == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to tileset" << tileset;

  if(incomplete != "" && toComplete == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to complete" << incomplete;

  if(spriteSet && toSpriteSet == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to sprite set" << *spriteSet;
#endif

  if(toSpriteSet != nullptr)
    toSpriteSet->toMaps.append(this);

  if(toMusic != nullptr)
    toMusic->toMaps.append(this);

  if(toTileset != nullptr)
    toTileset->toMaps.append(this);
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
  auto jsonData = GameData::json("maps");

  // Go through each map
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Create a new map entry
    auto entry = new MapDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  delete jsonData;
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
    entry->deepLink();
  }
}

MapSearch* MapsDB::search()
{
  return new MapSearch();
}

QVector<MapDBEntry*> MapsDB::store = QVector<MapDBEntry*>();
QHash<QString, MapDBEntry*> MapsDB::ind = QHash<QString, MapDBEntry*>();
