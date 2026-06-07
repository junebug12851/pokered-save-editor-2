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
#pragma once
#include <QObject>
#include <QVector>
#include "optional"
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
struct SpriteDBEntry;
struct MapDBEntrySprite;
struct MapDBEntry;
struct TmpSpritePos;

/// Sprite movement-status byte values, QML-visible.
struct SAVEFILE_AUTOPORT SpriteMovementStatus : public QObject
{
  Q_OBJECT
  Q_ENUMS(SpriteMovementStatus_)

public:
  enum SpriteMovementStatus_ : var8
  {
    UnInit = 0, ///< Uninitialized.
    Ready,      ///< Ready (advised value).
    Delayed,    ///< Delayed.
    Moving      ///< Moving.
  };
};

/// Sprite facing-direction values, QML-visible.
struct SAVEFILE_AUTOPORT SpriteFacing : public QObject
{
  Q_OBJECT
  Q_ENUMS(SpriteFacing_)

public:
  Q_INVOKABLE static var8 random(); ///< A random valid facing.

  // Place these in same scope
  enum SpriteFacing_ : var8
  {
    Down = 4 * 0,
    Up = 4 * 1,
    Left = 4 * 2,
    Right = 4 * 3,
    None = 0xFF
  };
};

/// Sprite mobility byte values, QML-visible.
struct SAVEFILE_AUTOPORT SpriteMobility : public QObject
{
  Q_OBJECT
  Q_ENUMS(SpriteMobility_)

public:
  Q_INVOKABLE static var8 random(); ///< A random mobility value.

  enum SpriteMobility_ : var8
  {
    Moving = 0xFF,
    NotMoving = 0xFE,
    MovingWithoutCollision = 0x00
  };
};

/// Sprite movement-pattern byte values, QML-visible.
struct SAVEFILE_AUTOPORT SpriteMovement : public QObject
{
  Q_OBJECT
  Q_ENUMS(SpriteMovement_)

public:
  Q_INVOKABLE static var8 random(); ///< A random movement pattern.

  enum SpriteMovement_ : var8 {
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
};

/// Sprite in-grass byte values, QML-visible.
struct SAVEFILE_AUTOPORT SpriteGrass : public QObject
{
  Q_OBJECT
  Q_ENUMS(SpriteGrass_)

public:
  Q_INVOKABLE static var8 random(); ///< A random grass value.

  enum SpriteGrass_ : var8
  {
    InGrass = 0x00,
    NotInGrass = 0x80
  };
};

/**
 * @brief One on-map sprite/NPC -- the most byte-level object in the area.
 *
 * Mirrors the game's per-sprite memory: picture, movement state, facing, map
 * coordinates, animation counters, and (for non-player, non-missable, missable
 * sprites in turn) a set of @c std::optional fields that only some sprites carry.
 * Because the game splits sprite memory across several tables, load()/save() are
 * broken into `...Data1` / `...Data2` / `...NPC` parts (and a missable check).
 *
 * The optional fields (rangeDirByte, textID, trainerClassOrItemID, trainerSetID,
 * missableIndex) are surfaced to QML through getter/setter/reset property triples
 * so QML -- which can't speak std::optional or var8 -- can read, write, and clear
 * them. The field comments below are the authoritative description of each byte.
 *
 * @see SpriteDBEntry, MapDBEntrySprite, AreaSprites (the container).
 */
class SAVEFILE_AUTOPORT SpriteData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int pictureID MEMBER pictureID NOTIFY pictureIDChanged)
  Q_PROPERTY(int movementStatus MEMBER movementStatus NOTIFY movementStatusChanged)
  Q_PROPERTY(int imageIndex MEMBER imageIndex NOTIFY imageIndexChanged)
  Q_PROPERTY(int faceDir MEMBER faceDir NOTIFY faceDirChanged)
  Q_PROPERTY(int yDisp MEMBER yDisp NOTIFY yDispChanged)
  Q_PROPERTY(int xDisp MEMBER xDisp NOTIFY xDispChanged)
  Q_PROPERTY(int mapY MEMBER mapY NOTIFY mapYChanged)
  Q_PROPERTY(int mapX MEMBER mapX NOTIFY mapXChanged)
  Q_PROPERTY(int movementByte MEMBER movementByte NOTIFY movementByteChanged)
  Q_PROPERTY(int grassPriority MEMBER grassPriority NOTIFY grassPriorityChanged)
  Q_PROPERTY(int yPixels MEMBER yPixels NOTIFY yPixelsChanged)
  Q_PROPERTY(int xPixels MEMBER xPixels NOTIFY xPixelsChanged)
  Q_PROPERTY(int intraAnimationFrameCounter MEMBER intraAnimationFrameCounter NOTIFY intraAnimationFrameCounterChanged)
  Q_PROPERTY(int animFrameCounter MEMBER animFrameCounter NOTIFY animFrameCounterChanged)
  Q_PROPERTY(int walkAnimationCounter MEMBER walkAnimationCounter NOTIFY walkAnimationCounterChanged)
  Q_PROPERTY(int movementDelay MEMBER movementDelay NOTIFY movementDelayChanged)
  Q_PROPERTY(int imageBaseOffset MEMBER imageBaseOffset NOTIFY imageBaseOffsetChanged)
  Q_PROPERTY(int rangeDirByte READ getRangeDirByte WRITE setRangeDirByte RESET resetRangeDirByte NOTIFY rangeDirByteChanged)
  Q_PROPERTY(int textID READ getTextID WRITE setTextID RESET resetTextID NOTIFY textIDChanged)
  Q_PROPERTY(int trainerClassOrItemID READ getTrainerClassOrItemID WRITE setTrainerClassOrItemID RESET resetTrainerClassOrItemID NOTIFY trainerClassOrItemIDChanged)
  Q_PROPERTY(int trainerSetID READ getTrainerSetID WRITE setTrainerSetID RESET resetTrainerSetID NOTIFY trainerSetIDChanged)
  Q_PROPERTY(int missableIndex READ getMissableIndex WRITE setMissableIndex RESET resetMissableIndex NOTIFY missableIndexChanged)
  Q_PROPERTY(int yStepVector READ getYStepVector WRITE setYStepVector NOTIFY yStepVectorChanged)
  Q_PROPERTY(int xStepVector READ getXStepVector WRITE setXStepVector NOTIFY xStepVectorChanged)

public:
  /// Create a blank sprite or load one from a map.
  SpriteData(bool blankNPC = false,
             SaveFile* saveFile = nullptr,
             var8 index = 0);

  /// Create a sprite from map data.
  SpriteData(MapDBEntrySprite* entry);

  virtual ~SpriteData();

  void load(bool blankNPC = false,
            SaveFile* saveFile = nullptr,
            var8 index = 0);

  /// Loads preset from map data, useful if your not already in the map but
  /// want to re-create the sprites as they should be on said map.
  void load(MapDBEntrySprite* spriteData);

  void loadSpriteData1(SaveFile* saveFile,
            var8 index); ///< Expand the first sprite-data table for @p index.
  void loadSpriteData2(SaveFile* saveFile,
            var8 index); ///< Expand the second sprite-data table for @p index.
  void loadSpriteDataNPC(SaveFile* saveFile,
            var8 index); ///< Expand the NPC-only sprite data for @p index.
  void checkMissable(SaveFile* saveFile,
            var8 index); ///< Resolve whether this sprite is a missable.

  void save(SaveFile* saveFile,
            var8 index);
  void saveSpriteData1(SaveFile* saveFile,
            var8 index); ///< Flatten the first sprite-data table.
  void saveSpriteData2(SaveFile* saveFile,
            var8 index); ///< Flatten the second sprite-data table.
  void saveSpriteDataNPC(SaveFile* saveFile,
            var8 index); ///< Flatten the NPC-only sprite data.

  SpriteDBEntry* toSprite(); ///< Resolve this sprite to its DB entry.

  // Can't be Q_INVOKABLE for some reason, can't be placed into the
  // Qt Meta System
  static void saveMissables(SaveFile* saveFile,
                       QVector<SpriteData*> spriteData);

  // These allow QML to work with std::optional values including resetting them
  // They are interfaced with a Q_PROPERTY
  int getRangeDirByte();              ///< @see rangeDirByte property.
  void setRangeDirByte(int val);      ///< @see rangeDirByte property.
  void resetRangeDirByte();           ///< Clear the optional rangeDirByte.
  int getTextID();                    ///< @see textID property.
  void setTextID(int val);            ///< @see textID property.
  void resetTextID();                 ///< Clear the optional textID.
  int getTrainerClassOrItemID();      ///< @see trainerClassOrItemID property.
  void setTrainerClassOrItemID(int val); ///< @see trainerClassOrItemID property.
  void resetTrainerClassOrItemID();   ///< Clear the optional trainerClassOrItemID.
  int getTrainerSetID();              ///< @see trainerSetID property.
  void setTrainerSetID(int val);      ///< @see trainerSetID property.
  void resetTrainerSetID();           ///< Clear the optional trainerSetID.
  int getMissableIndex();             ///< @see missableIndex property.
  void setMissableIndex(int val);     ///< @see missableIndex property.
  void resetMissableIndex();          ///< Clear the optional missableIndex.

  // These allow QML to work with values that require a var8 as QML can only
  // read and write int's
  // They are interfaced with a Q_PROPERTY
  int getYStepVector();         ///< @see yStepVector property.
  void setYStepVector(int val); ///< @see yStepVector property.
  int getXStepVector();         ///< @see xStepVector property.
  void setXStepVector(int val); ///< @see xStepVector property.

signals:
  void pictureIDChanged();
  void movementStatusChanged();
  void imageIndexChanged();
  void faceDirChanged();
  void yDispChanged();
  void xDispChanged();
  void mapYChanged();
  void mapXChanged();
  void movementByteChanged();
  void grassPriorityChanged();
  void yStepVectorChanged();
  void xStepVectorChanged();
  void yPixelsChanged();
  void xPixelsChanged();
  void intraAnimationFrameCounterChanged();
  void animFrameCounterChanged();
  void walkAnimationCounterChanged();
  void movementDelayChanged();
  void imageBaseOffsetChanged();
  void rangeDirByteChanged();
  void textIDChanged();
  void trainerClassOrItemIDChanged();
  void trainerSetIDChanged();
  void missableIndexChanged();

public slots:
  void reset(bool blankNPC = false); ///< Blank this sprite (optionally as a blank NPC).
  static QVector<SpriteData*> randomizeAll(QVector<MapDBEntrySprite*> mapSprites); ///< Randomize a whole map's sprites.
  void randomize(QVector<TmpSpritePos*>* tmpPos); ///< Randomize this sprite (avoiding @p tmpPos clashes).

  void setTo(MapDBEntrySprite* spriteData); ///< Copy values from a map-defined sprite.
  static QVector<SpriteData*> setToAll(QVector<MapDBEntrySprite*> spriteSigns); ///< Build sprites from a map's sprite list.

public:
  /**
   * Sprite data that applies to all sprites
  */

  /// Actual sprite image shown
  int pictureID;

  /// (0: uninitialized, 1: ready, 2: delayed, 3: moving)
  /// Advised to use ready
  int movementStatus;

  /// Basically, in the sprite sheet strip, which "pane" or "tile" is it at
  /// 0xFF if not on the screen. Advised to use 0xFF
  int imageIndex;

  /// (0: down, 4: up, 8: left, $c: right)
  int faceDir;

  /// Important to set to 0x8
  /// Keep sprites from wandering too far however it's noted that it's bugged
  /// to begin with. Both are init to 0x8
  int yDisp;
  int xDisp;

  /// These are very important to set, just usual coordinates for map placement
  /// +4
  /// Coordinate 0,0 would be 4,4
  /// Coordinate 5,5 would be 9,9
  /// etc...
  /// Values less than 4 don't appear on the map
  int mapY;
  int mapX;

  /// (0xFF not moving, 0xFE random movements, others move without collision)
  int movementByte;

  /// (0x80 in grass, 0x00 otherwise) - Prioritizing grass drawn around sprite
  int grassPriority;

  /**
   * These actually don't really matter and can be zero
   * I'm not sure if the game even uses these values in the sav file when
   * loading. Regardless things work just fine if they're all zero
  */

  /// When the sprite moves, exactly how far or how much is that?
  /// (-1, 0 or 1)
  /// These need to be kept at var8 since they deal with reading and writing
  /// unsigned numbers that can intentionally underflow
  var8 yStepVector;
  var8 xStepVector;

  /// Screen position in pixels aligned to 4 pixels offset from the grid
  /// (To appear centered)
  int yPixels;
  int xPixels;

  /// Counter that helps delay between animation frames so things aren't so
  /// instant and fast
  int intraAnimationFrameCounter;

  /// Animation frame counter
  int animFrameCounter;

  /// Tracks movement & wandering, sprites are given 0x10 and it's decremented

  int walkAnimationCounter;

  /// Delay until next movement, counts downward and flags movementStatus ready
  /// once reached
  int movementDelay;

  /// Used to help compute imageIndex based on vram
  int imageBaseOffset;

  /*
   * Sprite data that applies to all non-player sprites
   * (All sprites that aren't sprite #0)
   * In reality though the game doesn't use any sprite data at all for the
   * player so none of this matters to begin with
  */

  /// How far a walking sprite can wander, or if still the facing direction
  /// A walking sprite having a value of 0 faces all directions
  /// There are certain special values that give special movement
  /// Notably 1-2 and D0-D3
  std::optional<int> rangeDirByte;

  /// Text id when this sprite is interacted with
  std::optional<int> textID;

  /// If this is an item sprite, the item id, otherwise the trainer class
  std::optional<int> trainerClassOrItemID;

  /// Trainer data id
  std::optional<int> trainerSetID;

  /**
   * Sprite data that applies to all non-player missable sprites
   * (All sprites that aren't sprite #0 and have an associated index in the
   * global missable list which determine if it's rendered or not)
  */

  /// If this is not null, then this sprite is a missable and it's appearance
  /// is determined by the flag in the global missable index this points to
  std::optional<int> missableIndex;
};
