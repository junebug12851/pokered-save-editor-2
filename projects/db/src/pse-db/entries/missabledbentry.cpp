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
#include "missabledbentry.h"

#include <QVector>
#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "../mapsdb.h"
#include "./mapdbentry.h"

MissableDBEntry::MissableDBEntry() {
  qmlRegister();
}
MissableDBEntry::MissableDBEntry(QJsonValue& data)
{
  qmlRegister();

  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  map = data["map"].toString();
  sprite = data["sprite"].toDouble();
  defShow = data["defVal"].toString() == "Show";
}

void MissableDBEntry::deepLink()
{
  toMap = MapsDB::inst()->getInd().value(map, nullptr);

  // Deep link map sprite only if toMap is valid and sprite is a valid range
  if(toMap != nullptr && sprite < toMap->getSprites().size())
  {
    toMapSprite = toMap->getSprites().at(sprite);
  }

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Missables: " << name << ", could not be deep linked to map" << map;

  if(toMapSprite == nullptr &&

     // Don't warn about these errors as they're not my errors, they're
     // gen 1 errors

     // This is a valid map that refers to an extra sprite not on the map
     ((map == "Silph Co 7F" &&
     sprite != 11) ||

     // This is an invalid map with no sprites
     (map == "Unused Map F4" &&
     sprite != 1))

     )
    qCritical() << "Missables: " << name << ", could not be deep linked to map " << map << " sprite #" << sprite;
#endif
}

void MissableDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MissableDBEntry>(
        "PSE.DB.MissableDBEntry", 1, 0, "MissableDBEntry", "Can't instantiate in QML");
  once = true;
}

const MapDBEntrySprite* MissableDBEntry::getToMapSprite() const
{
  return toMapSprite;
}

void MissableDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

const MapDBEntry* MissableDBEntry::getToMap() const
{
    return toMap;
}

bool MissableDBEntry::getDefShow() const
{
    return defShow;
}

int MissableDBEntry::getSprite() const
{
    return sprite;
}

const QString MissableDBEntry::getMap() const
{
    return map;
}

int MissableDBEntry::getInd() const
{
    return ind;
}

const QString MissableDBEntry::getName() const
{
    return name;
}
