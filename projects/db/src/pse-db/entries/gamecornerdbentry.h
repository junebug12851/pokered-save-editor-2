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
#include <QJsonValue>
#include "../db_autoport.h"

struct PokemonDBEntry;
struct ItemDBEntry;
class QQmlEngine;
class GameCornerDB;


/**
 * @brief One Game Corner prize: a Pokemon or item, its coin price, and level.
 *
 * QObject-getter style DB entry. @ref type distinguishes a Pokemon prize from an
 * item prize; deepLink() resolves whichever applies (@ref toPokemon / @ref toItem).
 * See db.md.
 *
 * @see GameCornerDB, ItemDBEntry, PokemonDBEntry.
 */
struct DB_AUTOPORT GameCornerDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString        getName      READ getName      CONSTANT) ///< Prize name.
  Q_PROPERTY(QString        getType      READ getType      CONSTANT) ///< Prize type (pokemon/item).
  Q_PROPERTY(int            getPrice     READ getPrice     CONSTANT) ///< Coin price.
  Q_PROPERTY(int            getLevel     READ getLevel     CONSTANT) ///< Level (for Pokemon prizes).
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT) ///< Resolved Pokemon prize.
  Q_PROPERTY(ItemDBEntry*   getToItem    READ getToItem    CONSTANT) ///< Resolved item prize.

public:
  QString getName()       const; ///< @see getName property.
  QString getType()       const; ///< @see getType property.
  int getPrice()          const; ///< @see getPrice property.
  int getLevel()          const; ///< @see getLevel property.
  PokemonDBEntry* getToPokemon() const; ///< @see getToPokemon property.
  ItemDBEntry*    getToItem()    const; ///< @see getToItem property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  GameCornerDBEntry();                    ///< Empty entry (built by GameCornerDB).
  GameCornerDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                        ///< Resolve the Pokemon/item link.
  void qmlRegister() const;               ///< Register with QML.

  QString name = "";  ///< Backing field (read via getName()).
  QString type = "";  ///< Backing field (read via getType()).
  int price = 0;      ///< Backing field (read via getPrice()).
  int level = 0;      ///< Backing field (read via getLevel()).
  PokemonDBEntry* toPokemon = nullptr; ///< Resolved Pokemon prize (deepLink).
  ItemDBEntry*    toItem    = nullptr; ///< Resolved item prize (deepLink).

  friend class GameCornerDB; ///< Owning DB constructs/populates entries.
};
