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
#include <QDebug>
#include <pse-common/utility.h>

#include "../maps.h"
#include "hiddenitemdbentry.h"

HiddenItemDBEntry::HiddenItemDBEntry() {
  qmlRegister();
}

HiddenItemDBEntry::HiddenItemDBEntry(const QJsonValue& data)
{
  qmlRegister();

  // Set simple properties
  map = data["map"].toString();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
}

void HiddenItemDBEntry::deepLink()
{
  toMap = MapsDB::ind.value(map, nullptr);

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Hidden Coins Map: " << map << ", could not be deep linked." ;
#endif

  if(toMap != nullptr)
    toMap->toHiddenItems.append(this);
}

void HiddenItemDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<HiddenItemDBEntry>(
        "PSE.DB.HiddenItemDBEntry", 1, 0, "HiddenItemDBEntry", "Can't instantiate in QML");
  once = true;
}

const QString HiddenItemDBEntry::getMap() const
{
    return map;
}

int HiddenItemDBEntry::getX() const
{
    return x;
}

int HiddenItemDBEntry::getY() const
{
    return y;
}

const MapDBEntry* HiddenItemDBEntry::getToMap() const
{
  return toMap;
}

void HiddenItemDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}
