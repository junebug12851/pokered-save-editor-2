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

#include "./trainers.h"
#include "./util/gamedata.h"

TrainerDBEntry::TrainerDBEntry() {}
TrainerDBEntry::TrainerDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();

  // Set simple optional properties
  if(data["unused"].isBool())
    unused = data["unused"].toBool();

  if(data["opp"].isBool())
    opp = data["opp"].toBool();
}

void TrainersDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("trainers");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new TrainerDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void TrainersDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    if(entry->opp)
      ind.insert("Opp" + entry->name, entry);
    else
      ind.insert(entry->name, entry);

    ind.insert(QString::number(entry->ind), entry);
  }
}

QVector<TrainerDBEntry*> TrainersDB::store = QVector<TrainerDBEntry*>();
QHash<QString, TrainerDBEntry*> TrainersDB::ind =
    QHash<QString, TrainerDBEntry*>();
