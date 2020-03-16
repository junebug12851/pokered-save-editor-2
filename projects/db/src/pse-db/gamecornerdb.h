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

#include <QObject>
#include <QVector>
#include "./db_autoport.h"

class GameCornerDBEntry;
class QQmlEngine;

class DB_AUTOPORT GameCornerDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)
  Q_PROPERTY(int getBuyPrice READ getBuyPrice CONSTANT)

public:
  // Get Instance
  static GameCornerDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<GameCornerDBEntry*> getStore() const;
  int getStoreSize() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const GameCornerDBEntry* getStoreAt(const int ind) const;
  int getBuyPrice() const;

public slots:
  void load();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  GameCornerDB();

  // Buy and Sell Price
  // Pokedollars <=> Game Coins
  // Regular Casinos give you an even exchange, you get the exact amount back
  // But in the Poke-World I want to follow the global sell-back mechanics
  // whereby you get half back
  int buyPrice = 0;

  QVector<GameCornerDBEntry*> store;

  friend class GameCornerDBEntry;
};

#endif // GAMECORNERDB_H
