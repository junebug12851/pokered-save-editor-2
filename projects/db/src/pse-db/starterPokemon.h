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
#include <QString>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;

// Hand-curated list of good starter choices.
// Rules: base evolution (if one exists), non-legendary, feels "startery".
class DB_AUTOPORT StarterPokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static StarterPokemonDB* inst();

  [[nodiscard]] int getStoreSize() const;

  // First 3 entries are the canonical in-game starters.
  Q_INVOKABLE PokemonDBEntry* random3Starter() const;
  Q_INVOKABLE PokemonDBEntry* randomAnyStarter() const;

public slots:
  void load();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  StarterPokemonDB();

  QVector<QString>        store;
  QVector<PokemonDBEntry*> toPokemon;
};
