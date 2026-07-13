/*
  * Copyright 2020 Twilight
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
 * @file spritedata.cpp
 * @brief Implementation of SpriteData -- one on-map sprite/NPC, including the
 *        split sprite-data tables and the optional-field accessors. See spritedata.h.
 */

#include <QDebug>

#include "spritedata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/sprites.h>
#include <pse-db/entries/mapdbentrysprite.h>
#include <pse-db/entries/mapdbentryspritetrainer.h>
#include <pse-db/entries/mapdbentryspriteitem.h>
#include <pse-db/entries/mapdbentryspritepokemon.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-db/mapsdb.h>
#include <pse-db/trainers.h>
#include <pse-db/itemsdb.h>
#include <pse-db/pokemon.h>
#include <pse-common/random.h>

struct TmpSpritePos
{
  var8 x;
  var8 y;
};

var8 SpriteFacing::random()
{
  return Random::inst()->rangeInclusive(0, 3) * 4;
}

var8 SpriteMobility::random()
{
  // Only ever Walk (0xFE) or Stay (0xFF) -- never the no-collision values, which would let
  // a randomized NPC walk through walls and out of the map.
  return Random::inst()->rangeInclusive(SpriteMobility::Walk, SpriteMobility::Stay);
}

var8 SpriteMovement::random()
{
  // Movement byte 2. Deliberately restricted to the *unrestricted* patterns: a randomized
  // sprite should wander, not be pinned to an axis or a single direction (and certainly not
  // become a boulder). Randomization is meant to be fun, not to quietly lock an NPC in place.
  var8 ret = Random::inst()->rangeInclusive(0, 10);

  while(ret == UpDown ||
        ret == LeftRight ||
        ret == StrengthMovement ||
        ret == Down ||
        ret == Up ||
        ret == Left ||
        ret == Right)
    ret = Random::inst()->rangeInclusive(0, 10);

  return ret;
}

var8 SpriteGrass::random()
{
  var8 ret[2] = {
    SpriteGrass::NotInGrass,
    SpriteGrass::InGrass
  };

  return ret[Random::inst()->rangeExclusive(0, 2)];
}

/**
 * Movement byte 2 -> the animation facing (`spritestatedata1` field 9).
 *
 * These are two different bytes and the save carries both. But a sprite whose movement byte 2
 * is one of the four fixed directions will be facing that way the moment the game draws it,
 * so when we BUILD a sprite from map data it is honest -- and much nicer to look at -- to
 * start it facing where the game will put it. Everything else starts facing Down, which is
 * what the console leaves in the field after a map load (verified: Pallet's Oak, movement
 * byte 2 = $FF, reads facing = $00).
 */
static int facingFromMovement2(int movement2)
{
  switch(movement2) {
    case SpriteMovement::Down:  return SpriteFacing::Down;
    case SpriteMovement::Up:    return SpriteFacing::Up;
    case SpriteMovement::Left:  return SpriteFacing::Left;
    case SpriteMovement::Right: return SpriteFacing::Right;
    default:                    return SpriteFacing::Down;
  }
}

/**
 * `maps.json`'s `face` string -> movement byte 2.
 *
 * The curated data splits ONE byte in two: `range` (a number, for WALK sprites) and `face`
 * (a string, for STAY sprites). This is the string half.
 */
static int movement2FromFace(const QString& face)
{
  if(face == "None")  return SpriteMovement::None;
  if(face == "Down")  return SpriteMovement::Down;
  if(face == "Up")    return SpriteMovement::Up;
  if(face == "Left")  return SpriteMovement::Left;
  if(face == "Right") return SpriteMovement::Right;

  // The data spells the boulder's value out longhand rather than as a number.
  if(face == "Boulder Movement Byte 2") return SpriteMovement::StrengthMovement;

  // No `face` and no `range` at all -- the game's default: wander freely.
  return SpriteMovement::AnyDir;
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

  pictureID = spriteData->getToSprite()->ind;
  pictureIDChanged();

  // The game keeps a second copy of the picture id in spritestatedata2 field d. Keep them
  // agreeing -- a sprite whose two picture ids disagree is a thing the save can hold, but
  // never a thing we should CREATE.
  pictureIDCopy = pictureID;
  pictureIDCopyChanged();

  mapX = spriteData->adjustedX();
  mapXChanged();

  mapY = spriteData->adjustedY();
  mapYChanged();

  // MOVEMENT BYTE 1 -- may it move at all?  WALK = 0xFE, STAY = 0xFF.
  // (These were inverted here until 2026-07-13: "Stay" wrote 0xFE, i.e. WALK.)
  movementByte = (spriteData->getMove() == "Stay")
                   ? SpriteMobility::Stay
                   : SpriteMobility::Walk;
  movementByteChanged();

  textID = spriteData->getText();
  textIDChanged();

  // MOVEMENT BYTE 2 -- ONE byte, which maps.json curates into two fields: `range` (a number,
  // on WALK sprites) and `face` (a string, on STAY sprites). Whichever is present, it lands
  // in the same place: rangeDirByte.
  //
  // This used to route `face` into faceDir -- the *animation facing*, a different field in a
  // different table -- which both lost the real value and left movement byte 2 at 0 (ANY_DIR).
  const int movement2 = (spriteData->getRange() >= 0)
                          ? spriteData->getRange()
                          : movement2FromFace(spriteData->getFace());

  rangeDirByte = movement2;
  rangeDirByteChanged();

  // The animation facing is its own byte. Start it where the game will have it (see above).
  faceDir = facingFromMovement2(movement2);
  faceDirChanged();

  origFacingDir = faceDir;
  origFacingDirChanged();

  // Set Missable
  if(spriteData->getMissable() >= 0) {
    missableIndex = spriteData->getMissable();
    missableIndexChanged();
  }

  if(spriteData->type() == MapDBEntrySprite::SpriteType::TRAINER) {
    auto spriteDataTrainer = (MapDBEntrySpriteTrainer*)spriteData;
    trainerClassOrItemID = spriteDataTrainer->getToTrainer()->ind;
    trainerClassOrItemIDChanged();

    trainerSetID = spriteDataTrainer->getTeam();
    trainerSetIDChanged();
  }
  else if(spriteData->type() == MapDBEntrySprite::SpriteType::ITEM) {
    // There's two weird sprite data entries in gen1, both are named "0"
    // Not the number 0, an actual string consisting of a zero. The sprite
    // reference is also invalid. If we spot it we can't load any data from the
    // sprite
    auto spriteDataItem = (MapDBEntrySpriteItem*)spriteData;

    // As stated above, stop here if the item is named "0"
    if(spriteDataItem->getItem() == "0") {
      return;
    }

    trainerClassOrItemID = spriteDataItem->getToItem()->getInd();
    trainerClassOrItemIDChanged();
  }
  else if(spriteData->type() == MapDBEntrySprite::SpriteType::POKEMON) {
    auto spriteDataMon = (MapDBEntrySpritePokemon*)spriteData;
    trainerClassOrItemID = spriteDataMon->getToPokemon()->ind;
    trainerClassOrItemIDChanged();

    trainerSetID = spriteDataMon->getLevel();
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

  // Fields a, b, c -- the game keeps them, so we do too (they were never read before).
  yAdjusted = it->getByte();
  yAdjustedChanged();

  xAdjusted = it->getByte();
  xAdjustedChanged();

  collisionData = it->getByte();
  collisionDataChanged();

  // d, e, f are unused by the game. Left alone -- we do not touch bytes we have no business
  // touching.
  delete it;
}

void SpriteData::loadSpriteData2(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2E2C);

  walkAnimationCounter = it->getByte(1);   // field 0, then skip the unused field 1
  walkAnimationCounterChanged();

  yDisp = it->getByte();                   // 2
  yDispChanged();

  xDisp = it->getByte();                   // 3
  xDispChanged();

  mapY = it->getByte();                    // 4
  mapYChanged();

  mapX = it->getByte();                    // 5
  mapXChanged();

  movementByte = it->getByte();            // 6 -- MOVEMENT BYTE 1
  movementByteChanged();

  grassPriority = it->getByte();           // 7
  grassPriorityChanged();

  movementDelay = it->getByte();           // 8
  movementDelayChanged();

  origFacingDir = it->getByte(3);          // 9, then skip the unused a, b, c
  origFacingDirChanged();

  pictureIDCopy = it->getByte();           // d -- the game's second copy of the picture id
  pictureIDCopyChanged();

  imageBaseOffset = it->getByte();         // e
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
  it->setByte(yAdjusted);        // a
  it->setByte(xAdjusted);        // b
  it->setByte(collisionData);    // c
  // d, e, f: unused by the game -- and so not written by us.

  delete it;
}

void SpriteData::saveSpriteData2(SaveFile* saveFile, var8 index)
{
  auto it = saveFile->iterator()->offsetTo((0x10 * index) + 0x2E2C);
  it->setByte(walkAnimationCounter, 1); // 0, skip the unused 1
  it->setByte(yDisp);                   // 2
  it->setByte(xDisp);                   // 3
  it->setByte(mapY);                    // 4
  it->setByte(mapX);                    // 5
  it->setByte(movementByte);            // 6
  it->setByte(grassPriority);           // 7
  it->setByte(movementDelay);           // 8
  it->setByte(origFacingDir, 3);        // 9, skip the unused a, b, c
  it->setByte(pictureIDCopy);           // d
  it->setByte(imageBaseOffset);         // e
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

  // STAY (0xFF). A sprite you have just placed should stand where you put it -- and until
  // 2026-07-13 this wrote 0xFE, which is WALK: every new sprite wandered off.
  movementByte = (var8)SpriteMobility::Stay;
  movementByteChanged();

  // 0x00. Until 2026-07-13 this wrote 0x80 -- which flagged every blank sprite as standing
  // in tall grass.
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

  yAdjusted = 0;
  yAdjustedChanged();

  xAdjusted = 0;
  xAdjustedChanged();

  collisionData = 0;
  collisionDataChanged();

  origFacingDir = (var8)SpriteFacing::Down;
  origFacingDirChanged();

  pictureIDCopy = 0;
  pictureIDCopyChanged();

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
    if(entry->getSprite() == "Boulder")
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
    if(entry->getSprite() == "Boulder")
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
    var8 rndPos = Random::inst()->rangeExclusive(0, tmpPos->size());
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
      SpritesDB::inst()->getStore().at(Random::inst()->rangeExclusive(0, SpritesDB::inst()->getStoreSize()));

  pictureID = picture->ind;
  pictureIDChanged();

  pictureIDCopy = pictureID;   // the game's second copy -- keep them agreeing
  pictureIDCopyChanged();

  // Get a random facing direction and movement
  faceDir = SpriteFacing::random();
  faceDirChanged();

  origFacingDir = faceDir;
  origFacingDirChanged();

  movementByte = SpriteMobility::random();   // WALK or STAY -- never a no-collision value
  movementByteChanged();

  // Without absurdly more complex coding there's no way to tell if the sprite
  // is in grass or not so we just give it a random value.
  grassPriority = SpriteGrass::random();
  grassPriorityChanged();
}

void SpriteData::setTo(MapDBEntrySprite* spriteData)
{
  reset();

  if(spriteData == nullptr)
    return;

  load(spriteData);
}

QVector<SpriteData*> SpriteData::setToAll(QVector<MapDBEntrySprite*> spriteSigns)
{
  // Prepare return value
  QVector<SpriteData*> sprites;

  for(auto entry : spriteSigns) {

    // New Sprite from map data
    auto tmp = new SpriteData;
    sprites.append(tmp);
    tmp->setTo(entry);
  }

  return sprites;
}

SpriteDBEntry* SpriteData::toSprite()
{
  return SpritesDB::inst()->getIndAt(QString::number(pictureID));
}
