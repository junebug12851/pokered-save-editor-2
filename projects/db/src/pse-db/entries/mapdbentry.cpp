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

/**
 * @file mapdbentry.cpp
 * @brief Implementation of MapDBEntry (the map definition + its deep-link web).
 *        See mapdbentry.h for the documented API.
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
#include "./mapdbentrytext.h"
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
#include "../trades.h"
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

  if(data["textEntries"].isArray())
  {
    for(QJsonValue textEntry : data["textEntries"].toArray()) {
      auto tmp = new MapDBEntryText(textEntry, this);
      textEntries.append(tmp);
    }
  }

  // The named script steps for the "Current script" picker (plain structs; see MapScriptStep).
  if(data["scriptEntries"].isArray())
  {
    for(QJsonValue s : data["scriptEntries"].toArray()) {
      auto o = s.toObject();
      scriptSteps.append({ o["id"].toInt(), o["name"].toString(), o["label"].toString(),
                           o["desc"].toString() });
    }
  }

  // Where this map's scripts happen, and the flags they write there (plain structs; see
  // MapDBEntryStorageSpot). Absent on the ~200 maps whose scripts never test the player's coords.
  if(data["storageSpots"].isArray())
  {
    // Each write keeps WHO did it, in WHICH PHASE, and WHICH WAY. @see MapDBEntryFlagWrite
    const auto readWrites = [](const QJsonValue& arr) {
      QVector<MapDBEntryFlagWrite> out;
      for(QJsonValue w : arr.toArray()) {
        auto o = w.toObject();
        MapDBEntryFlagWrite fw;
        fw.ind = o["ind"].toInt(-1);
        fw.action = o["action"].toString();
        fw.step = o["step"].toInt(-1);
        fw.stepName = o["stepName"].toString();
        fw.routine = o["routine"].toString();
        fw.viaChain = o["viaChain"].toBool();
        out.append(fw);
      }
      return out;
    };

    for(QJsonValue s : data["storageSpots"].toArray()) {
      auto o = s.toObject();
      MapDBEntryStorageSpot spot;
      spot.kind = o["kind"].toString();
      // -1 (the default) means "this axis is not part of this spot" -- a scriptRow has a y and no
      // x, and that is the honest reading of a whole-row trigger, not a missing value.
      spot.x = o.contains("x") ? o["x"].toInt() : -1;
      spot.y = o.contains("y") ? o["y"].toInt() : -1;
      spot.routine = o["routine"].toString();
      spot.step = o["step"].toInt(-1);
      spot.events = readWrites(o["events"]);
      spot.filters = readWrites(o["filters"]);
      for(QJsonValue c : o["chain"].toArray())
        spot.chain.append(c.toString());
      storageSpots.append(spot);
    }
  }

  // The map's phase-by-phase story. @see MapScriptPhase
  if(data["scriptPhases"].isArray())
  {
    const auto readInds = [](const QJsonValue& arr) {
      QVector<int> out;
      for(QJsonValue v : arr.toArray())
        out.append(v.toInt());
      return out;
    };

    for(QJsonValue s : data["scriptPhases"].toArray()) {
      auto o = s.toObject();
      MapScriptPhase p;
      p.step = o["step"].toInt(-1);
      p.name = o["name"].toString();
      p.routine = o["routine"].toString();
      p.sets = readInds(o["sets"]);
      p.resets = readInds(o["resets"]);
      p.shows = readInds(o["shows"]);
      p.hides = readInds(o["hides"]);
      scriptPhases.append(p);
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
    toMusic = MusicDB::inst()->getIndAt(music);

  if(tileset != "")
    toTileset = TilesetDB::inst()->getIndAt(tileset);

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
        SpriteSetDB::inst()->getIndAt(QString::number(spriteSet));

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

ScriptDBEntry* MapDBEntry::getToScript() const
{
  return toScript;
}

void MapDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  // The text entries are handed to QML by getTextEntriesAt() (the sign picker), so they must be
  // pinned to C++ ownership or the engine will garbage-collect them mid-session. Cascaded here at
  // boot, alongside the map itself.
  for(auto textEntry : textEntries)
    textEntry->qmlProtect(engine);
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

const QVector<HiddenItemDBEntry*> MapDBEntry::getToHiddenCoins() const
{
  return toHiddenCoins;
}

int MapDBEntry::getToHiddenCoinsSize() const
{
  return toHiddenCoins.size();
}

const HiddenItemDBEntry* MapDBEntry::getToHiddenCoinsAt(const int ind) const
{
  if(ind >= toHiddenCoins.size())
    return nullptr;

  return toHiddenCoins.at(ind);
}

const QVector<TradeDBEntry*> MapDBEntry::getToTrades() const
{
  return toTrades;
}

int MapDBEntry::getToTradesSize() const
{
  return toTrades.size();
}

FlyDBEntry* MapDBEntry::getToFlyDestination() const
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

MapDBEntry* MapDBEntry::getToComplete() const
{
    return toComplete;
}

TilesetDBEntry* MapDBEntry::getToTileset() const
{
    return toTileset;
}

MusicDBEntry* MapDBEntry::getToMusic() const
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

SpriteSetDBEntry* MapDBEntry::getToSpriteSet() const
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

const QVector<MapDBEntryText*> MapDBEntry::getTextEntries() const
{
  return textEntries;
}

int MapDBEntry::getTextEntriesSize() const
{
  return textEntries.size();
}

const MapDBEntryText* MapDBEntry::getTextEntriesAt(const int ind) const
{
  if(ind < 0 || ind >= textEntries.size())
    return nullptr;

  return textEntries.at(ind);
}

const QVector<MapScriptStep>& MapDBEntry::getScriptSteps() const
{
  return scriptSteps;
}

const QVector<MapDBEntryStorageSpot>& MapDBEntry::getStorageSpots() const
{
  return storageSpots;
}

const QVector<MapScriptPhase>& MapDBEntry::getScriptPhases() const
{
  return scriptPhases;
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
