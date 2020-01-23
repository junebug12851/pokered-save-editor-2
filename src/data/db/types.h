/*
  * Copyright 2019 June Hanabi
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
#ifndef TYPES_H
#define TYPES_H

#include <QJsonValue>
#include <QString>
#include <QHash>
#include <QVector>

#include "../../common/types.h"

struct MoveDBEntry;
struct PokemonDBEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All types in the game

struct TypeDBEntry {
  TypeDBEntry();
  TypeDBEntry(QJsonValue& data);

  QString name;
  var8 ind = 0;
  QString readable;

  QVector<MoveDBEntry*> toMoves;
  QVector<PokemonDBEntry*> toPokemon;
};

class TypesDB
{
public:
  static void load();
  static void index();

  static QVector<TypeDBEntry*> store;
  static QHash<QString, TypeDBEntry*> ind;
};

#endif // TYPES_H
