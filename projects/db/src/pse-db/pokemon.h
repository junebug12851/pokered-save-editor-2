/*
  * Copyright 2019 Twilight
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
#pragma once

#include <QObject>
#include <QJsonValue>
#include <QString>
#include <QVector>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

constexpr var8 pokemonDexCount = 151;
constexpr var8 pokemonLevelMax = 100;

// Forward declarations
struct PokemonDBEntryEvolution;
struct MoveDBEntry;
struct PokemonDBEntry;
struct ItemDBEntry;
struct TypeDBEntry;
struct EventPokemonDBEntry;
struct MapDBEntrySpritePokemon;
struct MapDBEntryWildMon;
struct TradeDBEntry;
struct GameCornerDBEntry;
class QQmlEngine;

struct DB_AUTOPORT PokemonDBEntryEvolution
{
  PokemonDBEntryEvolution();
  PokemonDBEntryEvolution(QJsonValue& data, PokemonDBEntry* parent);
  void deepLink(PokemonDBEntry* deEvolution);

  QString toName;
  bool trade = false;
  QString item;
  std::optional<var8> level;

  PokemonDBEntry* toDeEvolution = nullptr;
  PokemonDBEntry* toEvolution   = nullptr;
  ItemDBEntry*    toItem        = nullptr;
  PokemonDBEntry* parent        = nullptr;
};

struct DB_AUTOPORT PokemonDBEntryMove
{
  PokemonDBEntryMove();
  PokemonDBEntryMove(QJsonValue& data, PokemonDBEntry* parent);
  void deepLink();

  var8 level = 0;
  QString move;

  MoveDBEntry*    toMove  = nullptr;
  PokemonDBEntry* parent  = nullptr;
};

struct DB_AUTOPORT PokemonDBEntry {
  PokemonDBEntry();
  PokemonDBEntry(QJsonValue& data);
  void deepLink();

  QString name;
  var8    ind = 0;
  QString readable;
  bool    glitch = false;
  QString type1;
  QString type2;

  QVector<PokemonDBEntryMove*>      moves;
  QVector<QString>                  initial;
  QVector<var8>                     tmHm;
  QVector<PokemonDBEntryEvolution*> evolution;

  std::optional<var8> pokedex;
  std::optional<var8> growthRate;
  std::optional<var8> baseHp;
  std::optional<var8> baseAttack;
  std::optional<var8> baseDefense;
  std::optional<var8> baseSpeed;
  std::optional<var8> baseSpecial;
  std::optional<var8> baseExpYield;
  std::optional<var8> catchRate;

  TypeDBEntry*          toType1             = nullptr;
  TypeDBEntry*          toType2             = nullptr;
  PokemonDBEntry*       toDeEvolution       = nullptr;
  QVector<MoveDBEntry*> toInitial;
  QVector<MoveDBEntry*> toTmHmMove;
  QVector<ItemDBEntry*> toTmHmItem;
  QVector<EventPokemonDBEntry*>     toEventMons;
  MapDBEntrySpritePokemon*          toMapSpritePokemon = nullptr;
  QVector<MapDBEntryWildMon*>       toWildMonMaps;
  QVector<TradeDBEntry*>            toTrades;
  QVector<GameCornerDBEntry*>       toGameCorner;
};

class DB_AUTOPORT PokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static PokemonDB* inst();

  [[nodiscard]] const QVector<PokemonDBEntry*> getStore() const;
  [[nodiscard]] const QHash<QString, PokemonDBEntry*> getInd() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE PokemonDBEntry* getStoreAt(int idx) const;
  Q_INVOKABLE PokemonDBEntry* getIndAt(const QString& key) const;

public slots:
  void load();
  void index();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  PokemonDB();

  QVector<PokemonDBEntry*>      store;
  QHash<QString, PokemonDBEntry*> ind;
};
