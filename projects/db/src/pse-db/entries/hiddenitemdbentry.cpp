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
 * @file hiddenitemdbentry.cpp
 * @brief Implementation of HiddenItemDBEntry. See hiddenitemdbentry.h.
 */

#include <QQmlEngine>
#include <QDebug>
#include <pse-common/utility.h>

#include "../mapsdb.h"
#include "./mapdbentry.h"
#include "hiddenitemdbentry.h"

HiddenItemDBEntry::HiddenItemDBEntry() {
  qmlRegister();
}

HiddenItemDBEntry::HiddenItemDBEntry(const QJsonValue& data, int ind, bool isCoin)
  : ind(ind), isCoin(isCoin)
{
  qmlRegister();

  // Set simple properties
  map = data["map"].toString();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  // Only one of these is present per file -- hiddenItems.json carries "item", hiddenCoins.json
  // carries "coins". The absent one stays at its default, which is the honest answer for it.
  item = data["item"].toString();
  coins = data["coins"].toDouble();
}

void HiddenItemDBEntry::deepLink()
{
  toMap = MapsDB::inst()->getIndAt(map);

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << (isCoin ? "Hidden Coins Map: " : "Hidden Items Map: ")
                << map << ", could not be deep linked.";
#endif

  // Route to the list that matches this entry's save array. Items and coins are separate
  // bitfields with independent numbering, so a shared list would make `ind` ambiguous.
  if(toMap != nullptr)
    (isCoin ? toMap->toHiddenCoins : toMap->toHiddenItems).append(this);
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

int HiddenItemDBEntry::getInd() const
{
    return ind;
}

bool HiddenItemDBEntry::getIsCoin() const
{
    return isCoin;
}

QString HiddenItemDBEntry::getMap() const
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

QString HiddenItemDBEntry::getItem() const
{
    return item;
}

int HiddenItemDBEntry::getCoins() const
{
    return coins;
}

MapDBEntry* HiddenItemDBEntry::getToMap() const
{
  return toMap;
}

void HiddenItemDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}
