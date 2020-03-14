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

#include "./namesPokemon.h"
#include "./gamedata.h"
#include "../../common/random.h"

void NamesPokemonDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("namesPokemon");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }

  delete jsonData;
}

QString NamesPokemonDB::randomName()
{
  int index = Random::rangeExclusive(0, store.size());
  QString ret = store.at(index);
  store.removeAt(index);

  if(store.size() == 0)
    load();

  return ret;
}

int NamesPokemonDB::lastInd = 0;
QVector<QString> NamesPokemonDB::store = QVector<QString>();
