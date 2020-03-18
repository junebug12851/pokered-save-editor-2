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

#include <QQmlEngine>
#include <pse-common/utility.h>
#include "mapdbentrysign.h"

MapDBEntrySign::MapDBEntrySign() {
  qmlRegister();
}

const MapDBEntry* MapDBEntrySign::getParent() const
{
  return parent;
}

void MapDBEntrySign::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

int MapDBEntrySign::getTextID() const
{
    return textID;
}

int MapDBEntrySign::getY() const
{
    return y;
}

int MapDBEntrySign::getX() const
{
    return x;
}
MapDBEntrySign::MapDBEntrySign(const QJsonValue& data, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  textID = data["text"].toDouble();
}

void MapDBEntrySign::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntrySign>(
        "PSE.DB.MapDBEntrySign", 1, 0, "MapDBEntrySign", "Can't instantiate in QML");
  once = true;
}
