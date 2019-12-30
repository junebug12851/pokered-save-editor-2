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
#include "types.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

void Types::load()
{
  // Grab Event Pokemon Data
  auto typesData = GameData::json("types");

  // Go through each event Pokemon
  for(QJsonValue typesEntry : typesData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TypeEntry();

    // Set simple properties
    entry->name = typesEntry["name"].toString();
    entry->ind = typesEntry["ind"].toDouble();
    entry->readable = typesEntry["readable"].toString();

    // Add to array
    types->append(entry);
  }
}

void Types::index()
{
  for(auto entry : *types)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);
    ind->insert(entry->readable, entry);
  }
}

QVector<TypeEntry*>* Types::types = new QVector<TypeEntry*>();
QHash<QString, TypeEntry*>* Types::ind = new QHash<QString, TypeEntry*>();
