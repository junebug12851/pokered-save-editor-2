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
#include "./util/gamedata.h"

TilesetDBEntry::TilesetDBEntry() {}
TilesetDBEntry::TilesetDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  type = data["type"].toString();
  nameAlias = data["nameAlias"].toString();
  typeAlias = data["typeAlias"].toString();

  ind = data["ind"].toDouble();
  grass = data["grass"].toDouble();

  bank = data["bank"].toDouble();
  blockPtr = data["blockPtr"].toDouble();
  gfxPtr = data["gfxPtr"].toDouble();
  collPtr = data["collPtr"].toDouble();

  QJsonValue talkArr = data["talk"].toArray();
  for(var8 i = 0; i < talkCount; i++)
  {
    talk[i] = talkArr[i].toDouble();
  }
}

TilesetType TilesetDBEntry::typeAsEnum()
{
  if(type == "Outdoor")
    return TilesetType::OUTDOOR;
  else if(type == "Cave")
    return TilesetType::CAVE;

  return TilesetType::INDOOR;
}

void TilesetDB::load()
{
  // Grab Music Data
  auto jsonData = GameData::inst()->json("tileset");

  // Go through each music
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new TilesetDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void TilesetDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(entry->nameAlias, entry);
  }
}

QVector<TilesetDBEntry*> TilesetDB::store = QVector<TilesetDBEntry*>();
QHash<QString, TilesetDBEntry*> TilesetDB::ind =
    QHash<QString, TilesetDBEntry*>();
