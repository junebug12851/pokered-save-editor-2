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
#include "trades.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

#include "./pokemon.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

TradeEntry::TradeEntry()
{
  unused = false;

  toGive = nullptr;
  toGet = nullptr;
}

void Trades::load()
{
  // Grab Event Pokemon Data
  auto tradeData = GameData::json("trades");

  // Go through each event Pokemon
  for(QJsonValue tradeEntry : tradeData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TradeEntry();

    // Set simple properties
    entry->give = tradeEntry["give"].toString();
    entry->get = tradeEntry["get"].toString();
    entry->textId = tradeEntry["textId"].toDouble();
    entry->nickname = tradeEntry["nickname"].toString();

    // Set simple optional properties
    if(tradeEntry["unused"].isBool())
       entry->unused = tradeEntry["unused"].toBool();

    // Add to array
    trades->append(entry);
  }
}

void Trades::deepLink()
{
  for(auto entry : *trades)
  {
    entry->toGive = Pokemon::ind->value(entry->give, nullptr);
    entry->toGet = Pokemon::ind->value(entry->get, nullptr);

#ifdef QT_DEBUG
    if(entry->toGive == nullptr)
      qCritical() << "Trade Give: " << entry->toGive << ", could not be deep linked." ;
    if(entry->toGet == nullptr)
      qCritical() << "Trade Get: " << entry->toGet << ", could not be deep linked." ;
#endif
  }
}

QVector<TradeEntry*>* Trades::trades = new QVector<TradeEntry*>();
