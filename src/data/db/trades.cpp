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

#include "./trades.h"
#include "./gamedata.h"
#include "./pokemon.h"

TradeDBEntry::TradeDBEntry() {}
TradeDBEntry::TradeDBEntry(QJsonValue& data)
{
  // Set simple properties
  give = data["give"].toString();
  get = data["get"].toString();
  textId = data["textId"].toDouble();
  nickname = data["nickname"].toString();

  // Set simple optional properties
  if(data["unused"].isBool())
     unused = data["unused"].toBool();
}

void TradeDBEntry::deepLink()
{
  toGive = PokemonDB::ind.value(give, nullptr);
  toGet = PokemonDB::ind.value(get, nullptr);

#ifdef QT_DEBUG
  if(toGive == nullptr)
    qCritical() << "Trade Give: " << toGive << ", could not be deep linked." ;
  if(toGet == nullptr)
    qCritical() << "Trade Get: " << toGet << ", could not be deep linked." ;
#endif

  if(toGive != nullptr)
    toGive->toTrades.append(this);

  if(toGet != nullptr)
    toGet->toTrades.append(this);
}

void TradesDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("trades");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TradeDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  delete jsonData;
}

void TradesDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<TradeDBEntry*> TradesDB::store = QVector<TradeDBEntry*>();
