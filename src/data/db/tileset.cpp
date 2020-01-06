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

#include "./tileset.h"
#include "./gamedata.h"

void TilesetDB::load()
{
  // Grab Music Data
  auto tilesetData = GameData::json("tileset");

  // Go through each music
  for(QJsonValue tilesetEntry : tilesetData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TilesetDBEntry();

    // Set simple properties
    entry->name = tilesetEntry["name"].toString();
    entry->type = tilesetEntry["type"].toString();

    // Add to array
    store.append(entry);
  }

  delete tilesetData;
}

void TilesetDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
  }
}

QVector<TilesetDBEntry*> TilesetDB::store = QVector<TilesetDBEntry*>();
QHash<QString, TilesetDBEntry*> TilesetDB::ind =
    QHash<QString, TilesetDBEntry*>();
