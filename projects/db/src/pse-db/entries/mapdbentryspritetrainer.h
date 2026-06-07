/*
  * Copyright 2020 Twilight
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
#pragma once
#include "./mapdbentrysprite.h"

class TrainerDBEntry;
class MapsDB;

// A trainer that can be battled
/**
 * @brief A map sprite that is a battleable trainer (type TRAINER).
 *
 * Adds the @ref trainerClass and which @ref team (class resolved to @ref toTrainer
 * in deepLink) to MapDBEntrySprite. type() returns TRAINER. See db.md.
 *
 * @see MapDBEntrySprite (base), TrainerDBEntry.
 */
struct DB_AUTOPORT MapDBEntrySpriteTrainer : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getTrainerClass READ getTrainerClass CONSTANT) ///< Trainer class name.
  Q_PROPERTY(int getTeam READ getTeam CONSTANT)                     ///< Which roster/team.
  Q_PROPERTY(TrainerDBEntry* getToTrainer READ getToTrainer CONSTANT) ///< Resolved trainer class.

public:
  virtual SpriteType type() const;        ///< Returns TRAINER.
  const QString getTrainerClass() const;  ///< @see getTrainerClass property.
  int getTeam() const;                    ///< @see getTeam property.
  TrainerDBEntry* getToTrainer() const;   ///< @see getToTrainer property.

protected:
  MapDBEntrySpriteTrainer(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  virtual void deepLink();          ///< Resolve the trainer-class link.
  virtual void qmlRegister() const; ///< Register with QML.

  // Trainer Details
  // What kind of trainer and which team
  QString trainerClass = ""; ///< Trainer class name (read via getTrainerClass()).
  int team = -1;             ///< Roster/team index.

  TrainerDBEntry* toTrainer = nullptr; ///< Resolved trainer class (deepLink).

  friend class MapsDB;
  friend class MapDBEntry;
};
