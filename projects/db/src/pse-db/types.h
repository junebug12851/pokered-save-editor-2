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
#include <QHash>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MoveDBEntry;
struct PokemonDBEntry;
class QQmlEngine;

struct DB_AUTOPORT TypeDBEntry {
  TypeDBEntry();
  TypeDBEntry(QJsonValue& data);

  QString name;
  var8 ind = 0;
  QString readable;

  QVector<MoveDBEntry*> toMoves;
  QVector<PokemonDBEntry*> toPokemon;
};

class DB_AUTOPORT TypesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static TypesDB* inst();

  [[nodiscard]] const QVector<TypeDBEntry*> getStore() const;
  [[nodiscard]] const QHash<QString, TypeDBEntry*> getInd() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE TypeDBEntry* getStoreAt(int idx) const;
  Q_INVOKABLE TypeDBEntry* getIndAt(const QString& key) const;

public slots:
  void load();
  void index();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  TypesDB();

  QVector<TypeDBEntry*> store;
  QHash<QString, TypeDBEntry*> ind;
};
