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
#include "mapdbentryspritetrainer.h"

MapDBEntrySpriteTrainer::MapDBEntrySpriteTrainer(QJsonValue& data, MapDBEntry* parent) :
  MapDBEntrySprite(data, parent)
{
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

SpriteType MapDBEntrySpriteTrainer::type()
{
  return SpriteType::TRAINER;
}
