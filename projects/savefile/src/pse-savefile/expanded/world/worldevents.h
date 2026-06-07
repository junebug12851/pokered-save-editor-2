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

// Total number of known events
// We can't break this down into a byte count because these bits
// have to be gotten all over the place
constexpr var16 eventCount = 508; ///< Number of known story-event flags (scattered across the save).

/**
 * @brief The game's story-event flags -- a flat array of 508 booleans.
 *
 * Gen 1 tracks story progress as individual event bits scattered all over the
 * save, so this object presents them as one flat @ref completedEvents array of
 * @ref eventCount flags, with QML count/at/set access. Toggling these is how
 * story flags are edited. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World.
 */
class SAVEFILE_AUTOPORT WorldEvents : public QObject
{
  Q_OBJECT

public:
  WorldEvents(SaveFile* saveFile = nullptr);
  virtual ~WorldEvents();

  Q_INVOKABLE int eventsCount();             ///< Number of event flags (eventCount).
  Q_INVOKABLE bool eventsAt(int ind);        ///< Is event @p ind set?
  Q_INVOKABLE void eventsSet(int ind, bool val); ///< Set/clear event @p ind.

signals:
  void completedEventsChanged();

public slots:
  void load(SaveFile* saveFile = nullptr); ///< Expand all event bits from the save.
  void save(SaveFile* saveFile);           ///< Flatten all event bits to the save.
  void reset();                            ///< Clear every event.
  void randomize();                        ///< Randomize the event flags.

public:
  bool completedEvents[eventCount]; ///< One flag per known story event.
};
