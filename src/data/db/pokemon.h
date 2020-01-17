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

#include <QString>
#include <QVector>
#include <QJsonValue>
#include <QHash>

#include <optional>

#include "../../common/types.h"

// The Pokedex data starts at pokedex #0 for Bulbasaur
// It ends at #150 for Mew, it's size is 151 dex mons
// These are also here for code cleanliness
constexpr var8 pokemonDexCount = 151;
constexpr var8 pokemonLevelMax = 100;

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
struct PokemonDBEntryEvolution;
struct MoveDBEntry;
struct PokemonDBEntry;

// Deep link forward-declarations
struct ItemDBEntry;
struct MoveDBEntry;
struct TypeDBEntry;
struct EventPokemonDBEntry;
struct MapDBEntrySpritePokemon;
struct MapDBEntryWildMon;
struct TradeDBEntry;

struct PokemonDBEntryEvolution
{
  PokemonDBEntryEvolution(QJsonValue& data);
  void deepLink(PokemonDBEntry* deEvolution);

  QString toName;
  bool trade;
  QString item;

  std::optional<var8> level;

  PokemonDBEntry* toDeEvolution;
  PokemonDBEntry* toEvolution;
  ItemDBEntry* toItem;
};

struct PokemonDBEntryMove
{
  PokemonDBEntryMove(QJsonValue& data);
  void deepLink();

  var8 level;
  QString move;

  MoveDBEntry* toMove;
};

struct PokemonDBEntry {
  PokemonDBEntry();
  void deepLink();

  QString name;
  var8 ind;
  QString readable;
  bool glitch;
  QString type1;
  QString type2;

  QVector<PokemonDBEntryMove*>* moves;
  QVector<QString>* initial;
  QVector<var8>* tmHm;
  QVector<PokemonDBEntryEvolution*>* evolution;

  std::optional<var8> pokedex;
  std::optional<var8> growthRate;
  std::optional<var8> baseHp;
  std::optional<var8> baseAttack;
  std::optional<var8> baseDefense;
  std::optional<var8> baseSpeed;
  std::optional<var8> baseSpecial;
  std::optional<var8> baseExpYield;
  std::optional<var8> catchRate;

  // Lots of deep linking
  TypeDBEntry* toType1;
  TypeDBEntry* toType2;
  PokemonDBEntry* toDeEvolution;
  QVector<MoveDBEntry*> toInitial;
  QVector<MoveDBEntry*> toTmHmMove;
  QVector<ItemDBEntry*> toTmHmItem;
  QVector<EventPokemonDBEntry*> toEventMons;
  MapDBEntrySpritePokemon* toMapSpritePokemon;
  QVector<MapDBEntryWildMon*> toWildMonMaps;
  QVector<TradeDBEntry*> toTrades;
};

class PokemonDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<PokemonDBEntry*> store;
  static QHash<QString, PokemonDBEntry*> ind;
};

#endif // POKEMON_H
