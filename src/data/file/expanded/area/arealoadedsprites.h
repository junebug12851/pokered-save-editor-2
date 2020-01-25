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
#ifndef AREALOADEDSPRITES_H
#define AREALOADEDSPRITES_H

#include <QObject>
#include <QVector>

#include "../../../../common/types.h"
class SaveFile;
struct MapDBEntry;
struct SpriteSetDBEntry;

constexpr var8 maxLoadedSprites = 11;

class AreaLoadedSprites : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 loadedSetId MEMBER loadedSetId NOTIFY loadedSetIdChanged)

public:
  AreaLoadedSprites(SaveFile* saveFile = nullptr);
  virtual ~AreaLoadedSprites();

  // Loaded sprites are a fixed size and cannot be moved, created, modified, or destroyed
  // They can be swapped
  Q_INVOKABLE var8 lSpriteCount();
  Q_INVOKABLE var8 lSpriteAt(var8 ind);
  Q_INVOKABLE void lSpriteSwap(var8 from, var8 to);

signals:
  void loadedSpritesChanged();
  void loadedSetIdChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(MapDBEntry* map, var8 x, var8 y);
  void loadSpriteSet(SpriteSetDBEntry* entry, var8 x, var8 y);

public:
  var8 loadedSprites[maxLoadedSprites];
  var8 loadedSetId;
};

#endif // AREALOADEDSPRITES_H
