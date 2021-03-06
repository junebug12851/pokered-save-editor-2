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
#include "./areasprites.h"
#include "../fragments/spritedata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>
#include <pse-db/sprites.h>

#include <QRandomGenerator>

AreaSprites::AreaSprites(SaveFile* saveFile)
{
  // By default add player sprite
  sprites.append(new SpriteData(false));
  load(saveFile);
}

AreaSprites::~AreaSprites()
{
  for(auto entry : sprites)
    entry->deleteLater();
}

int AreaSprites::spriteCount()
{
  return sprites.size();
}

int AreaSprites::spriteMax()
{
  return maxSprites;
}

SpriteData* AreaSprites::spriteAt(int ind)
{
  return sprites.at(ind);
}

void AreaSprites::spriteSwap(int from, int to)
{
  auto eFrom = sprites.at(from);
  auto eTo = sprites.at(to);

  sprites.replace(from, eTo);
  sprites.replace(to, eFrom);

  spritesChanged();
}

void AreaSprites::spriteRemove(int ind)
{
  // A sprite has to always have a player sprite in the first position
  if(sprites.size() <= 1)
    return;

  sprites.at(ind)->deleteLater();
  sprites.removeAt(ind);
  spritesChanged();
}

void AreaSprites::spriteNew()
{
  if(sprites.size() >= maxSprites)
    return;

  sprites.append(new SpriteData);
  spritesChanged();
}

void AreaSprites::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Total sprite count including player
  var8 spriteCount = toolset->getByte(0x278D) + 1;

  for (var8 i = 0; i < spriteCount && i < 16; i++) {
    sprites.append(new SpriteData(false, saveFile, i));
  }

  spritesChanged();
}

void AreaSprites::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  // Save sprite count minus player
  toolset->setByte(0x278D, sprites.size() - 1);
  for (var8 i = 0; i < sprites.size() && i < 16; i++) {
    sprites.at(i)->save(saveFile, i);
  }

  SpriteData::saveMissables(saveFile, sprites);
}

void AreaSprites::reset()
{
  for(auto entry : sprites)
    entry->deleteLater();

  sprites.clear();
  spritesChanged();
}

void AreaSprites::randomize(QVector<MapDBEntrySprite*> spriteData)
{
  reset();
  sprites = SpriteData::randomizeAll(spriteData);
  spritesChanged();
}

void AreaSprites::setTo(MapDBEntry* map)
{
  reset();

  if(map == nullptr)
    return;

  sprites = SpriteData::setToAll(map->sprites);
  spritesChanged();
}
