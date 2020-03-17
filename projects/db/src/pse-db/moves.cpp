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

#include "./moves.h"
#include "./types.h"
#include "./util/gamedata.h"
#include "./itemsdb.h"

MoveDBEntry::MoveDBEntry() {}
MoveDBEntry::MoveDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  readable = data["readable"].toString();

  // Set simple optional properties
  if(data["glitch"].isBool())
    glitch = data["glitch"].toBool();

  if(data["power"].isDouble())
    power = data["power"].toDouble();

  if(data["type"].isString())
    type = data["type"].toString();

  if(data["accuracy"].isDouble())
    accuracy = data["accuracy"].toDouble();

  if(data["pp"].isDouble())
    pp = data["pp"].toDouble();

  if(data["tm"].isDouble())
    tm = data["tm"].toDouble();

  if(data["hm"].isDouble())
    hm = data["hm"].toDouble();
}

void MoveDBEntry::deepLink()
{
  if(type != "")
    toType = TypesDB::ind.value(type, nullptr);

  if(tm && !hm)
    toItem = ItemsDB::ind.value("tm" + QString::number(*tm), nullptr);
  else if(tm && hm)
    toItem = ItemsDB::ind.value("hm" + QString::number(*hm), nullptr);

#ifdef QT_DEBUG
  if(type != "" && toType == nullptr)
    qCritical() << "Move Type: " << type << ", could not be deep linked." ;

  if((tm || hm) && toItem == nullptr)
    qCritical() << "Move: " << name << ", could not be deep linked." ;
#endif

  if(toType != nullptr)
    toType->toMoves.append(this);
}

void MovesDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("moves");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new MoveDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }
}

void MovesDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);

    if(entry->tm)
      ind.insert("tm" + QString::number(*entry->tm), entry);
    if(entry->hm)
      ind.insert("hm" + QString::number(*entry->hm), entry);
  }
}

void MovesDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<MoveDBEntry*> MovesDB::store = QVector<MoveDBEntry*>();
QHash<QString, MoveDBEntry*> MovesDB::ind = QHash<QString, MoveDBEntry*>();
