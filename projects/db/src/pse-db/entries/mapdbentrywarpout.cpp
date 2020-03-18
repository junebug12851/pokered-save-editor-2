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
#include "mapdbentrywarpout.h"
#include "./mapdbentry.h"
#include "./mapdbentrywarpin.h"
#include "../mapsdb.h"

MapDBEntryWarpOut::MapDBEntryWarpOut() {
  qmlRegister();
}
MapDBEntryWarpOut::MapDBEntryWarpOut(const QJsonValue& data, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  warp = data["toWarp"].toDouble();
  map = data["toMap"].toString();
  glitch = data["glitch"].toBool(false);
}

void MapDBEntryWarpOut::deepLink()
{
  toMap = MapsDB::inst()->getInd().value(map, nullptr);

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
    toWarp = const_cast<MapDBEntryWarpIn*>(toMap->getWarpInAt(warp));

    toWarp->toConnectingWarps.append(this);
}

void MapDBEntryWarpOut::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntryWarpOut>(
        "PSE.DB.MapDBEntryWarpOut", 1, 0, "MapDBEntryWarpOut", "Can't instantiate in QML");
  once = true;
}

const MapDBEntryWarpIn* MapDBEntryWarpOut::getToWarp() const
{
  return toWarp;
}

void MapDBEntryWarpOut::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const MapDBEntry* MapDBEntryWarpOut::getParent() const
{
    return parent;
}

const MapDBEntry* MapDBEntryWarpOut::getToMap() const
{
    return toMap;
}

bool MapDBEntryWarpOut::getGlitch() const
{
    return glitch;
}

const QString MapDBEntryWarpOut::getMap() const
{
    return map;
}

int MapDBEntryWarpOut::getWarp() const
{
    return warp;
}

int MapDBEntryWarpOut::getY() const
{
    return y;
}

int MapDBEntryWarpOut::getX() const
{
    return x;
}
