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
#include <QtMath>
#include <QRandomGenerator>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./tmHm.h"
#include "./gamedata.h"
#include "./items.h"
#include "./moves.h"

void TmHmsDB::load()
{
  // Grab Event Pokemon Data
  auto tmHmData = GameData::json("tmHm");

  // Go through each event Pokemon
  for(QJsonValue tmhmEntry : tmHmData->array())
  {
    // Add to array
    store.append(tmhmEntry.toString());
  }

  delete tmHmData;
}

void TmHmsDB::deepLink()
{
  for(var8 i = 0; i < store.size(); i++)
  {
    auto entry = store.at(i); // Move Name
    var8 ind = i + 1; // Move TM Number

    // Deep link to tm number
    toTmHmItem.append(ItemsDB::ind.value("tm" + QString::number(ind), nullptr));
    toTmHmMove.append(MovesDB::ind.value("tm" + QString::number(ind), nullptr));

#ifdef QT_DEBUG
    if(toTmHmItem.at(i) == nullptr)
      qCritical() << "TM/HM Item: " << entry << ", could not be deep linked." ;
    if(toTmHmMove.at(i) == nullptr)
      qCritical() << "TM/HM Move: " << entry << ", could not be deep linked." ;
#endif
  }
}

QVector<QString> TmHmsDB::store = QVector<QString>();
QVector<ItemDBEntry*> TmHmsDB::toTmHmItem = QVector<ItemDBEntry*>();
QVector<MoveDBEntry*> TmHmsDB::toTmHmMove = QVector<MoveDBEntry*>();
