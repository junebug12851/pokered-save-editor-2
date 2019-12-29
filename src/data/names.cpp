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
#include "names.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

void Names::load()
{
  // Grab Event Pokemon Data
  auto nameData = GameData::json("names");

  // Go through each event Pokemon
  for(QJsonValue nameEntry : nameData->array())
  {
    // Add to array
    names->append(nameEntry.toString());
  }
}

QVector<QString>* Names::names = new QVector<QString>();
