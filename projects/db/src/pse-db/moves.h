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
#include <QVector>
#include <QString>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct TypeDBEntry;
struct ItemDBEntry;
struct PokemonDBEntryMove;
struct PokemonDBEntry;
class QQmlEngine;

struct DB_AUTOPORT MoveDBEntry {
  MoveDBEntry();
  MoveDBEntry(QJsonValue& data);
  void deepLink();

  QString name;
  var8 ind = 0;
  bool glitch = false;
  QString type;
  QString readable;

  std::optional<var8> power;
  std::optional<var8> accuracy;
  std::optional<var8> pp;
  std::optional<var8> tm;
  std::optional<var8> hm;

  TypeDBEntry* toType  = nullptr;
  ItemDBEntry* toItem  = nullptr;

  // Back-references populated during PokemonDB deep-link
  QVector<struct PokemonDBEntryMove*> toPokemonLearned;
  QVector<struct PokemonDBEntry*>     toPokemonInitial;
  QVector<struct PokemonDBEntry*>     toPokemonTmHm;
};

class DB_AUTOPORT MovesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static MovesDB* inst();

  [[nodiscard]] const QVector<MoveDBEntry*> getStore() const;
  [[nodiscard]] const QHash<QString, MoveDBEntry*> getInd() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE MoveDBEntry* getStoreAt(int idx) const;
  Q_INVOKABLE MoveDBEntry* getIndAt(const QString& key) const;

public slots:
  void load();
  void index();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  MovesDB();

  QVector<MoveDBEntry*> store;
  QHash<QString, MoveDBEntry*> ind;
};
