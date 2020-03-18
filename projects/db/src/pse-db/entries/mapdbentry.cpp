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
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./mapdbentry.h"
#include "./mapdbentrywarpout.h"
#include "./mapdbentrywarpin.h"
#include "./mapdbentrysign.h"
#include "./mapdbentrysprite.h"
#include "./mapdbentryconnect.h"
#include "./mapdbentrywildmon.h"
#include "./mapdbentryspriteitem.h"
#include "./mapdbentryspritetrainer.h"
#include "./mapdbentryspritenpc.h"
#include "./mapdbentryspritepokemon.h"
#include "../spriteSet.h"
#include "../music.h"
#include "../tileset.h"
#include "./eventdbentry.h"
#include "./flydbentry.h"
#include "./hiddenitemdbentry.h"
#include "../scripts.h"
#include "../mapsdb.h"

MapDBEntry::MapDBEntry()
{
  qmlRegister();
}

MapDBEntry::MapDBEntry(const QJsonValue& data)
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
            MapDBEntryConnect::NORTH,
            new MapDBEntryConnect(MapDBEntryConnect::NORTH, this, tmp));
    }
    if(conVal["east"].isObject()) {
      QJsonValue tmp = conVal["east"].toObject();
      connect.insert(
            MapDBEntryConnect::EAST,
            new MapDBEntryConnect(MapDBEntryConnect::EAST, this, tmp));
    }
    if(conVal["south"].isObject()) {
      QJsonValue tmp = conVal["south"].toObject();
      connect.insert(
            MapDBEntryConnect::SOUTH,
            new MapDBEntryConnect(MapDBEntryConnect::SOUTH, this, tmp));
    }
    if(conVal["west"].isObject()) {
      QJsonValue tmp = conVal["west"].toObject();
      connect.insert(
            MapDBEntryConnect::WEST,
            new MapDBEntryConnect(MapDBEntryConnect::WEST, this, tmp));
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
    toComplete = MapsDB::inst()->getInd().value(incomplete, nullptr);

  if(connect.contains(MapDBEntryConnect::NORTH))
    connect.value(MapDBEntryConnect::NORTH)->deepLink();
  if(connect.contains(MapDBEntryConnect::EAST))
    connect.value(MapDBEntryConnect::EAST)->deepLink();
  if(connect.contains(MapDBEntryConnect::SOUTH))
    connect.value(MapDBEntryConnect::SOUTH)->deepLink();
  if(connect.contains(MapDBEntryConnect::WEST))
    connect.value(MapDBEntryConnect::WEST)->deepLink();

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

  if(spriteSet >= 0)
    toSpriteSet =
        SpriteSetDB::ind.value(QString::number(spriteSet), nullptr);

#ifdef QT_DEBUG
  if(music != "" && toMusic == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to music" << music;

  if(tileset != "" && toTileset == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to tileset" << tileset;

  if(incomplete != "" && toComplete == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to complete" << incomplete;

  if(spriteSet >= 0 && toSpriteSet == nullptr)
    qCritical() << "Map: " << name << ", could not be deep linked to sprite set" << spriteSet;
#endif

  if(toSpriteSet != nullptr)
    toSpriteSet->toMaps.append(this);

  if(toMusic != nullptr)
    toMusic->toMaps.append(this);

  if(toTileset != nullptr)
    toTileset->toMaps.append(this);
}

void MapDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntry>(
        "PSE.DB.MapDBEntry", 1, 0, "MapDBEntry", "Can't instantiate in QML");
  once = true;
}

const ScriptDBEntry* MapDBEntry::getToScript() const
{
  return toScript;
}

void MapDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const QVector<HiddenItemDBEntry*> MapDBEntry::getToHiddenItems() const
{
  return toHiddenItems;
}

int MapDBEntry::getToHiddenItemsSize() const
{
  return toHiddenItems.size();
}

const HiddenItemDBEntry* MapDBEntry::getToHiddenItemsAt(const int ind) const
{
  if(ind >= toHiddenItems.size())
    return nullptr;

  return toHiddenItems.at(ind);
}

const FlyDBEntry* MapDBEntry::getToFlyDestination() const
{
    return toFlyDestination;
}

const QVector<EventDBEntry*> MapDBEntry::getToEvents() const
{
  return toEvents;
}

int MapDBEntry::getToEventsSize() const
{
  return toEvents.size();
}

const EventDBEntry* MapDBEntry::getToEventsAt(const int ind) const
{
  if(ind >= toEvents.size())
    return nullptr;

  return toEvents.at(ind);
}

const MapDBEntry* MapDBEntry::getToComplete() const
{
    return toComplete;
}

const TilesetDBEntry* MapDBEntry::getToTileset() const
{
    return toTileset;
}

const MusicDBEntry* MapDBEntry::getToMusic() const
{
    return toMusic;
}

const QString MapDBEntry::getIncomplete() const
{
    return incomplete;
}

const QString MapDBEntry::getModernName() const
{
    return modernName;
}

const QString MapDBEntry::getTileset() const
{
    return tileset;
}

const QString MapDBEntry::getMusic() const
{
    return music;
}

int MapDBEntry::getHeight() const
{
    return height;
}

int MapDBEntry::getWidth() const
{
    return width;
}

int MapDBEntry::getTextPtr() const
{
    return textPtr;
}

int MapDBEntry::getScriptPtr() const
{
    return scriptPtr;
}

int MapDBEntry::getDataPtr() const
{
    return dataPtr;
}

int MapDBEntry::getBank() const
{
    return bank;
}

int MapDBEntry::getBorder() const
{
    return border;
}

const SpriteSetDBEntry* MapDBEntry::getToSpriteSet() const
{
    return toSpriteSet;
}

int MapDBEntry::getSpriteSet() const
{
    return spriteSet;
}

const QVector<MapDBEntryWildMon*> MapDBEntry::getMonsWater() const
{
  return monsWater;
}

int MapDBEntry::getMonsWaterSize() const
{
  return monsWater.size();
}

const MapDBEntryWildMon* MapDBEntry::getMonsWaterAt(const int ind) const
{
  if(ind >= monsWater.size())
    return nullptr;

  return monsWater.at(ind);
}

const QVector<MapDBEntryWildMon*> MapDBEntry::getMonsBlue() const
{
  return monsBlue;
}

int MapDBEntry::getMonsBlueSize() const
{
  return monsBlue.size();
}

const MapDBEntryWildMon* MapDBEntry::getMonsBlueAt(const int ind) const
{
  if(ind >= monsBlue.size())
    return nullptr;

  return monsBlue.at(ind);
}

const QVector<MapDBEntryWildMon*> MapDBEntry::getMonsRed() const
{
  return monsRed;
}

int MapDBEntry::getMonsRedSize() const
{
  return monsRed.size();
}

const MapDBEntryWildMon* MapDBEntry::getMonsRedAt(const int ind) const
{
  if(ind >= monsRed.size())
    return nullptr;

  return monsRed.at(ind);
}

int MapDBEntry::getMonRateWater() const
{
    return monRateWater;
}

int MapDBEntry::getMonRate() const
{
    return monRate;
}

const QHash<int, MapDBEntryConnect*> MapDBEntry::getConnect() const
{
  return connect;
}

const MapDBEntryConnect* MapDBEntry::getConnectAt(const int val) const
{
  return connect.value(val, nullptr);
}

const QVector<MapDBEntrySprite*> MapDBEntry::getSprites() const
{
  return sprites;
}

int MapDBEntry::getSpritesSize() const
{
  return sprites.size();
}

const MapDBEntrySprite* MapDBEntry::getSpritesAt(const int ind) const
{
  if(ind >= sprites.size())
    return nullptr;

  return sprites.at(ind);
}

const QVector<MapDBEntrySign*> MapDBEntry::getSigns() const
{
  return signs;
}

int MapDBEntry::getSignsSize() const
{
  return signs.size();
}

const MapDBEntrySign* MapDBEntry::getSignsAt(const int ind) const
{
  if(ind >= signs.size())
    return nullptr;

  return signs.at(ind);
}

const QVector<MapDBEntryWarpIn*> MapDBEntry::getWarpIn() const
{
  return warpIn;
}

int MapDBEntry::getWarpInSize() const
{
  return warpIn.size();
}

const MapDBEntryWarpIn* MapDBEntry::getWarpInAt(const int ind) const
{
  if(ind >= warpIn.size())
    return nullptr;

  return warpIn.at(ind);
}

const QVector<MapDBEntryWarpOut*> MapDBEntry::getWarpOut() const
{
  return warpOut;
}

int MapDBEntry::getWarpOutSize() const
{
  return warpOut.size();
}

const MapDBEntryWarpOut* MapDBEntry::getWarpOutAt(const int ind) const
{
  if(ind >= warpOut.size())
    return nullptr;

  return warpOut.at(ind);
}

bool MapDBEntry::getSpecial() const
{
    return special;
}

bool MapDBEntry::getGlitch() const
{
    return glitch;
}

int MapDBEntry::getInd() const
{
    return ind;
}

const QString MapDBEntry::getName() const
{
    return name;
}

const QString MapDBEntry::bestName() const
{
    if(modernName != "")
    return modernName;
  else
    return name;
}

int MapDBEntry::height2X2() const
{
  if(height >= 0)
    return height * 2;

  return -1;
}

int MapDBEntry::width2X2() const
{
  if(width >= 0)
    return width * 2;

  return -1;
}
