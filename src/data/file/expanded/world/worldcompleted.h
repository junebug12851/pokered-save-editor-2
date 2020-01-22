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
#ifndef WORLDCOMPLETED_H
#define WORLDCOMPLETED_H

#include <QObject>
#include "../../../../common/types.h"
class SaveFile;

class WorldCompleted : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool obtainedOldRod_ MEMBER obtainedOldRod NOTIFY obtainedOldRodChanged)
  Q_PROPERTY(bool obtainedGoodRod_ MEMBER obtainedGoodRod NOTIFY obtainedGoodRodChanged)
  Q_PROPERTY(bool obtainedSuperRod_ MEMBER obtainedSuperRod NOTIFY obtainedSuperRodChanged)
  Q_PROPERTY(bool obtainedLapras_ MEMBER obtainedLapras NOTIFY obtainedLaprasChanged)
  Q_PROPERTY(bool obtainedStarterPokemon_ MEMBER obtainedStarterPokemon NOTIFY obtainedStarterPokemonChanged)
  Q_PROPERTY(bool everHealedPokemon_ MEMBER everHealedPokemon NOTIFY everHealedPokemonChanged)
  Q_PROPERTY(bool satisfiedSaffronGuards_ MEMBER satisfiedSaffronGuards NOTIFY satisfiedSaffronGuardsChanged)
  Q_PROPERTY(bool defeatedLorelei_ MEMBER defeatedLorelei NOTIFY defeatedLoreleiChanged)

public:
  WorldCompleted(SaveFile* saveFile = nullptr);
  virtual ~WorldCompleted();

signals:
  void obtainedOldRodChanged();
  void obtainedGoodRodChanged();
  void obtainedSuperRodChanged();
  void obtainedLaprasChanged();
  void obtainedStarterPokemonChanged();
  void everHealedPokemonChanged();
  void satisfiedSaffronGuardsChanged();
  void defeatedLoreleiChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  // Rods
  bool obtainedOldRod;
  bool obtainedGoodRod;
  bool obtainedSuperRod;

  // Pokemon
  bool obtainedLapras;
  bool obtainedStarterPokemon;
  bool everHealedPokemon;

  // Other
  bool satisfiedSaffronGuards;
  bool defeatedLorelei;
};

#endif // WORLDCOMPLETED_H
