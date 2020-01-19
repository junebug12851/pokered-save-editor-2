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
#ifndef SPRITEDATA_H
#define SPRITEDATA_H

#include <QVector>
#include "optional"
#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;
struct SpriteDBEntry;
struct MapDBEntrySprite;
struct MapDBEntry;
struct TmpSpritePos;

enum class SpriteMovementStatus : var8
{
  UnInit = 0,
  Ready,
  Delayed,
  Moving
};

struct SpriteFacing
{
  // Place these in same scope
  enum : var8
  {
    Down = 4 * 0,
    Up = 4 * 1,
    Left = 4 * 2,
    Right = 4 * 3,
    None = 0xFF
  };

  static var8 random();
};

struct SpriteMobility
{
  enum : var8
  {
    Moving = 0xFF,
    NotMoving = 0xFE,
    MovingWithoutCollision = 0x00
  };

  static var8 random();
};

struct SpriteMovement
{
  enum : var8 {
    UpDown = 0x01,
    LeftRight = 0x02,
    Down = 0xD0,
    Up = 0xD1,
    Left = 0xD2,
    Right = 0xD3,

    // Allow to be moved with strength
    // Does this apply only to boulders or any sprite or does it enable it
    // For special scripted sprites???
    StrengthMovement = 0x10
  };

  static var8 random();
};

struct SpriteGrass
{
  enum : var8
  {
    InGrass = 0x00,
    NotInGrass = 0x80
  };

  static var8 random();
};

class SpriteData : ExpandedInterface
{
public:
  // Create a blank sprite or load one from a map
  SpriteData(bool blankNPC = false,
             SaveFile* saveFile = nullptr,
             var8 index = 0);

  // Create a sprite from map data
  SpriteData(MapDBEntrySprite* entry);

  virtual ~SpriteData();

  void load(bool blankNPC = false,
            SaveFile* saveFile = nullptr,
            var8 index = 0);

  // Loads preset from map data, useful if your not already in the map but
  // want to re-create the sprites as they should be on said map
  void load(MapDBEntrySprite* spriteData);

  void loadSpriteData1(SaveFile* saveFile,
            var8 index);
  void loadSpriteData2(SaveFile* saveFile,
            var8 index);
  void loadSpriteDataNPC(SaveFile* saveFile,
            var8 index);
  void checkMissable(SaveFile* saveFile,
            var8 index);

  void save(SaveFile* saveFile,
            var8 index);
  void saveSpriteData1(SaveFile* saveFile,
            var8 index);
  void saveSpriteData2(SaveFile* saveFile,
            var8 index);
  void saveSpriteDataNPC(SaveFile* saveFile,
            var8 index);

  static void saveMissables(SaveFile* saveFile,
                       QVector<SpriteData*> spriteData);

  void reset(bool blankNPC = false);
  static QVector<SpriteData*> randomizeAll(QVector<MapDBEntrySprite*> mapSprites);
  void randomize(QVector<TmpSpritePos*>* tmpPos);

  SpriteDBEntry* toSprite();

  /**
   * Sprite data that applies to all sprites
  */

  // Actual sprite image shown
  var8 pictureID;

  // (0: uninitialized, 1: ready, 2: delayed, 3: moving)
  // Advised to use ready
  var8 movementStatus;

  // Basically, in the sprite sheet strip, which "pane" or "tile" is it at
  // 0xFF if not on the screen. Advised to use 0xFF
  var8 imageIndex;

  // (0: down, 4: up, 8: left, $c: right)
  var8 faceDir;

  // Important to set to 0x8
  // Keep sprites from wandering too far however it's noted that it's bugged
  // to begin with. Both are init to 0x8
  var8 yDisp;
  var8 xDisp;

  // These are very important to set, just usual coordinates for map placement
  // +4
  // Coordinate 0,0 would be 4,4
  // Coordinate 5,5 would be 9,9
  // etc...
  // Values less than 4 don't appear on the map
  var8 mapY;
  var8 mapX;

  // (0xFF not moving, 0xFE random movements, others move without collision)
  var8 movementByte;

  // (0x80 in grass, 0x00 otherwise) - Prioritizing grass drawn around sprite
  var8 grassPriority;

  /**
   * These actually don't really matter and can be zero
   * I'm not sure if the game even uses these values in the sav file when
   * loading. Regardless things work just fine if they're all zero
  */

  // When the sprite moves, exactly how far or how much is that?
  // (-1, 0 or 1)
  var8 yStepVector;
  var8 xStepVector;

  // Screen position in pixels aligned to 4 pixels offset from the grid
  // (To appear centered)
  var8 yPixels;
  var8 xPixels;

  // Counter that helps delay between animation frames so things aren't so
  // instant and fast
  var8 intraAnimationFrameCounter;

  // Animation frame counter
  var8 animFrameCounter;

  // Tracks movement & wandering, sprites are given 0x10 and it's decremented

  var8 walkAnimationCounter;

  // Delay until next movement, counts downward and flags movementStatus ready
  // once reached
  var8 movementDelay;

  // Used to help compute imageIndex based on vram
  var8 imageBaseOffset;

  /*
   * Sprite data that applies to all non-player sprites
   * (All sprites that aren't sprite #0)
   * In reality though the game doesn't use any sprite data at all for the
   * player so none of this matters to begin with
  */

  // How far a walking sprite can wander, or if still the facing direction
  // A walking sprite having a value of 0 faces all directions
  // There are certain special values that give special movement
  // Notably 1-2 and D0-D3
  std::optional<var8> rangeDirByte;

  // Text id when this sprite is interacted with
  std::optional<var8> textID;

  // If this is an item sprite, the item id, otherwise the trainer class
  std::optional<var8> trainerClassOrItemID;

  // Trainer data id
  std::optional<var8> trainerSetID;

  /**
   * Sprite data that applies to all non-player missable sprites
   * (All sprites that aren't sprite #0 and have an associated index in the
   * global missable list which determine if it's rendered or not)
  */

  // If this is not null, then this sprite is a missable and it's appearance
  // is determined by the flag in the global missable index this points to
  std::optional<var8> missableIndex;

private:
  // To surpress warnings with using the ExpandedInterface contract
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();
};

#endif // SPRITEDATA_H
