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
#include "spritedata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

#include <QRandomGenerator>

SpriteData::SpriteData(bool blankNPC, SaveFile* saveFile, var8 index)
{
  load(blankNPC, saveFile, index);
}

SpriteData::~SpriteData() {}

void SpriteData::load(bool blankNPC, SaveFile* saveFile, var8 index)
{
  if(saveFile == nullptr)
    return reset(blankNPC);

  // Grab sprite data 1 and 2 which applies to all sprites
  loadSpriteData1(saveFile, index);
  loadSpriteData2(saveFile, index);

  // If this isn't the player sprite then load additional non-player data
  // and check to see if it's missable
  if (index > 0) {
    loadSpriteDataNPC(saveFile, index);
    checkMissable(saveFile, index);
  }
}

void SpriteData::loadSpriteData1(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2D2C);

  pictureID = it->getByte();
  movementStatus = it->getByte();
  imageIndex = it->getByte();
  yStepVector = it->getByte();
  yPixels = it->getByte();
  xStepVector = it->getByte();
  xPixels = it->getByte();
  intraAnimationFrameCounter = it->getByte();
  animFrameCounter = it->getByte();
  faceDir = it->getByte();
}

void SpriteData::loadSpriteData2(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2E2C);
  walkAnimationCounter = it->getByte(1);
  yDisp = it->getByte();
  xDisp = it->getByte();
  mapY = it->getByte();
  mapX = it->getByte();
  movementByte = it->getByte();
  grassPriority = it->getByte();
  movementDelay = it->getByte(5);
  imageBaseOffset = it->getByte();
}

void SpriteData::loadSpriteDataNPC(SaveFile* saveFile, var8 index)
{
  index--;

  // Init missable index to non-player value
  missableIndex = -1;

  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x2790);
  rangeDirByte = it->getByte();
  textID = it->getByte();

  it->offsetTo((2 * index) + 0x27B0);
  trainerClassOrItemID = it->getByte();
  trainerSetID = it->getByte();
}

void SpriteData::checkMissable(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator();

  for (var8 i = 0; i < 17; i++) {
    it->offsetTo((0x2 * i) + 0x287A);
    var8 mId = it->getByte();
    var8 mIndex = it->getByte();

    // Stop when reach list terminator
    if (mId == 0xFF)
      break;

    // Skip if the id doesn't match this sprite
    if (mId != index)
      continue;

    // Save bit index this missable refers to and add to missable
    // array for quick reference
    this->missableIndex = mIndex;
    break;
  }
}

void SpriteData::save(SaveFile* saveFile, var8 index)
{
  // Grab sprite data 1 and 2 which applies to all sprites
  saveSpriteData1(saveFile, index);
  saveSpriteData2(saveFile, index);

  // If this isn't the player sprite then save NPC data
  if (index > 0) {
    saveSpriteDataNPC(saveFile, index);
  }
}

void SpriteData::saveSpriteData1(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2D2C);
  it->setByte(pictureID);
  it->setByte(movementStatus);
  it->setByte(imageIndex);
  it->setByte(yStepVector);
  it->setByte(yPixels);
  it->setByte(xStepVector);
  it->setByte(xPixels);
  it->setByte(intraAnimationFrameCounter);
  it->setByte(animFrameCounter);
  it->setByte(faceDir);
}

void SpriteData::saveSpriteData2(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2E2C);
  it->setByte(walkAnimationCounter, 1);
  it->setByte(yDisp);
  it->setByte(xDisp);
  it->setByte(mapY);
  it->setByte(mapX);
  it->setByte(movementByte);
  it->setByte(grassPriority);
  it->setByte(movementDelay, 5);
  it->setByte(imageBaseOffset);
}

void SpriteData::saveSpriteDataNPC(SaveFile* saveFile, var8 index)
{
  // Correct index, this data starts sprite 0 at sprite 1
  index--;
  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x2790);
  it->setByte(*rangeDirByte);
  it->setByte(*textID);

  it->offsetTo((2 * index) + 0x27B0);
  it->setByte(*trainerClassOrItemID);
  it->setByte(*trainerSetID);
}

void SpriteData::saveMissables(SaveFile* saveFile, QVector<SpriteData*> spriteData)
{
  auto it = saveFile->iterator();

  it->offsetTo(0x287A);
  for (var8 i = 0; i < spriteData.size() && i < 16; i++) {
    auto val = spriteData[i];

    // Skip all sprites that aren't missables
    if(val->missableIndex || !(val->missableIndex >= 0))
      continue;

    // Save sprite index that's missable
    // Don't correct index, this data starts at sprite 1
    it->setByte(i);

    // Save missable index this sprite connects to
    it->setByte(*val->missableIndex);
  }

  it->setByte(0xFF);
}

void SpriteData::reset(bool blankNPC)
{
  pictureID = 0;
  movementStatus = 0;
  imageIndex = 0;
  yStepVector = 0;
  yPixels = 0;
  xStepVector = 0;
  xPixels = 0;
  intraAnimationFrameCounter = 0;
  animFrameCounter = 0;
  faceDir = 0;
  walkAnimationCounter = 0;
  yDisp = 0;
  xDisp = 0;
  mapY = 0;
  mapX = 0;
  movementByte = 0;
  grassPriority = 0;
  movementDelay = 0;
  imageBaseOffset = 0;

  if(!blankNPC) {
    rangeDirByte.reset();
    textID.reset();
    trainerClassOrItemID.reset();
    trainerSetID.reset();
  }
  else {
    rangeDirByte = 0;
    textID = 0;
    trainerClassOrItemID = 0;
    trainerSetID = 0;
  }

  missableIndex.reset();
}

// Can't really make this possible, needs more information before can randomize
// @TODO make this happen
void SpriteData::randomize() {}

// Don't Use
void SpriteData::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void SpriteData::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void SpriteData::reset() {}
