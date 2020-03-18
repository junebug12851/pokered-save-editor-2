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
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "mapdbentrywarpin.h"

MapDBEntryWarpIn::MapDBEntryWarpIn() {
  qmlRegister();
}
MapDBEntryWarpIn::MapDBEntryWarpIn(const QJsonValue& data, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();

  x = data["x"].toDouble();
  y = data["y"].toDouble();
}

void MapDBEntryWarpIn::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntryWarpIn>(
        "PSE.DB.MapDBEntryWarpIn", 1, 0, "MapDBEntryWarpIn", "Can't instantiate in QML");
  once = true;
}

const MapDBEntry* MapDBEntryWarpIn::getParent() const
{
  return parent;
}

void MapDBEntryWarpIn::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const QVector<MapDBEntryWarpOut*> MapDBEntryWarpIn::getToConnectingWarps() const
{
  return toConnectingWarps;
}

int MapDBEntryWarpIn::getToConnectingWarpsSize() const
{
  return toConnectingWarps.size();
}

MapDBEntryWarpOut* MapDBEntryWarpIn::getToConnectingWarpsAt(const int ind) const
{
  if(ind >= toConnectingWarps.size())
    return nullptr;

  return toConnectingWarps.at(ind);
}

int MapDBEntryWarpIn::getY() const
{
    return y;
}

int MapDBEntryWarpIn::getX() const
{
    return x;
}
