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

#include <QString>
#include <QVector>
#include <QJsonValue>

#include <variant>
#include <optional>

#include "../../common/types.h"

struct PokemonDBEntry;

// These are Pokemon you get by going to or participating in real-life events
// that were held around the world

struct EventPokemonDBEntry {
  EventPokemonDBEntry();
  EventPokemonDBEntry(QJsonValue& data);
  void deepLink();

  QString title; // Event Title
  QString desc; // Event Pokemon Description
  QString pokemon; // Pokemon Name
  QVector<QString> otName; // Pokemon OT Name
  std::optional<var16> otId; // Pokemon OT ID, random if not specified
  QVector<var8> dv; // Pokemon DV List, random if not specified
  QString region; // Region Code
  QVector<QString> moves; // Move list
  std::optional<var8> level; // Level, default minimum if not specified

  PokemonDBEntry* toPokemon = nullptr; // Deep link to associated Pokemon
};

class EventPokemonDB
{
public:
  static void load();
  static void deepLink();

  static QVector<EventPokemonDBEntry*> store;
};

#endif // EVENTPOKEMON_H
