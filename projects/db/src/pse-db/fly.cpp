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
#include "./util/gamedata.h"
#include "./maps.h"

FlyDBEntry::FlyDBEntry() {}

FlyDBEntry::FlyDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
}

void FlyDBEntry::deepLink()
{
  toMap = MapsDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Fly Destination: " << name << ", could not be deep linked." ;
#endif

  if(toMap != nullptr)
    toMap->toFlyDestination = this;
}

void FlyDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("fly");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new FlyDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
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
    entry->deepLink();
  }
}

QVector<FlyDBEntry*> FlyDB::store = QVector<FlyDBEntry*>();
QHash<QString, FlyDBEntry*> FlyDB::ind = QHash<QString, FlyDBEntry*>();
