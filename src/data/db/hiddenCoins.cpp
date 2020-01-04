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
#include "hiddenCoins.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

void HiddenCoins::load()
{
  // Grab Event Pokemon Data
  auto hiddenCoinData = GameData::json("hiddenCoins");

  // Go through each event Pokemon
  for(QJsonValue hiddenCoinEntry : hiddenCoinData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new HiddenCoinEntry();

    // Set simple properties
    entry->map = hiddenCoinEntry["map"].toString();
    entry->x = hiddenCoinEntry["x"].toDouble();
    entry->y = hiddenCoinEntry["y"].toDouble();

    // Add to array
    hiddenCoins->append(entry);
  }
}

void HiddenCoins::deepLink()
{
  for(auto entry : *hiddenCoins)
  {
    entry->toMap = Maps::ind->value(entry->map, nullptr);

#ifdef QT_DEBUG
    if(entry->toMap == nullptr)
      qCritical() << "Hidden Coins Map: " << entry->map << ", could not be deep linked." ;
#endif
  }
}

QVector<HiddenCoinEntry*>* HiddenCoins::hiddenCoins =
    new QVector<HiddenCoinEntry*>();