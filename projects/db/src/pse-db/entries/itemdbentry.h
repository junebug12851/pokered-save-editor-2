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


struct DB_AUTOPORT ItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName     READ getName     CONSTANT)
  Q_PROPERTY(int     getInd      READ getInd      CONSTANT)
  Q_PROPERTY(bool    getOnce     READ getOnce     CONSTANT)
  Q_PROPERTY(bool    getGlitch   READ getGlitch   CONSTANT)
  Q_PROPERTY(QString getReadable READ getReadable CONSTANT)
  Q_PROPERTY(int     getTm       READ getTm       CONSTANT)
  Q_PROPERTY(int     getHm       READ getHm       CONSTANT)
  Q_PROPERTY(int     getPrice    READ getPrice    CONSTANT)
  Q_PROPERTY(int     buyPriceMoney          READ buyPriceMoney          CONSTANT)
  Q_PROPERTY(int     buyPriceCoins          READ buyPriceCoins          CONSTANT)
  Q_PROPERTY(int     sellPriceMoney         READ sellPriceMoney         CONSTANT)
  Q_PROPERTY(int     sellPriceCoins         READ sellPriceCoins         CONSTANT)
  Q_PROPERTY(bool    canSell                READ canSell                CONSTANT)
  Q_PROPERTY(bool    isGameCornerExclusive  READ isGameCornerExclusive  CONSTANT)
  Q_PROPERTY(MoveDBEntry*      getToMove       READ getToMove       CONSTANT)
  Q_PROPERTY(GameCornerDBEntry* getToGameCorner READ getToGameCorner CONSTANT)
  // Indexed list properties exposed via size + Q_INVOKABLE accessor (not direct property)
  Q_PROPERTY(int getToMapSpriteItemSize   READ getToMapSpriteItemSize   CONSTANT)
  Q_PROPERTY(int getToEvolvePokemonSize   READ getToEvolvePokemonSize   CONSTANT)
  Q_PROPERTY(int getToTeachPokemonSize    READ getToTeachPokemonSize    CONSTANT)

public:
  QString getName()     const;
  int     getInd()      const;
  bool    getOnce()     const;
  bool    getGlitch()   const;
  QString getReadable() const;
  int     getTm()       const;
  int     getHm()       const;
  int     getPrice()    const;
  int     buyPriceMoney()         const;
  int     buyPriceCoins()         const;
  int     sellPriceMoney()        const;
  int     sellPriceCoins()        const;
  bool    canSell()               const;
  bool    isGameCornerExclusive() const;

  MoveDBEntry*       getToMove()       const;
  GameCornerDBEntry* getToGameCorner() const;

  const QVector<MapDBEntrySpriteItem*>    getToMapSpriteItem()   const;
  int                                     getToMapSpriteItemSize() const;
  Q_INVOKABLE MapDBEntrySpriteItem*       getToMapSpriteItemAt(int ind) const;

  const QVector<PokemonDBEntryEvolution*> getToEvolvePokemon()   const;
  int                                     getToEvolvePokemonSize() const;
  Q_INVOKABLE PokemonDBEntryEvolution*    getToEvolvePokemonAt(int ind) const;

  const QVector<PokemonDBEntry*>          getToTeachPokemon()    const;
  int                                     getToTeachPokemonSize() const;
  Q_INVOKABLE PokemonDBEntry*             getToTeachPokemonAt(int ind) const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  ItemDBEntry();
  ItemDBEntry(const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  QString name     = "";
  int     ind      = -1;
  bool    once     = false;
  bool    glitch   = false;
  QString readable = "";
  int     tm       = -1;
  int     hm       = -1;
  int     price    = -1;

  MoveDBEntry*       toMove       = nullptr;
  GameCornerDBEntry* toGameCorner = nullptr;
  QVector<MapDBEntrySpriteItem*>    toMapSpriteItem;
  QVector<PokemonDBEntryEvolution*> toEvolvePokemon;
  QVector<PokemonDBEntry*>          toTeachPokemon;

  friend class ItemsDB;
  friend struct MapDBEntrySpriteItem;
  friend struct GameCornerDBEntry; // writes toGameCorner in GameCornerDBEntry::deepLink
  friend struct PokemonDBEntry;          // writes toTeachPokemon in PokemonDBEntry::deepLink
  friend struct PokemonDBEntryEvolution; // writes toEvolvePokemon in PokemonDBEntryEvolution::deepLink
};
