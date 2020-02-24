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
#ifndef ITEMS_H
#define ITEMS_H

#include <QVector>
#include <QString>
#include <QHash>
#include <QJsonValue>

#include "optional"

#include "../../common/types.h"

// Prevents includes from including each other and causing errors
// We include them in the cpp file
struct MoveDBEntry;
struct MapDBEntrySpriteItem;
struct PokemonDBEntryEvolution;
struct PokemonDBEntry;
struct GameCornerDBEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All the in-game items and glitch items

struct ItemDBEntry {
  ItemDBEntry();
  ItemDBEntry(QJsonValue& data);
  void deepLink();

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  QString name; // Item Output
  var8 ind = 0; // Item Code
  bool once = false; // Item can only be obtained once
  bool glitch = false; // Item is a glitch item
  QString readable;

  std::optional<var8> tm; // TM Number if present
  std::optional<var8> hm; // HM Number if present

  // Item Prices if available
  std::optional<var8> price;
  int buyPriceMoney();
  int buyPriceCoins();
  int sellPriceMoney();
  int sellPriceCoins();
  bool canSell();
  bool isGameCornerExclusive();

  MoveDBEntry* toMove = nullptr; // To TM or HM Move
  GameCornerDBEntry* toGameCorner = nullptr;
  QVector<MapDBEntrySpriteItem*> toMapSpriteItem;
  QVector<PokemonDBEntryEvolution*> toEvolvePokemon;
  QVector<PokemonDBEntry*> toTeachPokemon;
};

class ItemsDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<ItemDBEntry*> store;
  static QHash<QString, ItemDBEntry*> ind;
};

#endif // ITEMS_H
