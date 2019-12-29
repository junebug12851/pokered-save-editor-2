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
#ifndef POKEMON_H
#define POKEMON_H

#include "../common/types.h"
#include <optional>
#include <QString>
#include <QVector>
#include <QJsonValue>

// With amazing help of Quicktype!!!
// Really needed it with Pokemon as this was quite complicated
// https://app.quicktype.io

// Optional bool values are only present when true,
// Optional arrays are empty when not present,
// Strings are empty when not present
// so we simplify things and avoid using variant unless dealing with primitive
// types

// All the Pokemon and glitch Pokemon in the game

// Forward Declare Structs
struct EvolutionEntry;
struct MoveEntry;
struct PokemonEntry;

struct EvolutionEntry
{
  EvolutionEntry(QJsonValue& data);

  QString toName;
  bool trade;
  QString item;

  std::optional<var8> level;
};

struct MoveEntry
{
  MoveEntry(QJsonValue& data);

  var8 level;
  QString move;
};

struct PokemonEntry {
  PokemonEntry();

  QString name;
  var8 ind;
  bool glitch;
  QString type1;
  QString type2;

  QVector<MoveEntry*>* moves;
  QVector<QString>* initial;
  QVector<var8>* tmHm;
  QVector<EvolutionEntry*>* evolution;

  std::optional<var8> pokedex;
  std::optional<var8> growthRate;
  std::optional<var8> baseHp;
  std::optional<var8> baseAttack;
  std::optional<var8> baseDefense;
  std::optional<var8> baseSpeed;
  std::optional<var8> baseSpecial;
  std::optional<var8> baseExpYield;
  std::optional<var8> catchRate;
};

class Pokemon
{
public:
  static void load();
  static QVector<PokemonEntry*>* pokemon;
};

#endif // POKEMON_H
