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

#include <QDebug>

#include "spritedata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/sprites.h"
#include "../../../db/maps.h"
#include "../../../db/trainers.h"
#include "../../../db/items.h"
#include "../../../db/pokemon.h"
#include "../../../../common/random.h"

struct TmpSpritePos
{
  var8 x;
  var8 y;
};

var8 SpriteFacing::random()
{
  return Random::rangeInclusive(0, 3) * 4;
}

var8 SpriteMobility::random()
{
  // Lets not randomize in the possibility of no-collision
  return Random::rangeInclusive(0xFE, 0xFF);
}

var8 SpriteMovement::random()
{
  // Lets not randomize such restricted movment
  var8 ret = Random::rangeInclusive(0, 10);

  // Ensure we don't get a specialized movement value
  // I have no idea where these sprites are or on what map so I'd hate to intend
  // for them to move but don't or very far, randomization is about being a
  // bit fun
  while(ret == UpDown ||
        ret == LeftRight ||
        ret == Down ||
        ret == Up ||
        ret == Left ||
        ret == Right)
    ret = Random::rangeInclusive(0, 10);

  return ret;
}

var8 SpriteGrass::random()
{
  var8 ret[2] = {
    0x00,
    0x80
  };

  return ret[Random::rangeExclusive(0, 2)];
}

SpriteData::SpriteData(bool blankNPC, SaveFile* saveFile, var8 index)
{
  load(blankNPC, saveFile, index);
}

SpriteData::SpriteData(MapDBEntrySprite* entry)
{
  load(entry);
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

void SpriteData::load(MapDBEntrySprite* spriteData)
{
  reset(true);

  pictureID = spriteData->toSprite->ind;
  pictureIDChanged();

  mapX = spriteData->adjustedX();
  mapXChanged();

  mapY = spriteData->adjustedY();
  mapYChanged();

  if(spriteData->move == "Stay")
    movementByte = SpriteMobility::NotMoving;
  else
    movementByte = SpriteMobility::Moving;
  movementByteChanged();

  textID = spriteData->text;
  textIDChanged();

  if(spriteData->face == "Down")
    faceDir = SpriteFacing::Down;
  else if(spriteData->face == "Left")
    faceDir = SpriteFacing::Left;
  else if(spriteData->face == "None")
    faceDir = SpriteFacing::None;
  else if(spriteData->face == "Right")
    faceDir = SpriteFacing::Right;
  else if(spriteData->face == "Up")
    faceDir = SpriteFacing::Up;

  faceDirChanged();

  // Set Missable
  if(spriteData->missable) {
    missableIndex = *spriteData->missable;
    missableIndexChanged();
  }

  if(spriteData->range) {
    rangeDirByte = *spriteData->range;
    rangeDirByteChanged();
  }

  // Because this is a string, it got incorrectly placed into "face"
  // It's actually a number representing 0x10 and probably needs to go into
  // range instead
  else if(spriteData->face == "Boulder Movement Byte 2") {
    rangeDirByte = SpriteMovement::StrengthMovement;
    rangeDirByteChanged();
  }

  if(spriteData->type() == SpriteType::TRAINER) {
    auto spriteDataTrainer = (MapDBEntrySpriteTrainer*)spriteData;
    trainerClassOrItemID = spriteDataTrainer->toTrainer->ind;
    trainerClassOrItemIDChanged();

    trainerSetID = spriteDataTrainer->team;
    trainerSetIDChanged();
  }
  else if(spriteData->type() == SpriteType::ITEM) {
    auto spriteDataItem = (MapDBEntrySpriteItem*)spriteData;
    trainerClassOrItemID = spriteDataItem->toItem->ind;
    trainerClassOrItemIDChanged();
  }
  else if(spriteData->type() == SpriteType::POKEMON) {
    auto spriteDataMon = (MapDBEntrySpritePokemon*)spriteData;
    trainerClassOrItemID = spriteDataMon->toPokemon->ind;
    trainerClassOrItemIDChanged();

    trainerSetID = spriteDataMon->level;
    trainerSetIDChanged();
  }
}

void SpriteData::loadSpriteData1(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2D2C);

  pictureID = it->getByte();
  pictureIDChanged();

  movementStatus = it->getByte();
  movementStatusChanged();

  imageIndex = it->getByte();
  imageIndexChanged();

  yStepVector = it->getByte();
  yStepVectorChanged();

  yPixels = it->getByte();
  yPixelsChanged();

  xStepVector = it->getByte();
  xStepVectorChanged();

  xPixels = it->getByte();
  xPixelsChanged();

  intraAnimationFrameCounter = it->getByte();
  intraAnimationFrameCounterChanged();

  animFrameCounter = it->getByte();
  animFrameCounterChanged();

  faceDir = it->getByte();
  faceDirChanged();

  delete it;
}

void SpriteData::loadSpriteData2(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2E2C);

  walkAnimationCounter = it->getByte(1);
  walkAnimationCounterChanged();

  yDisp = it->getByte();
  yDispChanged();

  xDisp = it->getByte();
  xDispChanged();

  mapY = it->getByte();
  mapYChanged();

  mapX = it->getByte();
  mapXChanged();

  movementByte = it->getByte();
  movementByteChanged();

  grassPriority = it->getByte();
  grassPriorityChanged();

  movementDelay = it->getByte(5);
  movementDelayChanged();

  imageBaseOffset = it->getByte();
  imageBaseOffsetChanged();

  delete it;
}

void SpriteData::loadSpriteDataNPC(SaveFile* saveFile, var8 index)
{
  index--;

  // Init missable index to non-player value
  missableIndex = -1;
  missableIndexChanged();

  auto it = saveFile->iterator()->offsetTo((2 * index) + 0x2790);
  rangeDirByte = it->getByte();
  rangeDirByteChanged();

  textID = it->getByte();
  textIDChanged();

  it->offsetTo((2 * index) + 0x27B0);
  trainerClassOrItemID = it->getByte();
  trainerClassOrItemIDChanged();

  trainerSetID = it->getByte();
  trainerSetIDChanged();

  delete it;
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
    this->missableIndexChanged();
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

  delete it;
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
  delete it;
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
  delete it;
}

void SpriteData::saveMissables(SaveFile* saveFile, QVector<SpriteData*> spriteData)
{
  auto it = saveFile->iterator();

  it->offsetTo(0x287A);
  for (var8 i = 0; i < spriteData.size() && i < 16; i++) {
    auto val = spriteData[i];

    // Skip all sprites that aren't missables
    if(!val->missableIndex || *val->missableIndex < 0)
      continue;

    // Save sprite index that's missable
    // Don't correct index, this data starts at sprite 1
    it->setByte(i);

    // Save missable index this sprite connects to
    it->setByte(*val->missableIndex);
  }

  it->setByte(0xFF);
  delete it;
}

int SpriteData::getRangeDirByte()
{
  if(rangeDirByte)
    return *rangeDirByte;

  return -1;
}

void SpriteData::setRangeDirByte(int val)
{
  rangeDirByte = val;
  rangeDirByteChanged();
}

void SpriteData::resetRangeDirByte()
{
  rangeDirByte.reset();
  rangeDirByteChanged();
}

int SpriteData::getTextID()
{
  if(textID)
    return *textID;

  return -1;
}

void SpriteData::setTextID(int val)
{
  textID = val;
  textIDChanged();
}

void SpriteData::resetTextID()
{
  textID.reset();
  textIDChanged();
}

int SpriteData::getTrainerClassOrItemID()
{
  if(trainerClassOrItemID)
    return *trainerClassOrItemID;

  return -1;
}

void SpriteData::setTrainerClassOrItemID(int val)
{
  trainerClassOrItemID = val;
  trainerClassOrItemIDChanged();
}

void SpriteData::resetTrainerClassOrItemID()
{
  trainerClassOrItemID.reset();
  trainerClassOrItemIDChanged();
}

int SpriteData::getTrainerSetID()
{
  if(trainerSetID)
    return *trainerSetID;

  return -1;
}

void SpriteData::setTrainerSetID(int val)
{
  trainerSetID = val;
  trainerSetIDChanged();
}

void SpriteData::resetTrainerSetID()
{
  trainerSetID.reset();
  trainerSetIDChanged();
}

int SpriteData::getMissableIndex()
{
  if(missableIndex)
    return *missableIndex;

  return -1;
}

void SpriteData::setMissableIndex(int val)
{
  missableIndex = val;
  missableIndexChanged();
}

void SpriteData::resetMissableIndex()
{
  missableIndex.reset();
  missableIndexChanged();
}

int SpriteData::getYStepVector()
{
  return yStepVector;
}

void SpriteData::setYStepVector(int val)
{
  yStepVector = val;
  yStepVectorChanged();
}

int SpriteData::getXStepVector()
{
  return xStepVector;
}

void SpriteData::setXStepVector(int val)
{
  xStepVector = val;
  xStepVectorChanged();
}

void SpriteData::reset(bool blankNPC)
{
  pictureID = 0;
  pictureIDChanged();

  movementStatus = (var8)SpriteMovementStatus::Ready;
  movementStatusChanged();

  imageIndex = 0xFF;
  imageIndexChanged();

  faceDir = (var8)SpriteFacing::Down;
  faceDirChanged();

  yDisp = 0x8;
  yDispChanged();

  xDisp = 0x8;
  xDispChanged();

  mapY = 4;
  mapYChanged();

  mapX = 4;
  mapXChanged();

  movementByte = (var8)SpriteMobility::NotMoving;
  movementByteChanged();

  grassPriority = (var8)SpriteGrass::NotInGrass;
  grassPriorityChanged();

  yStepVector = 0;
  yStepVectorChanged();

  yPixels = 0;
  yPixelsChanged();

  xStepVector = 0;
  xStepVectorChanged();

  xPixels = 0;
  xPixelsChanged();

  intraAnimationFrameCounter = 0;
  intraAnimationFrameCounterChanged();

  animFrameCounter = 0;
  animFrameCounterChanged();

  walkAnimationCounter = 0x10; // Could also be zero, not important
  walkAnimationCounterChanged();

  movementDelay = 0;
  movementDelayChanged();

  imageBaseOffset = 0;
  imageBaseOffsetChanged();

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

  rangeDirByteChanged();
  textIDChanged();
  trainerClassOrItemIDChanged();
  trainerSetIDChanged();

  missableIndex.reset();
  missableIndexChanged();
}

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
QVector<SpriteData*> SpriteData::randomizeAll(QVector<MapDBEntrySprite*> mapSprites)
{
  // Prepare return value
  QVector<SpriteData*> sprites;

  // Add blank player sprite, very important for it to be at pos 0
  // Player sprite actually has a lot of data but none of it's used at all by
  // the game
  sprites.append(new SpriteData(false));

  // Make first pass and get all sprite positions on map
  QVector<TmpSpritePos*> tmpPos;

  for(auto entry : mapSprites) {

    // Don't note any positions of boulder sprites, leave them as they are
    // with nobody placed on top of them. The map could be unplayable otherwise.
    if(entry->sprite == "Boulder")
      continue;

    auto tmp = new TmpSpritePos;
    tmp->x = entry->adjustedX();
    tmp->y = entry->adjustedY();
    tmpPos.append(tmp);
  }

  // Now create new sprites from data and randomize their contents
  for(auto entry : mapSprites) {

    // New Sprite from map data
    auto tmp = new SpriteData(entry);
    sprites.append(tmp);

    // If boulders are changed at all then the player cannot progress, leave
    // boulders the way they are. Do this only for randomizeAll, if the player
    // wants to randomize a specific one, even if it breaks gameplay, let them
    // do it
    if(entry->sprite == "Boulder")
      continue;

    // Randomize newly created sprite
    tmp->randomize(&tmpPos);
  }

  return sprites;
}

void SpriteData::randomize(QVector<TmpSpritePos*>* tmpPos)
{
  // Randomize coordinates if coord list is provided
  if(tmpPos != nullptr) {

    // Pull random coordinates
    var8 rndPos = Random::rangeExclusive(0, tmpPos->size());
    auto rndCoords = tmpPos->at(rndPos);

    // Change coords
    mapX = rndCoords->x;
    mapXChanged();

    mapY = rndCoords->y;
    mapYChanged();

    // Remove chosen random coordinate
    delete rndCoords;
    tmpPos->removeAt(rndPos);
  }

  // Get a random sprite picture and change it
  auto picture =
      SpritesDB::store.at(Random::rangeExclusive(0, SpritesDB::store.size()));

  pictureID = picture->ind;
  pictureIDChanged();

  // Get a random facing direction and movement
  faceDir = SpriteFacing::random();
  faceDirChanged();

  movementByte = SpriteMobility::random();
  movementByteChanged();

  // Without absurdly more complex coding there's no way to tell if the sprite
  // is in grass or not so we just give it a random value.
  grassPriority = SpriteGrass::random();
  grassPriorityChanged();
}

SpriteDBEntry* SpriteData::toSprite()
{
  return SpritesDB::ind.value(QString::number(pictureID), nullptr);
}
