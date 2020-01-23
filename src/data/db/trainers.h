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
#ifndef TRAINER_H
#define TRAINER_H

#include <QMetaType>
#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include "../../common/types.h"

struct MapDBEntrySpriteTrainer;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All trainer classes in the game, this includes unused or glitch ones

struct TrainerDBEntry {
  TrainerDBEntry();
  TrainerDBEntry(QJsonValue& data);

  QString name;
  var8 ind = 0;
  bool unused = false;
  bool opp = false;

  QVector<MapDBEntrySpriteTrainer*> tpMapSpriteTrainers;
};

Q_DECLARE_METATYPE(TrainerDBEntry)

class TrainersDB
{
public:
  static void load();
  static void index();

  static QVector<TrainerDBEntry*> store;
  static QHash<QString, TrainerDBEntry*> ind;
};

Q_DECLARE_METATYPE(TrainersDB)

#endif // TRAINER_H
