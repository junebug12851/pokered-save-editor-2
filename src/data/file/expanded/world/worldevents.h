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
#ifndef WORLDEVENTS_H
#define WORLDEVENTS_H

#include <QObject>
#include "../../../../common/types.h"
class SaveFile;

// Total number of known events
// We can't break this down into a byte count because these bits
// have to be gotten all over the place
constexpr var16 eventCount = 508;

class WorldEvents : public QObject
{
  Q_OBJECT

public:
  WorldEvents(SaveFile* saveFile = nullptr);
  virtual ~WorldEvents();

signals:
  void completedEventsChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  bool completedEvents[eventCount];
};

#endif // WORLDEVENTS_H
