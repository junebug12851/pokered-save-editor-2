/*
  * Copyright 2020 Twilight
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
#include <QString>
#include <QVector>
#include <QJsonValue>
#include "../db_autoport.h"

// as property types without requiring full includes (avoids circular deps).
struct MoveDBEntry;
struct MapDBEntrySpriteItem;
struct PokemonDBEntryEvolution;
struct PokemonDBEntry;
struct GameCornerDBEntry;
class QQmlEngine;
class ItemsDB;


/**
 * @brief One item's static data: name/flags, pricing, and where it's used.
 *
 * QObject-getter style DB entry. Beyond the basic fields it computes the full
 * pricing surface (buy/sell, money/coins, plus @ref canSell /
 * @ref isGameCornerExclusive) and exposes several cross-reference @e lists -- the
 * map sprites that drop it, the evolutions that use it, and the Pokemon it teaches.
 * Because Qt can't expose a vector as a property, each list is surfaced as a
 * `...Size` property plus an invokable `...At(ind)` accessor (the standard
 * workaround). Each `friend` writes its respective back-reference during deepLink.
 *
 * @see ItemsDB, Item (the save-side inventory slot), db.md.
 */
struct DB_AUTOPORT ItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName     READ getName     CONSTANT) ///< Internal item name.
  Q_PROPERTY(int     getInd      READ getInd      CONSTANT) ///< Item index.
  Q_PROPERTY(bool    getOnce     READ getOnce     CONSTANT) ///< One-time-only item.
  Q_PROPERTY(bool    getGlitch   READ getGlitch   CONSTANT) ///< Glitch item.
  Q_PROPERTY(QString getReadable READ getReadable CONSTANT) ///< Display name.
  Q_PROPERTY(int     getTm       READ getTm       CONSTANT) ///< TM number, if a TM.
  Q_PROPERTY(int     getHm       READ getHm       CONSTANT) ///< HM number, if an HM.
  Q_PROPERTY(int     getPrice    READ getPrice    CONSTANT) ///< Base price.
  Q_PROPERTY(QString getInfo     READ getInfo     CONSTANT) ///< Detailed-tooltip lore: what it is / how it's obtained.
  Q_PROPERTY(int     buyPriceMoney          READ buyPriceMoney          CONSTANT) ///< Buy price in money.
  Q_PROPERTY(int     buyPriceCoins          READ buyPriceCoins          CONSTANT) ///< Buy price in coins.
  Q_PROPERTY(int     sellPriceMoney         READ sellPriceMoney         CONSTANT) ///< Sell price in money.
  Q_PROPERTY(int     sellPriceCoins         READ sellPriceCoins         CONSTANT) ///< Sell price in coins.
  Q_PROPERTY(bool    canSell                READ canSell                CONSTANT) ///< Whether it can be sold.
  Q_PROPERTY(bool    isGameCornerExclusive  READ isGameCornerExclusive  CONSTANT) ///< Coins-only (Game Corner) item.
  Q_PROPERTY(MoveDBEntry*      getToMove       READ getToMove       CONSTANT) ///< Move taught (if a TM/HM).
  Q_PROPERTY(GameCornerDBEntry* getToGameCorner READ getToGameCorner CONSTANT) ///< Game Corner prize entry, if any.
  // Indexed list properties exposed via size + Q_INVOKABLE accessor (not direct property)
  Q_PROPERTY(int getToMapSpriteItemSize   READ getToMapSpriteItemSize   CONSTANT) ///< Count of map sprites dropping this.
  Q_PROPERTY(int getToEvolvePokemonSize   READ getToEvolvePokemonSize   CONSTANT) ///< Count of evolutions using this.
  Q_PROPERTY(int getToTeachPokemonSize    READ getToTeachPokemonSize    CONSTANT) ///< Count of Pokemon it teaches.

public:
  QString getName()     const; ///< @see getName property.
  int     getInd()      const; ///< @see getInd property.
  bool    getOnce()     const; ///< @see getOnce property.
  bool    getGlitch()   const; ///< @see getGlitch property.
  QString getReadable() const; ///< @see getReadable property.
  int     getTm()       const; ///< @see getTm property.
  int     getHm()       const; ///< @see getHm property.
  int     getPrice()    const; ///< @see getPrice property.
  QString getInfo()     const; ///< @see getInfo property.
  int     buyPriceMoney()         const; ///< @see buyPriceMoney property.
  int     buyPriceCoins()         const; ///< @see buyPriceCoins property.
  int     sellPriceMoney()        const; ///< @see sellPriceMoney property.
  int     sellPriceCoins()        const; ///< @see sellPriceCoins property.
  bool    canSell()               const; ///< @see canSell property.
  bool    isGameCornerExclusive() const; ///< @see isGameCornerExclusive property.

  MoveDBEntry*       getToMove()       const; ///< @see getToMove property.
  GameCornerDBEntry* getToGameCorner() const; ///< @see getToGameCorner property.

  const QVector<MapDBEntrySpriteItem*>    getToMapSpriteItem()   const; ///< Map sprites that drop this item.
  int                                     getToMapSpriteItemSize() const; ///< @see getToMapSpriteItemSize property.
  Q_INVOKABLE MapDBEntrySpriteItem*       getToMapSpriteItemAt(int ind) const; ///< Map-sprite-item @p ind (for QML).

  const QVector<PokemonDBEntryEvolution*> getToEvolvePokemon()   const; ///< Evolutions triggered by this item.
  int                                     getToEvolvePokemonSize() const; ///< @see getToEvolvePokemonSize property.
  Q_INVOKABLE PokemonDBEntryEvolution*    getToEvolvePokemonAt(int ind) const; ///< Evolution @p ind (for QML).

  const QVector<PokemonDBEntry*>          getToTeachPokemon()    const; ///< Pokemon this TM/HM teaches.
  int                                     getToTeachPokemonSize() const; ///< @see getToTeachPokemonSize property.
  Q_INVOKABLE PokemonDBEntry*             getToTeachPokemonAt(int ind) const; ///< Taught Pokemon @p ind (for QML).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  ItemDBEntry();                    ///< Empty entry (built by ItemsDB).
  ItemDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                  ///< Resolve the move/Game-Corner links.
  void qmlRegister() const;         ///< Register with QML.

  QString name     = "";  ///< Backing field (read via getName()).
  int     ind      = -1;  ///< Backing field (read via getInd()).
  bool    once     = false; ///< Backing field (read via getOnce()).
  bool    glitch   = false; ///< Backing field (read via getGlitch()).
  QString readable = "";  ///< Backing field (read via getReadable()).
  int     tm       = -1;  ///< Backing field (read via getTm()).
  int     hm       = -1;  ///< Backing field (read via getHm()).
  int     price    = -1;  ///< Backing field (read via getPrice()).
  QString info     = "";  ///< Backing field (read via getInfo()); optional "info" in items.json.

  MoveDBEntry*       toMove       = nullptr; ///< Resolved taught move (deepLink).
  GameCornerDBEntry* toGameCorner = nullptr; ///< Resolved Game Corner entry (deepLink).
  QVector<MapDBEntrySpriteItem*>    toMapSpriteItem; ///< Map sprites dropping this (back-ref).
  QVector<PokemonDBEntryEvolution*> toEvolvePokemon; ///< Evolutions using this (back-ref).
  QVector<PokemonDBEntry*>          toTeachPokemon;  ///< Pokemon this teaches (back-ref).

  friend class ItemsDB;                  ///< Owning DB constructs/populates entries.
  friend struct MapDBEntrySpriteItem;    ///< Writes toMapSpriteItem.
  friend struct GameCornerDBEntry; // writes toGameCorner in GameCornerDBEntry::deepLink
  friend struct PokemonDBEntry;          // writes toTeachPokemon in PokemonDBEntry::deepLink
  friend struct PokemonDBEntryEvolution; // writes toEvolvePokemon in PokemonDBEntryEvolution::deepLink
};
