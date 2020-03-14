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
#ifndef WORLDOTHER_H
#define WORLDOTHER_H

#include <QObject>
#include <pse-common/types.h>
class SaveFile;

// Counts playtime up to
// 10 days, 15 hours, 59 minutes, 59 seconds, and 59 frames
class Playtime : public QObject {
  Q_OBJECT

  Q_PROPERTY(int days READ getDays WRITE setDays NOTIFY hoursChanged)
  Q_PROPERTY(int hours MEMBER hours NOTIFY hoursChanged)
  Q_PROPERTY(int hoursAdjusted READ getHoursAdjusted WRITE setHoursAdjusted NOTIFY hoursChanged)
  Q_PROPERTY(int minutes MEMBER minutes NOTIFY minutesChanged)
  Q_PROPERTY(int seconds MEMBER seconds NOTIFY secondsChanged)
  Q_PROPERTY(int frames MEMBER frames NOTIFY framesChanged)
  Q_PROPERTY(bool clockMaxed MEMBER clockMaxed NOTIFY clockMaxedChanged)

  // Converts hours into days and allows converting back from days
  Q_INVOKABLE int getDays();
  Q_INVOKABLE void setDays(int val);

  // Gets hours within the day,
  Q_INVOKABLE int getHoursAdjusted();
  Q_INVOKABLE void setHoursAdjusted(int val);

signals:
  void hoursChanged();
  void minutesChanged();
  void secondsChanged();
  void framesChanged();
  void clockMaxedChanged();

public:
  int hours; // Max 255
  int minutes; // Max 59, any higher will reset to zero and increment hr by 1
  int seconds; // Max 59, any higher will reset to zero and increment min by 1
  int frames; // Max 59, any higher will reset to zero and increment sec by 1

  // Any value, it just stops the clock completely
  bool clockMaxed;
};

class WorldOther : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool debugMode MEMBER debugMode NOTIFY debugModeChanged)
  Q_PROPERTY(Playtime* playtime MEMBER playtime NOTIFY playtimeChanged)
  Q_PROPERTY(int fossilItemGiven MEMBER fossilItemGiven NOTIFY fossilItemGivenChanged)
  Q_PROPERTY(int fossilPkmnResult MEMBER fossilPkmnResult NOTIFY fossilPkmnResultChanged)

public:
  WorldOther(SaveFile* saveFile = nullptr);
  virtual ~WorldOther();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void debugModeChanged();
  void playtimeChanged();
  void fossilItemGivenChanged();
  void fossilPkmnResultChanged();

public slots:
  void reset();
  void randomize();
  void randomizePlaytime();
  void clearPlaytime();

public:
  // Hold B to avoid wild battles
  // It also skips most of Prof Oaks speech and sets default names however the
  // latter two don't apply since loading a saveFile means all that is over with
  // anyways
  bool debugMode;

  // Playtime
  Playtime* playtime;

  // Fossils
  int fossilItemGiven;
  int fossilPkmnResult;
};

#endif // WORLDOTHER_H
