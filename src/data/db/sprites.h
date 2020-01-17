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

#include <QVector>
#include <QString>
#include <QHash>

#include "../../common/types.h"

struct MapDBEntrySprite;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All sprites in the game, glitch and not

struct SpriteDBEntry {
  QString name;
  var8 ind;

  QVector<MapDBEntrySprite*> toMaps;
};

class SpritesDB
{
public:
  static void load();
  static void index();

  static QVector<SpriteDBEntry*> store;
  static QHash<QString, SpriteDBEntry*> ind;
};

#endif // SPRITE_H
