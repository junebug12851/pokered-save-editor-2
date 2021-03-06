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
#ifndef SPRITE_H
#define SPRITE_H

#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntrySprite;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All sprites in the game, glitch and not

struct DB_AUTOPORT SpriteDBEntry {
  SpriteDBEntry();
  SpriteDBEntry(QJsonValue& data);

  QString name;
  var8 ind = 0;

  QVector<MapDBEntrySprite*> toMaps;
};

class DB_AUTOPORT SpritesDB
{
public:
  static void load();
  static void index();

  static QVector<SpriteDBEntry*> store;
  static QHash<QString, SpriteDBEntry*> ind;
};

#endif // SPRITE_H
