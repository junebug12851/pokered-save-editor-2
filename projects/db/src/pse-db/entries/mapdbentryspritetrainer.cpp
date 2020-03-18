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

#include <QDebug>
#include <QJsonValue>
#include <QQmlEngine>
#include <pse-common/utility.h>
#include "mapdbentryspritetrainer.h"

#include "../trainers.h"

MapDBEntrySpriteTrainer::MapDBEntrySpriteTrainer(const QJsonValue& data,
                                                 MapDBEntry* const parent) :
  MapDBEntrySprite(data, parent)
{
  qmlRegister();

  trainerClass = data["class"].toString();
  team = data["team"].toDouble();
}

void MapDBEntrySpriteTrainer::deepLink()
{
  MapDBEntrySprite::deepLink();
  toTrainer = TrainersDB::ind.value("Opp"+trainerClass);

#ifdef QT_DEBUG
  if(toTrainer == nullptr)
    qCritical() << "MapDBEntrySpriteTrainer: Unable to deep link " + trainerClass + " to trainer";
#endif

  if(toTrainer != nullptr)
    toTrainer->tpMapSpriteTrainers.append(this);
}

void MapDBEntrySpriteTrainer::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntrySpriteTrainer>(
        "PSE.DB.MapDBEntrySpriteTrainer", 1, 0, "MapDBEntrySpriteTrainer", "Can't instantiate in QML");
  once = true;
}

const TrainerDBEntry* MapDBEntrySpriteTrainer::getToTrainer() const
{
    return toTrainer;
}

int MapDBEntrySpriteTrainer::getTeam() const
{
    return team;
}

const QString MapDBEntrySpriteTrainer::getTrainerClass() const
{
    return trainerClass;
}

MapDBEntrySpriteTrainer::SpriteType MapDBEntrySpriteTrainer::type() const
{
    return SpriteType::TRAINER;
}
