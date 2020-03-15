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

#include "./sprites.h"
#include "./util/gamedata.h"

SpriteDBEntry::SpriteDBEntry() {}
SpriteDBEntry::SpriteDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
}

void SpritesDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("sprites");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new SpriteDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void SpritesDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

QVector<SpriteDBEntry*> SpritesDB::store = QVector<SpriteDBEntry*>();
QHash<QString, SpriteDBEntry*> SpritesDB::ind =
    QHash<QString, SpriteDBEntry*>();
