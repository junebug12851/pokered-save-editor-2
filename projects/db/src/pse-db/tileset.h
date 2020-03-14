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
#ifndef TILESET_H
#define TILESET_H

#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntry;

enum class TilesetType
{
  INDOOR = 0,
  CAVE,
  OUTDOOR
};

// How many talk tiles are there in each entry
constexpr var8 talkCount = 3;

struct DB_AUTOPORT TilesetDBEntry {
  TilesetDBEntry();
  TilesetDBEntry(QJsonValue& data);

  QString name;
  QString type;
  QString nameAlias;
  QString typeAlias;

  TilesetType typeAsEnum();

  var8 ind = 0;
  var8 talk[3] = {0,0,0};
  var8 grass = 0;

  var8 bank = 0;
  var16 blockPtr = 0;
  var16 gfxPtr = 0;
  var16 collPtr = 0;

  QVector<MapDBEntry*> toMaps;
};

class DB_AUTOPORT TilesetDB
{
public:
  static void load();
  static void index();

  static QVector<TilesetDBEntry*> store;
  static QHash<QString, TilesetDBEntry*> ind;
};

#endif // TILESET_H
