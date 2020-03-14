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

  Q_PROPERTY(int loadedSetId MEMBER loadedSetId NOTIFY loadedSetIdChanged)

public:
  AreaLoadedSprites(SaveFile* saveFile = nullptr);
  virtual ~AreaLoadedSprites();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void loadSpriteSet(SpriteSetDBEntry* entry, int x, int y);

  // Loaded sprites are a fixed size and cannot be moved, created, modified, or destroyed
  // They can be swapped
  Q_INVOKABLE int lSpriteCount();
  Q_INVOKABLE int lSpriteAt(int ind);
  Q_INVOKABLE void lSpriteSwap(int from, int to);

signals:
  void loadedSpritesChanged();
  void loadedSetIdChanged();

public slots:
  void reset();
  void randomize(MapDBEntry* map, int x, int y);
  void setTo(MapDBEntry* map, int x, int y);

public:
  var8 loadedSprites[maxLoadedSprites];
  int loadedSetId;
};

#endif // AREALOADEDSPRITES_H
