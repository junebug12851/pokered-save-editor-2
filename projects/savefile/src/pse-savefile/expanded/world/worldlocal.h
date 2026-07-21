/*
  * Copyright 2020 Fairy Fox
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
 * @brief A few map-specific puzzle/minigame state values.
 *
 * Small, named bits of state tied to specific places: the two Lt. Surge trash-can
 * lock positions, the Cinnabar Gym quiz's next opponent, and the Safari Zone game
 * state (over-flag, balls left, steps left). Grouped in the fields by location.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World.
 */
class SAVEFILE_AUTOPORT WorldLocal : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int lock1 MEMBER lock1 NOTIFY lock1Changed)                       ///< Lt. Surge trash-can lock #1.
  Q_PROPERTY(int lock2 MEMBER lock2 NOTIFY lock2Changed)                       ///< Lt. Surge trash-can lock #2.
  Q_PROPERTY(int quizOpp MEMBER quizOpp NOTIFY quizOppChanged)                 ///< Cinnabar Gym next quiz opponent.
  Q_PROPERTY(bool safariGameOver MEMBER safariGameOver NOTIFY safariGameOverChanged) ///< Safari run is over.
  Q_PROPERTY(int safariBallCount MEMBER safariBallCount NOTIFY safariBallCountChanged) ///< Safari balls remaining.
  Q_PROPERTY(int safariSteps MEMBER safariSteps NOTIFY safariStepsChanged)     ///< Safari steps remaining.

public:
  WorldLocal(SaveFile* saveFile = nullptr);
  virtual ~WorldLocal();

  void load(SaveFile* saveFile = nullptr); ///< Expand these values from the save.
  void save(SaveFile* saveFile);           ///< Flatten these values to the save.

signals:
  void lock1Changed();
  void lock2Changed();
  void quizOppChanged();
  void safariGameOverChanged();
  void safariBallCountChanged();
  void safariStepsChanged();

public slots:
  void reset();     ///< Blank these values.
  void randomize(); ///< Randomize these values.

public:
  // Lt. Surge Trash Can Locks
  int lock1;             ///< @see lock1 property.
  int lock2;             ///< @see lock2 property.

  // Cinnabar Gym Next Opp
  int quizOpp;           ///< @see quizOpp property.

  // Safari
  bool safariGameOver;   ///< @see safariGameOver property.
  int safariBallCount;   ///< @see safariBallCount property.
  int safariSteps;       ///< @see safariSteps property.
};
