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
#ifndef AREASPRITES_H
#define AREASPRITES_H

#include <QObject>
#include <QVector>
#include <pse-common/types.h>

class SaveFile;
class SpriteData;
class MapDBEntry;
class MapDBEntrySprite;

constexpr var8 maxSprites = 16;

class AreaSprites : public QObject
{
  Q_OBJECT

public:
  AreaSprites(SaveFile* saveFile = nullptr);
  virtual ~AreaSprites();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int spriteCount();
  Q_INVOKABLE int spriteMax();
  Q_INVOKABLE SpriteData* spriteAt(int ind);
  Q_INVOKABLE void spriteSwap(int from, int to);
  Q_INVOKABLE void spriteRemove(int ind);
  Q_INVOKABLE void spriteNew();

signals:
  void spritesChanged();

public slots:
  void reset();
  void randomize(QVector<MapDBEntrySprite*> spriteData);
  void setTo(MapDBEntry* map);

public:
  QVector<SpriteData*> sprites;
};

#endif // AREASPRITES_H
