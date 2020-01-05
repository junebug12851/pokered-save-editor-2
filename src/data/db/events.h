/*
  * Copyright 2019 June Hanabi
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
#ifndef EVENTS_H
#define EVENTS_H

#include <QString>
#include <QHash>

#include "../../common/types.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// In-Game events, there's like a million of them, not kidding lol. Every little
// thing you do changes and moves around events

struct EventDBEntry {
  QString name; // Event name
  var16 ind; // Internal index
  var16 byte; // Byte in SAV file
  var8 bit; // Bit in byte
};

class EventsDB
{
public:
  static void load();
  static void index();

  static QVector<EventDBEntry*> store;
  static QHash<QString, EventDBEntry*> ind;
};

#endif // EVENTS_H
