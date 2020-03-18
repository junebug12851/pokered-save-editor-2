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
#ifndef ITEMDBENTRY_H
#define ITEMDBENTRY_H

#include <QObject>
#include <QString>
#include <QJsonValue>
#include "../db_autoport.h"

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All the in-game items and glitch items

// Prevents includes from including each other and causing errors
// We include them in the cpp file
struct MoveDBEntry;
struct MapDBEntrySpriteItem;
struct PokemonDBEntryEvolution;
struct PokemonDBEntry;
struct GameCornerDBEntry;
class QQmlEngine;
class ItemsDB;

struct DB_AUTOPORT ItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int getInd READ getInd CONSTANT)
  Q_PROPERTY(bool getOnce READ getOnce CONSTANT)
  Q_PROPERTY(bool getGlitch READ getGlitch CONSTANT)
  Q_PROPERTY(QString getReadable READ getReadable CONSTANT)
  Q_PROPERTY(int getTm READ getTm CONSTANT)
  Q_PROPERTY(int getHm READ getHm CONSTANT)
  Q_PROPERTY(int getPrice READ getPrice CONSTANT)
  Q_PROPERTY(int buyPriceMoney READ buyPriceMoney CONSTANT)
  Q_PROPERTY(int buyPriceCoins READ buyPriceCoins CONSTANT)
  Q_PROPERTY(int sellPriceMoney READ sellPriceMoney CONSTANT)
  Q_PROPERTY(int sellPriceCoins READ sellPriceCoins CONSTANT)
  Q_PROPERTY(bool canSell READ canSell CONSTANT)
  Q_PROPERTY(bool isGameCornerExclusive READ isGameCornerExclusive CONSTANT)
  Q_PROPERTY(MoveDBEntry* getToMove READ getToMove CONSTANT)
  Q_PROPERTY(GameCornerDBEntry* getToGameCorner READ getToGameCorner CONSTANT)
  Q_PROPERTY(int getToMapSpriteItemSize READ getToMapSpriteItemSize CONSTANT)
  Q_PROPERTY(MapDBEntrySpriteItem* getToMapSpriteItemAt READ getToMapSpriteItemAt CONSTANT)
  Q_PROPERTY(int getToEvolvePokemonSize READ getToEvolvePokemonSize CONSTANT)
  Q_PROPERTY(PokemonDBEntryEvolution* getToEvolvePokemonAt READ getToEvolvePokemonAt CONSTANT)
  Q_PROPERTY(int getToTeachPokemonSize READ getToTeachPokemonSize CONSTANT)
  Q_PROPERTY(PokemonDBEntryEvolution* getToTeachPokemonAt READ getToTeachPokemonAt CONSTANT)

public:
  const QString getName() const;
  int getInd() const;
  bool getOnce() const;
  bool getGlitch() const;
  const QString getReadable() const;
  int getTm() const;
  int getHm() const;
  int getPrice() const;
  int buyPriceMoney() const;
  int buyPriceCoins() const;
  int sellPriceMoney() const;
  int sellPriceCoins() const;
  bool canSell() const;
  bool isGameCornerExclusive() const;
  const MoveDBEntry* getToMove() const;
  const GameCornerDBEntry* getToGameCorner() const;

  const QVector<MapDBEntrySpriteItem*> getToMapSpriteItem() const;
  int getToMapSpriteItemSize() const;
  const MapDBEntrySpriteItem* getToMapSpriteItemAt(const int ind) const;

  const QVector<PokemonDBEntryEvolution*> getToEvolvePokemon() const;
  int getToEvolvePokemonSize() const;
  const PokemonDBEntryEvolution* getToEvolvePokemonAt(const int ind) const;

  const QVector<PokemonDBEntry*> getToTeachPokemon() const;
  int getToTeachPokemonSize() const;
  const PokemonDBEntry* getToTeachPokemonAt(const int ind) const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  ItemDBEntry();
  ItemDBEntry(const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  QString name = ""; // Item Output
  int ind = -1; // Item Code
  bool once = false; // Item can only be obtained once
  bool glitch = false; // Item is a glitch item
  QString readable = "";

  int tm = -1; // TM Number if present
  int hm = -1; // HM Number if present

  // Item Prices if available
  int price = -1;

  MoveDBEntry* toMove = nullptr; // To TM or HM Move
  GameCornerDBEntry* toGameCorner = nullptr;
  QVector<MapDBEntrySpriteItem*> toMapSpriteItem;
  QVector<PokemonDBEntryEvolution*> toEvolvePokemon;
  QVector<PokemonDBEntry*> toTeachPokemon;

  friend class ItemsDB;
  friend struct MapDBEntrySpriteItem;
};

#endif // ITEMDBENTRY_H
