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
#ifndef MAP_H
#define MAP_H

#include "../common/types.h"
#include "optional"
#include <QString>
#include <QHash>

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Details on all the maps in the game

struct MapEntry {

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  MapEntry();

  QString name;
  var8 ind;

  bool glitch;
  bool special;

  std::optional<var8> bank;
  std::optional<var16> dataPtr;
  std::optional<var16> scriptPtr;
  std::optional<var16> textPtr;
  std::optional<var8> width;
  std::optional<var8> height;

  // These have been removed from the JSON data because they are simply
  // dimensions times 2 and thus redundant and repetitive to inlclude in JSON
  std::optional<var8> height2X2();
  std::optional<var8> width2X2();
};

class Maps
{
public:
  static void load();
  static void index();

  static QVector<MapEntry*>* maps;
  static QHash<QString, MapEntry*>* ind;
};

#endif // MAP_H
