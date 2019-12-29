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
#include "sprites.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

void Sprites::load()
{
  // Grab Event Pokemon Data
  auto spriteData = GameData::json("sprites");

  // Go through each event Pokemon
  for(QJsonValue spriteEntry : spriteData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new SpriteEntry();

    // Set simple properties
    entry->name = spriteEntry["name"].toString();
    entry->ind = spriteEntry["ind"].toDouble();

    // Add to array
    sprites->append(entry);
  }
}

void Sprites::index()
{
  for(auto entry : *sprites)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);
  }
}

QVector<SpriteEntry*>* Sprites::sprites = new QVector<SpriteEntry*>();
QHash<QString, SpriteEntry*>* Sprites::ind =
    new QHash<QString, SpriteEntry*>();
