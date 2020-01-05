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
#include "./gamedata.h"

TrainerDBEntry::TrainerDBEntry()
{
  unused = false;
  opp = false;
}

void TrainersDB::load()
{
  // Grab Event Pokemon Data
  auto trainerData = GameData::json("trainers");

  // Go through each event Pokemon
  for(QJsonValue trainerEntry : trainerData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new TrainerDBEntry();

    // Set simple properties
    entry->name = trainerEntry["name"].toString();
    entry->ind = trainerEntry["ind"].toDouble();

    // Set simple optional properties
    if(trainerEntry["unused"].isBool())
      entry->unused = trainerEntry["unused"].toBool();

    if(trainerEntry["opp"].isBool())
      entry->opp = trainerEntry["opp"].toBool();

    // Add to array
    store.append(entry);
  }

  delete trainerData;
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
