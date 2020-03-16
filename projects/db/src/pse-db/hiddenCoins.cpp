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

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./hiddenCoins.h"
#include "./maps.h"
#include "./util/gamedata.h"

HiddenCoinDBEntry::HiddenCoinDBEntry() {}

HiddenCoinDBEntry::HiddenCoinDBEntry(QJsonValue& data)
{
  // Set simple properties
  map = data["map"].toString();
  x = data["x"].toDouble();
  y = data["y"].toDouble();
}

void HiddenCoinDBEntry::deepLink()
{
  toMap = MapsDB::ind.value(map, nullptr);

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Hidden Coins Map: " << map << ", could not be deep linked." ;
#endif

  if(toMap != nullptr)
    toMap->toHiddenCoins.append(this);
}

void HiddenCoinsDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("hiddenCoins");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new HiddenCoinDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void HiddenCoinsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<HiddenCoinDBEntry*> HiddenCoinsDB::store =
    QVector<HiddenCoinDBEntry*>();