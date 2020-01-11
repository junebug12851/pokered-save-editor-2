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
#include "../../../db/maps.h"
#include "../../../db/sprites.h"

#include <QRandomGenerator>


AreaSprites::AreaSprites(SaveFile* saveFile)
{
  // By default add player sprite
  sprites.append(new SpriteData(false));
  load(saveFile);
}

AreaSprites::~AreaSprites()
{
  reset();
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
    delete entry;

  sprites.clear();
}

void AreaSprites::randomize(MapDBEntry* mapData)
{
  reset();

  auto mapSprites = mapData->sprites;
  auto rnd = QRandomGenerator::global();

  // Add blank player sprite
  sprites.append(new SpriteData(false));

  var8 stepVector[] = {
    0xFF, 0, 1
  };

  var8 imgInc = 2;

  for(auto entry : mapSprites) {
    auto tmp = new SpriteData(true);
    tmp->pictureID = entry->toSprite->ind;
    tmp->movementStatus = (var8)SpriteMovementStatus::Ready;
    tmp->imageIndex = 0xFF;
    tmp->yStepVector = stepVector[rnd->bounded(0, 3)];
    tmp->xStepVector = stepVector[rnd->bounded(0, 3)];
    tmp->yPixels = entry->adjustedY();
    tmp->xPixels = entry->adjustedX();
    tmp->intraAnimationFrameCounter = 0;
    tmp->animFrameCounter = 0;
    tmp->faceDir = rnd->bounded(0, 3+1) * 4;
    tmp->walkAnimationCounter = 0x10;
    tmp->yDisp = 8;
    tmp->xDisp = 8;
    tmp->mapY = entry->adjustedY() / 2;
    tmp->mapX = entry->adjustedX() / 2;
    tmp->movementByte = rnd->bounded(0xFE, 0xFF+1);
    tmp->grassPriority = (rnd->bounded(0, 1+1) == 1) ? 0x00 : 0x80;
    tmp->movementDelay = 0;
    tmp->imageBaseOffset = imgInc; // 2 + Cached Sprite Entry

    imgInc++;
  }
}

// Not to use
void AreaSprites::randomize() {}
