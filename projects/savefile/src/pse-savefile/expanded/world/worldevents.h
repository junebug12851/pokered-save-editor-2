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

/// Every event flag the game has: `wEventFlags` is `flag_array NUM_EVENTS`, and
/// pret's `event_constants.asm` ends at `const_next $A00` -- so NUM_EVENTS is
/// **0xA00 = 2560**, fixed by the ROM. Raised from 508 (v1 only listed the flags
/// it had names for) on 2026-07-16; `EventsDB` now carries all 2560, and this
/// array is indexed by the DB's store size, so the two MUST stay in step.
/// @see notes/reference/event-flags.md
constexpr var16 eventCount = 2560; ///< Number of story-event flags (= pret NUM_EVENTS, $A00).

/**
 * @brief The game's story-event flags -- a flat array of @ref eventCount booleans.
 *
 * Gen 1 keeps story progress in **`wEventFlags`: ONE CONTIGUOUS 320-byte bitfield**
 * at save offset **0x29F3--0x2B32** (WRAM 0xD747--0xD886), ending exactly where
 * `wGrassRate` begins -- bit `i` lives at byte `0x29F3 + i/8`, bit `i%8`. (The old
 * comment here said the bits were "scattered all over the save"; they are not --
 * that was legacy confusion. `EventsDB` merely stores each bit's absolute
 * byte+bit, which is why it *looks* scattered.)
 *
 * This object presents them as one flat @ref completedEvents array with QML
 * count/at/set access; toggling these is how story flags are edited. Standard
 * expanded-node convention (see SaveFileExpanded).
 *
 * @see World, notes/reference/event-flags.md
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
