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

#include "./types.h"
#include "./gamedata.h"

void TypesDB::load()
{
  // Grab Event Pokemon Data
  auto typesData = GameData::json("types");

  // Go through each event Pokemon
  for(QJsonValue typesEntry : typesData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TypeDBEntry();

    // Set simple properties
    entry->name = typesEntry["name"].toString();
    entry->ind = typesEntry["ind"].toDouble();
    entry->readable = typesEntry["readable"].toString();

    // Add to array
    store.append(entry);
  }

  delete typesData;
}

void TypesDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);
  }
}

QVector<TypeDBEntry*> TypesDB::store = QVector<TypeDBEntry*>();
QHash<QString, TypeDBEntry*> TypesDB::ind = QHash<QString, TypeDBEntry*>();
