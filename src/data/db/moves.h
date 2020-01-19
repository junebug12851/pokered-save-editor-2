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
#ifndef MOVE_H
#define MOVE_H

#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include <optional>

#include "../../common/types.h"

struct TypeDBEntry;
struct ItemDBEntry;
struct PokemonDBEntryMove;
struct PokemonDBEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All the Pokemon moves in the game including special or glitch moves

struct MoveDBEntry {

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  MoveDBEntry();
  MoveDBEntry(QJsonValue& data);
  void deepLink();

  QString name;
  var8 ind = 0;
  bool glitch = false;
  QString type;
  QString readable;

  std::optional<var8> power;
  std::optional<var8> accuracy;
  std::optional<var8> pp;
  std::optional<var8> tm;
  std::optional<var8> hm;

  TypeDBEntry* toType = nullptr; // Deep link to move type
  ItemDBEntry* toItem = nullptr; // Deep link to tm/hm item if present
  QVector<PokemonDBEntryMove*> toPokemonLearned;
  QVector<PokemonDBEntry*> toPokemonInitial;
  QVector<PokemonDBEntry*> toPokemonTmHm;
};

class MovesDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<MoveDBEntry*> store;
  static QHash<QString, MoveDBEntry*> ind;
};

#endif // MOVE_H
