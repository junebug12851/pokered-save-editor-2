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
#ifndef GAMECORNERDB_H
#define GAMECORNERDB_H

#include <optional>

#include <QVector>
#include <QString>
#include <QJsonValue>

class PokemonDBEntry;
class ItemDBEntry;

struct GameCornerDBEntry {
  GameCornerDBEntry();
  GameCornerDBEntry(QJsonValue& data);
  void deepLink();

  QString name = "";
  QString type = "";
  int price = 0;
  std::optional<int> level;

  PokemonDBEntry* toPokemon = nullptr;
  ItemDBEntry* toItem = nullptr;
};

class GameCornerDB
{
public:
  static void load();
  static void deepLink();

  // Buy and Sell Price
  // Pokedollars <=> Game Coins
  // Regular Casinos give you an even exchange, you get the exact amount back
  // But in the Poke-World I want to follow the global sell-back mechanics
  // whereby you get half back
  static int buyPrice;
  static int sellPrice;

  static QVector<GameCornerDBEntry*> store;
};

#endif // GAMECORNERDB_H