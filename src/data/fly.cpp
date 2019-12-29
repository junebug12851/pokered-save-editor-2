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
#include "fly.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

void Fly::load()
{
  // Grab Event Pokemon Data
  auto flyData = GameData::json("fly");

  // Go through each event Pokemon
  for(QJsonValue flyEntry : flyData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new FlyEntry();

    // Set simple properties
    entry->name = flyEntry["name"].toString();
    entry->ind = flyEntry["ind"].toDouble();

    // Add to array
    fly->append(entry);
  }
}

void Fly::index()
{
  for(auto entry : *fly)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);
  }
}

QVector<FlyEntry*>* Fly::fly = new QVector<FlyEntry*>();
QHash<QString, FlyEntry*>* Fly::ind = new QHash<QString, FlyEntry*>();
