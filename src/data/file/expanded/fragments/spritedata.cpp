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
#include "../../../db/sprites.h"
#include "../../../db/maps.h"
#include "../../../db/trainers.h"
#include "../../../db/items.h"
#include "../../../db/pokemon.h"

#include <QRandomGenerator>

struct TmpSpritePos
{
  var8 x;
  var8 y;
};

var8 SpriteFacing::random()
{
  auto rnd = QRandomGenerator::global();
  return rnd->bounded(0, 3+1) * 4;
}

var8 SpriteMobility::random()
{
  // Lets not randomize in the possibility of no-collision
  auto rnd = QRandomGenerator::global();
  return rnd->bounded(0xFE, 0xFF+1);
}

var8 SpriteMovement::random()
{
  auto rnd = QRandomGenerator::global();

  // Lets not randomize such restricted movment
  var8 ret = rnd->bounded(0, 10+1);

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
    ret = rnd->bounded(0, 10+1);

  return ret;
}

var8 SpriteGrass::random()
{
  auto rnd = QRandomGenerator::global();

  var8 ret[2] = {
    0x00,
    0x80
  };

  return ret[rnd->bounded(0, 2)];
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
  mapX = spriteData->adjustedX();
  mapY = spriteData->adjustedY();

  if(spriteData->move == "Stay")
    movementByte = SpriteMobility::NotMoving;
  else
    movementByte = SpriteMobility::Moving;

  textID = spriteData->text;

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

  // Set Missable
  if(spriteData->missable)
    missableIndex = *spriteData->missable;

  if(spriteData->range)
    rangeDirByte = *spriteData->range;

  // Because this is a string, it got incorrectly placed into "face"
  // It's actually a number representing 0x10 and probably needs to go into
  // range instead
  else if(spriteData->face == "Boulder Movement Byte 2")
    rangeDirByte = SpriteMovement::StrengthMovement;

  if(spriteData->type() == SpriteType::TRAINER) {
    auto spriteDataTrainer = (MapDBEntrySpriteTrainer*)spriteData;
    trainerClassOrItemID = spriteDataTrainer->toTrainer->ind;
    trainerSetID = spriteDataTrainer->team;
  }
  else if(spriteData->type() == SpriteType::ITEM) {
    auto spriteDataItem = (MapDBEntrySpriteItem*)spriteData;
    trainerClassOrItemID = spriteDataItem->toItem->ind;
  }
  else if(spriteData->type() == SpriteType::POKEMON) {
    auto spriteDataMon = (MapDBEntrySpritePokemon*)spriteData;
    trainerClassOrItemID = spriteDataMon->toPokemon->ind;
    trainerSetID = spriteDataMon->level;
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

  delete it;
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

  delete it;
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
    if(val->missableIndex || !(val->missableIndex >= 0))
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

void SpriteData::reset(bool blankNPC)
{
  pictureID = 0;
  movementStatus = (var8)SpriteMovementStatus::Ready;
  imageIndex = 0xFF;
  faceDir = (var8)SpriteFacing::Down;
  yDisp = 0x8;
  xDisp = 0x8;
  mapY = 4;
  mapX = 4;
  movementByte = (var8)SpriteMobility::NotMoving;
  grassPriority = (var8)SpriteGrass::NotInGrass;
  yStepVector = 0;
  yPixels = 0;
  xStepVector = 0;
  xPixels = 0;
  intraAnimationFrameCounter = 0;
  animFrameCounter = 0;
  walkAnimationCounter = 0x10; // Could also be zero, not important
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
  // Parepare random method alias
  auto rnd = QRandomGenerator::global();

  // Randomize coordinates if coord list is provided
  if(tmpPos != nullptr) {

    // Pull random coordinates
    var8 rndPos = rnd->bounded(0, tmpPos->size());
    auto rndCoords = tmpPos->at(rndPos);

    // Change coords
    mapX = rndCoords->x;
    mapY = rndCoords->y;

    // Remove chosen random coordinate
    delete rndCoords;
    tmpPos->removeAt(rndPos);
  }

  // Get a random sprite picture and change it
  auto picture =
      SpritesDB::store.at(rnd->bounded(0, SpritesDB::store.size()));

  pictureID = picture->ind;

  // Get a random facing direction and movement
  faceDir = SpriteFacing::random();
  movementByte = SpriteMobility::random();

  // Without absurdly more complex coding there's no way to tell if the sprite
  // is in grass or not so we just give it a random value.
  grassPriority = SpriteGrass::random();
}

SpriteDBEntry* SpriteData::toSprite()
{
  return SpritesDB::ind.value(QString::number(pictureID), nullptr);
}

// Don't Use
void SpriteData::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void SpriteData::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void SpriteData::reset() {}
void SpriteData::randomize() {}
