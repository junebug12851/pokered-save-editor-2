/*
  * Copyright 2020 June Hanabi
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

#include "./examplesrival.h"
#include "./gamedata.h"
#include "../../common/random.h"

void ExamplesRival::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("rivalExamples");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }

  delete jsonData;
}

QString ExamplesRival::randomExample()
{
  lastInd++;
  if(lastInd >= (var32)store.size())
    lastInd = 0;

  return store.at(lastInd);
}

var32 ExamplesRival::lastInd = 0;
QVector<QString> ExamplesRival::store = QVector<QString>();
