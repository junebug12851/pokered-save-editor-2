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
#ifndef MOVE_H
#define MOVE_H

#include "../common/types.h"
#include <optional>
#include <QString>
#include <QHash>

#include "./types.h"

// Prevents includes from including each other and causing errors
// We include them in the cpp file
struct ItemEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All the Pokemon moves in the game including special or glitch moves

struct MoveEntry {

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  MoveEntry();

  QString name;
  var8 ind;
  bool glitch;
  QString type;
  QString readable;

  std::optional<var8> power;
  std::optional<var8> accuracy;
  std::optional<var8> pp;
  std::optional<var8> tm;
  std::optional<var8> hm;

  TypeEntry* toType; // Deep link to move type
  ItemEntry* toItem; // Deep link to tm/hm item if present
};

class Moves
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<MoveEntry*>* moves;
  static QHash<QString, MoveEntry*>* ind;
};

#endif // MOVE_H
