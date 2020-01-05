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

#include "./missables.h"
#include "./gamedata.h"

void MissablesDB::load()
{
  // Grab Event Pokemon Data
  auto missableData = GameData::json("missables");

  // Go through each event Pokemon
  for(QJsonValue missableEntry : missableData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new MissableDBEntry();

    // Set simple properties
    entry->name = missableEntry["name"].toString();
    entry->ind = missableEntry["ind"].toDouble();

    // Add to array
    store.append(entry);
  }

  delete missableData;
}

void MissablesDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

QVector<MissableDBEntry*> MissablesDB::store = QVector<MissableDBEntry*>();
QHash<QString, MissableDBEntry*> MissablesDB::ind =
    QHash<QString, MissableDBEntry*>();
