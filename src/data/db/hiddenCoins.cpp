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
#include "./gamedata.h"

void HiddenCoinsDB::load()
{
  // Grab Event Pokemon Data
  auto hiddenCoinData = GameData::json("hiddenCoins");

  // Go through each event Pokemon
  for(QJsonValue hiddenCoinEntry : hiddenCoinData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new HiddenCoinDBEntry();

    // Set simple properties
    entry->map = hiddenCoinEntry["map"].toString();
    entry->x = hiddenCoinEntry["x"].toDouble();
    entry->y = hiddenCoinEntry["y"].toDouble();

    // Add to array
    store.append(entry);
  }

  delete hiddenCoinData;
}

void HiddenCoinsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->toMap = MapsDB::ind.value(entry->map, nullptr);

#ifdef QT_DEBUG
    if(entry->toMap == nullptr)
      qCritical() << "Hidden Coins Map: " << entry->map << ", could not be deep linked." ;
#endif

    if(entry->toMap != nullptr)
      entry->toMap->toHiddenCoins.append(entry);
  }
}

QVector<HiddenCoinDBEntry*> HiddenCoinsDB::store =
    QVector<HiddenCoinDBEntry*>();
