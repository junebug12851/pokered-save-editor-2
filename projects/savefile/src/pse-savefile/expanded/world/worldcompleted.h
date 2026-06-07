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
#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

/**
 * @brief A handful of one-off "have you done X yet" milestone flags.
 *
 * Unlike the bulk WorldEvents bitfield, these are individually-meaningful named
 * one-shots the game tracks specially: the three fishing rods, the Lapras gift,
 * the starter, whether you've ever healed, the Saffron guards, and beating
 * Lorelei. Grouped (in the fields) into rods / Pokemon / other. Standard
 * expanded-node convention (see SaveFileExpanded).
 *
 * @see World, WorldEvents (the bulk event bitfield).
 */
class SAVEFILE_AUTOPORT WorldCompleted : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool obtainedOldRod MEMBER obtainedOldRod NOTIFY obtainedOldRodChanged)       ///< Got the Old Rod.
  Q_PROPERTY(bool obtainedGoodRod MEMBER obtainedGoodRod NOTIFY obtainedGoodRodChanged)    ///< Got the Good Rod.
  Q_PROPERTY(bool obtainedSuperRod MEMBER obtainedSuperRod NOTIFY obtainedSuperRodChanged) ///< Got the Super Rod.
  Q_PROPERTY(bool obtainedLapras MEMBER obtainedLapras NOTIFY obtainedLaprasChanged)       ///< Received the Lapras gift.
  Q_PROPERTY(bool obtainedStarterPokemon MEMBER obtainedStarterPokemon NOTIFY obtainedStarterPokemonChanged) ///< Chose a starter.
  Q_PROPERTY(bool everHealedPokemon MEMBER everHealedPokemon NOTIFY everHealedPokemonChanged) ///< Have ever healed at a center.
  Q_PROPERTY(bool satisfiedSaffronGuards MEMBER satisfiedSaffronGuards NOTIFY satisfiedSaffronGuardsChanged) ///< Gave the Saffron guards their drink.
  Q_PROPERTY(bool defeatedLorelei MEMBER defeatedLorelei NOTIFY defeatedLoreleiChanged)    ///< Beat Elite Four Lorelei.

public:
  WorldCompleted(SaveFile* saveFile = nullptr);
  virtual ~WorldCompleted();

  void load(SaveFile* saveFile = nullptr); ///< Expand these flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten these flags to the save.

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
  void reset();     ///< Blank these milestones.
  void randomize(); ///< Randomize these milestones.

public:
  // Rods
  bool obtainedOldRod;          ///< @see obtainedOldRod property.
  bool obtainedGoodRod;         ///< @see obtainedGoodRod property.
  bool obtainedSuperRod;        ///< @see obtainedSuperRod property.

  // Pokemon
  bool obtainedLapras;          ///< @see obtainedLapras property.
  bool obtainedStarterPokemon;  ///< @see obtainedStarterPokemon property.
  bool everHealedPokemon;       ///< @see everHealedPokemon property.

  // Other
  bool satisfiedSaffronGuards;  ///< @see satisfiedSaffronGuards property.
  bool defeatedLorelei;         ///< @see defeatedLorelei property.
};
