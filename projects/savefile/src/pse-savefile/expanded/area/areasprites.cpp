/*
  * Copyright 2020 Fairy Fox
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

/**
 * @file areasprites.cpp
 * @brief Implementation of AreaSprites -- the map's list of sprites/NPCs.
 *        See areasprites.h for the documented API.
 */
#include "./areasprites.h"
#include "../../qmlownership.h"
#include "../fragments/spritedata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>
#include <pse-db/sprites.h>
#include <pse-db/entries/mapdbentry.h>

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
  return qmlCppOwned(sprites.at(ind));
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

  // Never remove the player (slot 0) -- the game requires him there.
  if(ind <= 0 || ind >= sprites.size())
    return;

  // The 16 slots are an ordered array and the game packs them, so removing one SLIDES THE
  // REST UP. That is what the console does and what project leadership asked for.
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

int AreaSprites::spriteAdd(int pictureID, int x, int y)
{
  if(sprites.size() >= maxSprites)
    return -1;   // 15 NPCs plus the player. The caller says so before the user hits it.

  // A blank NPC: the four NPC-table optionals engaged, and the game's own sane defaults
  // (movementStatus Ready, imageIndex $FF, yDisp/xDisp 8, STAY, not in grass).
  auto sprite = new SpriteData(true);

  sprite->pictureID = pictureID;
  sprite->pictureIDChanged();

  sprite->pictureIDCopy = pictureID;   // the game keeps a second copy
  sprite->pictureIDCopyChanged();

  // Map coordinates carry the game's +4 bias ("the topmost 2x2 tile has value 4").
  sprite->mapX = x + 4;
  sprite->mapXChanged();

  sprite->mapY = y + 4;
  sprite->mapYChanged();

  sprites.append(sprite);
  spritesChanged();

  return sprites.size() - 1;
}

void AreaSprites::spriteMove(int ind, int x, int y)
{
  if(ind < 0 || ind >= sprites.size())
    return;

  auto sprite = sprites.at(ind);

  // Exactly two bytes, both carrying the game's +4 bias. Nothing else is touched -- not the
  // pixel positions, not the animation scratch. The game recomputes those from these.
  sprite->mapX = x + 4;
  sprite->mapXChanged();

  sprite->mapY = y + 4;
  sprite->mapYChanged();

  spritesChanged();
}

void AreaSprites::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Total sprite count including player
  // (wNumSprites counts NPCs only -- the player is slot 0 and isn't in it.)
  var8 spriteCount = toolset->getByte(0x278D) + 1;

  for (var8 i = 0; i < spriteCount && i < maxSprites; i++) {
    sprites.append(new SpriteData(false, saveFile, i));
  }

  spritesChanged();
}

void AreaSprites::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  // Save sprite count minus player
  toolset->setByte(0x278D, sprites.size() - 1);
  for (var8 i = 0; i < sprites.size() && i < maxSprites; i++) {
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

  sprites = SpriteData::setToAll(map->getSprites());
  spritesChanged();
}
