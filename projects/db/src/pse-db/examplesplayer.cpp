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

#include "./examplesplayer.h"
#include "./gamedata.h"
#include <pse-common/random.h>

void ExamplesPlayer::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("playerExamples");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }

  delete jsonData;
}

QString ExamplesPlayer::randomExample()
{
  int index = Random::rangeExclusive(0, store.size());
  QString ret = store.at(index);
  store.removeAt(index);

  if(store.size() == 0)
    load();

  return ret;
}

var32 ExamplesPlayer::lastInd = 0;
QVector<QString> ExamplesPlayer::store = QVector<QString>();
