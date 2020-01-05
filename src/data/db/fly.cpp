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

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./fly.h"
#include "./gamedata.h"
#include "./maps.h"

void FlyDB::load()
{
  // Grab Event Pokemon Data
  auto flyData = GameData::json("fly");

  // Go through each event Pokemon
  for(QJsonValue flyEntry : flyData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new FlyDBEntry();

    // Set simple properties
    entry->name = flyEntry["name"].toString();
    entry->ind = flyEntry["ind"].toDouble();

    // Add to array
    store.append(entry);
  }

  delete flyData;
}

void FlyDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

void FlyDB::deepLink()
{
  for(auto entry : store)
  {
    entry->toMap = MapsDB::ind.value(entry->name, nullptr);

#ifdef QT_DEBUG
    if(entry->toMap == nullptr)
      qCritical() << "Fly Destination: " << entry->name << ", could not be deep linked." ;
#endif
  }
}

QVector<FlyDBEntry*> FlyDB::store = QVector<FlyDBEntry*>();
QHash<QString, FlyDBEntry*> FlyDB::ind = QHash<QString, FlyDBEntry*>();
