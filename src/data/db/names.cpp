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

#include "./names.h"
#include "./gamedata.h"
#include "../../random.h"

void NamesDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("names");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }

  delete jsonData;
}

QString NamesDB::randomName()
{
  var32 ind = Random::rangeExclusive(0, store.size());
  return store.at(ind);
}

QVector<QString> NamesDB::store = QVector<QString>();
