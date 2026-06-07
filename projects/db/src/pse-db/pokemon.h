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

constexpr var8 pokemonDexCount = 151; ///< Number of species.
constexpr var8 pokemonLevelMax = 100; ///< Maximum level.

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

/**
 * @brief One evolution edge of a species: how it evolves (and de-evolves).
 *
 * Sub-entry of PokemonDBEntry. Records the target (@ref toName) and the trigger
 * (@ref level, @ref trade, or @ref item). deepLink() resolves the forward
 * (@ref toEvolution), backward (@ref toDeEvolution), and item links. See db.md.
 */
struct DB_AUTOPORT PokemonDBEntryEvolution
{
  PokemonDBEntryEvolution();                                      ///< Empty edge.
  PokemonDBEntryEvolution(QJsonValue& data, PokemonDBEntry* parent); ///< Build from JSON under @p parent.
  void deepLink(PokemonDBEntry* deEvolution);                     ///< Resolve target/item; set @p deEvolution back-link.

  QString toName;            ///< Name of the species this evolves into.
  bool trade = false;        ///< Evolves on trade.
  QString item;              ///< Evolution item, if any.
  std::optional<var8> level; ///< Evolution level, if level-based.

  PokemonDBEntry* toDeEvolution = nullptr; ///< Resolved pre-evolution.
  PokemonDBEntry* toEvolution   = nullptr; ///< Resolved evolved species.
  ItemDBEntry*    toItem        = nullptr; ///< Resolved evolution item.
  PokemonDBEntry* parent        = nullptr; ///< Owning species.
};

/**
 * @brief One learnable move of a species, with the level it's learned at.
 *
 * Sub-entry of PokemonDBEntry. deepLink() resolves @ref toMove. See db.md.
 */
struct DB_AUTOPORT PokemonDBEntryMove
{
  PokemonDBEntryMove();                                      ///< Empty learn entry.
  PokemonDBEntryMove(QJsonValue& data, PokemonDBEntry* parent); ///< Build from JSON under @p parent.
  void deepLink();                                          ///< Resolve the move link.

  var8 level = 0; ///< Level the move is learned at.
  QString move;   ///< Move name (resolved to @ref toMove).

  MoveDBEntry*    toMove  = nullptr; ///< Resolved move.
  PokemonDBEntry* parent  = nullptr; ///< Owning species.
};

/**
 * @brief One species' complete static data -- the richest entry in the db layer.
 *
 * Holds the base stats, types, learnset (@ref moves / @ref initial / @ref tmHm),
 * and @ref evolution edges, all loaded from JSON. deepLink() then wires up a large
 * web of cross-references (the `to*` members): types, learnable moves, TM/HM
 * moves+items, plus back-references to where the species appears -- events, on-map
 * sprites, wild-encounter maps, trades, and Game Corner prizes. The `std::optional`
 * base-stat fields are absent for entries that don't define them (e.g. glitch mons).
 *
 * @see PokemonDB, PokemonBox (the save-side mon), db.md for the entry/deepLink convention.
 */
struct DB_AUTOPORT PokemonDBEntry {
  PokemonDBEntry();                ///< Empty species.
  PokemonDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                 ///< Resolve the full cross-reference web (the `to*` members).

  QString name;          ///< Internal species name (key).
  var8    ind = 0;       ///< Internal species index.
  QString readable;      ///< Human-readable species name.
  bool    glitch = false; ///< Whether this is a glitch species.
  QString type1;         ///< Primary type name (resolved to @ref toType1).
  QString type2;         ///< Secondary type name (resolved to @ref toType2).

  QVector<PokemonDBEntryMove*>      moves;     ///< Level-up learnset.
  QVector<QString>                  initial;   ///< Moves known at capture (resolved to @ref toInitial).
  QVector<var8>                     tmHm;      ///< TM/HM numbers it can learn.
  QVector<PokemonDBEntryEvolution*> evolution; ///< Evolution edges.

  std::optional<var8> pokedex;      ///< Pokedex number, if assigned.
  std::optional<var8> growthRate;   ///< EXP growth-rate group.
  std::optional<var8> baseHp;       ///< Base HP.
  std::optional<var8> baseAttack;   ///< Base Attack.
  std::optional<var8> baseDefense;  ///< Base Defense.
  std::optional<var8> baseSpeed;    ///< Base Speed.
  std::optional<var8> baseSpecial;  ///< Base Special.
  std::optional<var8> baseExpYield; ///< Base EXP yield.
  std::optional<var8> catchRate;    ///< Catch rate.

  TypeDBEntry*          toType1             = nullptr; ///< Resolved primary type.
  TypeDBEntry*          toType2             = nullptr; ///< Resolved secondary type.
  PokemonDBEntry*       toDeEvolution       = nullptr; ///< Resolved pre-evolution.
  QVector<MoveDBEntry*> toInitial;                     ///< Resolved capture moves.
  QVector<MoveDBEntry*> toTmHmMove;                    ///< Resolved TM/HM moves.
  QVector<ItemDBEntry*> toTmHmItem;                    ///< Resolved TM/HM items.
  QVector<EventPokemonDBEntry*>     toEventMons;       ///< Event distributions of this species.
  MapDBEntrySpritePokemon*          toMapSpritePokemon = nullptr; ///< On-map static-Pokemon sprite, if any.
  QVector<MapDBEntryWildMon*>       toWildMonMaps;     ///< Maps where it appears wild.
  QVector<TradeDBEntry*>            toTrades;          ///< In-game trades giving/getting it.
  QVector<GameCornerDBEntry*>       toGameCorner;      ///< Game Corner prizes of this species.
};

/**
 * @brief The Pokemon database -- all 151 species, keyed by name.
 *
 * Standard DB-singleton with a name index and the (heaviest) deepLink() pass. See
 * CreditsDB / db.md.
 *
 * @see PokemonDBEntry, DB.
 */
class DB_AUTOPORT PokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of species.

public:
  static PokemonDB* inst(); ///< The process-wide PokemonDB singleton.

  [[nodiscard]] const QVector<PokemonDBEntry*> getStore() const;       ///< All species.
  [[nodiscard]] const QHash<QString, PokemonDBEntry*> getInd() const;  ///< Name->species index.
  [[nodiscard]] int getStoreSize() const;                            ///< Species count.

  Q_INVOKABLE PokemonDBEntry* getStoreAt(int idx) const;            ///< Species by store index (for QML).
  Q_INVOKABLE PokemonDBEntry* getIndAt(const QString& key) const;   ///< Species by name key (for QML).

public slots:
  void load();     ///< Load species from JSON.
  void index();    ///< Build the name->species index.
  void deepLink(); ///< Resolve every species' cross-reference web.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  PokemonDB(); ///< Private -- use inst().

  QVector<PokemonDBEntry*>      store;     ///< The loaded species.
  QHash<QString, PokemonDBEntry*> ind;     ///< Name->species lookup.
};
