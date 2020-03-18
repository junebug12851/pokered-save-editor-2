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
#ifndef MAPDBENTRYSPRITETRAINER_H
#define MAPDBENTRYSPRITETRAINER_H

#include "./mapdbentrysprite.h"

class TrainerDBEntry;
class MapsDB;

// A trainer that can be battled
struct DB_AUTOPORT MapDBEntrySpriteTrainer : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getTrainerClass READ getTrainerClass CONSTANT)
  Q_PROPERTY(int getTeam READ getTeam CONSTANT)
  Q_PROPERTY(TrainerDBEntry* getToTrainer READ getToTrainer CONSTANT)

public:
  virtual SpriteType type() const;
  const QString getTrainerClass() const;
  int getTeam() const;
  const TrainerDBEntry* getToTrainer() const;

protected:
  MapDBEntrySpriteTrainer(const QJsonValue& data, MapDBEntry* const parent);
  virtual void deepLink();
  virtual void qmlRegister() const;

  // Trainer Details
  // What kind of trainer and which team
  QString trainerClass = "";
  int team = -1;

  TrainerDBEntry* toTrainer = nullptr;

  friend class MapsDB;
  friend class MapDBEntry;
};

#endif // MAPDBENTRYSPRITETRAINER_H
