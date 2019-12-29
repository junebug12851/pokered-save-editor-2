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
#include "starterPokemon.h"
#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QRandomGenerator>
#include "./gamedata.h"

void StarterPokemon::load()
{
  // Grab Event Pokemon Data
  auto starterData = GameData::json("starters");

  // Go through each event Pokemon
  for(QJsonValue starterEntry : starterData->array())
  {
    // Add to array
    starters->append(starterEntry.toString());
  }
}

QString StarterPokemon::random3Starter()
{
  // First 3 in list are in-game starters
  var32 ind = QRandomGenerator::global()->bounded(0, 3);
  return starters->at(ind);
}

QString StarterPokemon::randomAnyStarter()
{
  // List as a whole are all potential starters
  var32 ind = QRandomGenerator::global()->bounded(0, starters->size());
  return starters->at(ind);
}

QVector<QString>* StarterPokemon::starters = new QVector<QString>();
