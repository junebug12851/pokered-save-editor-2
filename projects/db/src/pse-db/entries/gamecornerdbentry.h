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
#ifndef GAMECORNERDBENTRY_H
#define GAMECORNERDBENTRY_H

#include <QObject>
#include <QString>
#include <QJsonValue>

#include "../db_autoport.h"

class PokemonDBEntry;
class ItemDBEntry;
class QQmlEngine;
class GameCornerDB;

struct DB_AUTOPORT GameCornerDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(QString getType READ getType CONSTANT)
  Q_PROPERTY(int getPrice READ getPrice CONSTANT)
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT)
  Q_PROPERTY(ItemDBEntry* getToItem READ getToItem CONSTANT)

public:
  const QString getName() const;
  const QString getType() const;
  int getPrice() const;
  int getLevel() const;
  const PokemonDBEntry* getToPokemon() const;
  const ItemDBEntry* getToItem() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  GameCornerDBEntry();
  GameCornerDBEntry(const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  QString name = "";
  QString type = "";
  int price = 0;
  int level;

  PokemonDBEntry* toPokemon = nullptr;
  ItemDBEntry* toItem = nullptr;

  friend class GameCornerDB;
};

#endif // GAMECORNERDBENTRY_H
