/*
  * Copyright 2020 Fairy Fox
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
#include <QVector>
#include "./db_autoport.h"

class GameCornerDBEntry;
class QQmlEngine;

/**
 * @brief The Game Corner database -- prize entries plus the coin exchange rate.
 *
 * Standard DB-singleton (see CreditsDB / db.md) with two extra rate properties:
 * @ref getBuyPrice / @ref getSellPrice convert between Pokedollars and Game Coins.
 * Per the note below, the sell-back follows the project's "half back" policy rather
 * than an even exchange. The entry type is in `entries/gamecornerdbentry.h`.
 *
 * @see GameCornerDBEntry, DB.
 */
class DB_AUTOPORT GameCornerDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of prize entries.
  Q_PROPERTY(int getBuyPrice READ getBuyPrice CONSTANT)   ///< Coins-per-Pokedollar buy rate.
  Q_PROPERTY(int getSellPrice READ getSellPrice CONSTANT) ///< Sell-back rate (half of buy; see note).

public:
  // Get Instance
  static GameCornerDB* inst(); ///< The process-wide GameCornerDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<GameCornerDBEntry*> getStore() const; ///< All prize entries.
  int getStoreSize() const;                           ///< Prize count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE GameCornerDBEntry* getStoreAt(const int ind) const; ///< Prize by store index (for QML).
  int getBuyPrice() const;  ///< The buy rate (backs @c getBuyPrice).
  int getSellPrice() const; ///< The sell rate (half the buy rate).

public slots:
  void load();     ///< Load prizes from JSON.
  void deepLink(); ///< Resolve each prize's item/Pokemon links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  GameCornerDB(); ///< Private -- use inst().

  QVector<GameCornerDBEntry*> store; ///< The loaded prize entries.

  // Buy and Sell Price
  // Pokedollars <=> Game Coins
  // Regular Casinos give you an even exchange, you get the exact amount back
  // But in the Poke-World I want to follow the global sell-back mechanics
  // whereby you get half back
  int buyPrice = 0; ///< Buy rate; sell rate is half of this (see note above).

  friend class GameCornerDBEntry; ///< Lets entries populate the store/price during load.
};
