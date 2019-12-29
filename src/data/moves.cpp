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
#include "moves.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

MoveEntry::MoveEntry()
{
  glitch = false;
}

void Moves::load()
{
  // Grab Event Pokemon Data
  auto moveData = GameData::json("moves");

  // Go through each event Pokemon
  for(QJsonValue moveEntry : moveData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new MoveEntry();

    // Set simple properties
    entry->name = moveEntry["name"].toString();
    entry->ind = moveEntry["ind"].toDouble();

    // Set simple optional properties
    if(moveEntry["glitch"].isBool())
      entry->glitch = moveEntry["glitch"].toBool();

    if(moveEntry["power"].isDouble())
      entry->power = moveEntry["power"].toDouble();

    if(moveEntry["type"].isString())
      entry->type = moveEntry["type"].toString();

    if(moveEntry["accuracy"].isDouble())
      entry->accuracy = moveEntry["accuracy"].toDouble();

    if(moveEntry["pp"].isDouble())
      entry->pp = moveEntry["pp"].toDouble();

    if(moveEntry["tm"].isDouble())
      entry->tm = moveEntry["tm"].toDouble();

    if(moveEntry["hm"].isDouble())
      entry->hm = moveEntry["hm"].toDouble();

    // Add to array
    moves->append(entry);
  }
}

QVector<MoveEntry*>* Moves::moves = new QVector<MoveEntry*>();
