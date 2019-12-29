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
#ifndef EVENTPOKEMON_H
#define EVENTPOKEMON_H

// With amazing help of Quicktype!!!
// https://app.quicktype.io

#include "../common/types.h"
#include <variant>
#include <optional>
#include <QString>
#include <QVector>

// Optional
using OtId =   std::optional<var16>;
using DV =     QVector<var8>*;
using Level =  std::optional<var8>;

struct EventPokemonEntry {
    EventPokemonEntry();

    QString title; // Event Title
    QString desc; // Event Pokemon Description
    QString pokemon; // Pokemon Name
    QVector<QString>* otName; // Pokemon OT Name
    OtId otId; // Pokemon OT ID, random if not specified
    DV dv; // Pokemon DV List, random if not specified
    QString region; // Region Code
    QVector<QString>* moves; // Move list
    Level level; // Level, default minimum if not specified
};

class EventPokemon
{
public:
  static void load();
  static QVector<EventPokemonEntry*>* eventPokemon;
};

#endif // EVENTPOKEMON_H
