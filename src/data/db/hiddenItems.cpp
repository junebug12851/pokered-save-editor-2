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

#include "./hiddenItems.h"
#include "./maps.h"
#include "./gamedata.h"

void HiddenItemsDB::load()
{
  // Grab Event Pokemon Data
  auto hiddenItemData = GameData::json("hiddenItems");

  // Go through each event Pokemon
  for(QJsonValue hiddenItemEntry : hiddenItemData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new HiddenItemDBEntry();

    // Set simple properties
    entry->map = hiddenItemEntry["map"].toString();
    entry->x = hiddenItemEntry["x"].toDouble();
    entry->y = hiddenItemEntry["y"].toDouble();

    // Add to array
    store.append(entry);
  }

  delete hiddenItemData;
}

void HiddenItemsDB::deepLink()
{
  for(auto entry : store)
  {
    entry->toMap = MapsDB::ind.value(entry->map, nullptr);

#ifdef QT_DEBUG
    if(entry->toMap == nullptr)
      qCritical() << "Hidden Items Map: " << entry->map << ", could not be deep linked." ;
#endif

    if(entry->toMap != nullptr)
      entry->toMap->toHiddenItems.append(entry);
  }
}

QVector<HiddenItemDBEntry*> HiddenItemsDB::store =
    QVector<HiddenItemDBEntry*>();
