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

// Counts playtime up to
// 10 days, 15 hours, 59 minutes, 59 seconds, and 59 frames
/**
 * @brief The save's playtime clock, surfaced to QML as days/hours/minutes/seconds/frames.
 *
 * Stored on disk as hours/minutes/seconds/frames (hours up to 255), this object
 * adds day<->hour conversion (@ref getDays / @ref getHoursAdjusted) so the UI can
 * show "10 days, 15 hours" instead of raw hours, and a @ref clockMaxed flag that
 * stops the clock. Backs the Trainer Card playtime display. The field comments
 * below give the per-unit overflow rules.
 *
 * @see WorldOther (owns this).
 */
class SAVEFILE_AUTOPORT Playtime : public QObject {
  Q_OBJECT

  Q_PROPERTY(int days READ getDays WRITE setDays NOTIFY hoursChanged)                 ///< Whole days (derived from hours).
  Q_PROPERTY(int hours MEMBER hours NOTIFY hoursChanged)                              ///< Total hours (max 255).
  Q_PROPERTY(int hoursAdjusted READ getHoursAdjusted WRITE setHoursAdjusted NOTIFY hoursChanged) ///< Hours within the current day.
  Q_PROPERTY(int minutes MEMBER minutes NOTIFY minutesChanged)                        ///< Minutes (0-59).
  Q_PROPERTY(int seconds MEMBER seconds NOTIFY secondsChanged)                        ///< Seconds (0-59).
  Q_PROPERTY(int frames MEMBER frames NOTIFY framesChanged)                           ///< Frames (0-59).
  Q_PROPERTY(bool clockMaxed MEMBER clockMaxed NOTIFY clockMaxedChanged)              ///< Stops the clock completely.

  // Converts hours into days and allows converting back from days
  Q_INVOKABLE int getDays();          ///< Hours expressed as whole days.
  Q_INVOKABLE void setDays(int val);  ///< Set the day component (adjusts hours).

  // Gets hours within the day,
  Q_INVOKABLE int getHoursAdjusted();         ///< Hours within the current day (0-23-ish).
  Q_INVOKABLE void setHoursAdjusted(int val); ///< Set hours-within-day (adjusts total hours).

signals:
  void hoursChanged();
  void minutesChanged();
  void secondsChanged();
  void framesChanged();
  void clockMaxedChanged();

public:
  int hours; ///< Max 255
  int minutes; ///< Max 59, any higher will reset to zero and increment hr by 1
  int seconds; ///< Max 59, any higher will reset to zero and increment min by 1
  int frames; ///< Max 59, any higher will reset to zero and increment sec by 1

  // Any value, it just stops the clock completely
  bool clockMaxed; ///< Any value; it just stops the clock completely.
};

/**
 * @brief Odds-and-ends world state: debug mode, the playtime clock, fossil results.
 *
 * The one World child QML traverses (for the Trainer Card playtime). Carries the
 * @ref debugMode flag, the @ref playtime clock, and the fossil-revival bookkeeping
 * (@ref fossilItemGiven / @ref fossilPkmnResult). Standard expanded-node
 * convention (see SaveFileExpanded).
 *
 * @see World, Playtime.
 */
class SAVEFILE_AUTOPORT WorldOther : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool debugMode MEMBER debugMode NOTIFY debugModeChanged)                 ///< In-game debug mode flag (see field note).
  Q_PROPERTY(Playtime* playtime MEMBER playtime NOTIFY playtimeChanged)               ///< The playtime clock.
  Q_PROPERTY(int fossilItemGiven MEMBER fossilItemGiven NOTIFY fossilItemGivenChanged) ///< Fossil item handed to the lab.
  Q_PROPERTY(int fossilPkmnResult MEMBER fossilPkmnResult NOTIFY fossilPkmnResultChanged) ///< Resulting revived Pokemon.

public:
  WorldOther(SaveFile* saveFile = nullptr);
  virtual ~WorldOther();

  void load(SaveFile* saveFile = nullptr); ///< Expand this region from the save.
  void save(SaveFile* saveFile);           ///< Flatten this region to the save.

signals:
  void debugModeChanged();
  void playtimeChanged();
  void fossilItemGivenChanged();
  void fossilPkmnResultChanged();

public slots:
  void reset();             ///< Blank this region.
  void randomize();         ///< Randomize this region.
  void randomizePlaytime(); ///< Randomize just the playtime clock.
  void clearPlaytime();     ///< Zero the playtime clock.

public:
  // Hold B to avoid wild battles
  // It also skips most of Prof Oaks speech and sets default names however the
  // latter two don't apply since loading a saveFile means all that is over with
  // anyways
  bool debugMode;

  // Playtime
  Playtime* playtime; ///< @see playtime property.

  // Fossils
  int fossilItemGiven;  ///< @see fossilItemGiven property.
  int fossilPkmnResult; ///< @see fossilPkmnResult property.
};
