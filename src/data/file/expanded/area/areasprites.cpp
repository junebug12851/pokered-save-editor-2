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
  /*
   * After soem expirementation with my old editor I learned that:
   * X & Y pixel values don't do anything, only the X & Y map does
   * The X & Y map values are exactly +4 off each of normal positioning
   * Literally all the animation details can be zero and work just fine
   *     Frame Index seems to do better at 255
   *     X&Y Track have to be at 8 each
   * Outdoor sprites will be glitchy, I've played around with pre-loaded sprites
   * and many other variables but can't really find a solution as to why.
   *
   * Outside of that it doesn't seem to matter at all what most all of these
   * values are set to. There is no getting around the outdoor sprites being
   * glitchy though. I've tried everything I can think of, even changing tileset
   * type to "indoor" but it's not the end of the world.
   */
  reset();

  auto mapSprites = mapData->sprites;
  auto rnd = QRandomGenerator::global();

  // Add blank player sprite, very important for it to be at pos 0
  // Player sprite actually has a lot of data but none of it's used at all by
  // the game
  sprites.append(new SpriteData(false));

  for(auto entry : mapSprites) {
    // New NPC Sprite
    auto tmp = new SpriteData(true);

    // If these are overwritten and randomized then the player cannot get items
    // or progress in the game
    if(entry->sprite == "Boulder" ||
       entry->sprite == "Pokeball")
      continue;

    // Get a random sprite picture
    auto picture =
        SpritesDB::store.at(rnd->bounded(0, SpritesDB::store.size()));

    tmp->pictureID = picture->ind;
    tmp->movementStatus = (var8)SpriteMovementStatus::Ready;
    tmp->faceDir = SpriteFacing::random();

    // Without absurdly more complex coding there's no way to tell if the sprite
    // is in grass or not so we just give it a random value.
    tmp->grassPriority = SpriteGrass::random();

    // These values are more important
    tmp->imageIndex = 0xFF;
    tmp->yDisp = 8;
    tmp->xDisp = 8;
    tmp->mapY = entry->adjustedY(); // Important to use adjusted +4 values
    tmp->mapX = entry->adjustedX();
    tmp->movementByte = SpriteMobility::random();

    // Literally none of these values matter
    tmp->walkAnimationCounter = 0x10; // Could be zero but we set it to 0x10
    tmp->yStepVector = 0;
    tmp->xStepVector = 0;
    tmp->yPixels = 0;
    tmp->xPixels = 0;
    tmp->intraAnimationFrameCounter = 0;
    tmp->animFrameCounter = 0;
    tmp->movementDelay = 0;
    tmp->imageBaseOffset = 0;
  }
}

// Not to use
void AreaSprites::randomize() {}
