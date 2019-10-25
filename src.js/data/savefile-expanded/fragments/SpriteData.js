.pragma library

/*
  * Sprite Data is one complicated section. Sprite data is scattered all across
  * The save file in many sorts of confusing setups and whatnot. It took a very
  * long time to understand sprite data and get it correct after much trial and
  * error and while it's correct a bit of it is still not fully understood.
*/

class SpriteData {

  /**
     * 0 arguments or false - Empty player character
     * 1 argument true - Empty NPC character
     * 2 arguments - Load data with savefile and index
     */
  constructor(savefile, index) {
    /**
       * Sprite data that applies to all sprites
       */

    // Actual sprite image shown
    this.pictureID = 0;

    // (0: uninitialized, 1: ready, 2: delayed, 3: moving)
    this.movementStatus = 0;

    // Basically, in the sprite sheet strip, which "pane" or "tile" is it at
    // 0xFF if not on the screen
    this.imageIndex = 0;

    // When the sprite moves, exactly how far or how much is that?
    //(-1, 0 or 1)
    this.yStepVector = 0;

    // Screen position in pixels aligned to 4 pixels offset from the grid (To appear centered)
    this.yPixels = 0;

    // When the sprite moves, exactly how far or how much is that?
    //(-1, 0 or 1)
    this.xStepVector = 0;

    // Screen position in pixels aligned to 4 pixels offset from the grid (To appear centered)
    this.xPixels = 0;

    // Counter that helps delay between animation frames so things aren't so instant and fast
    this.intraAnimationFrameCounter = 0;

    // Animation frame counter
    this.animFrameCounter = 0;

    // (0: down, 4: up, 8: left, $c: right)
    this.faceDir = 0;

    // Tracks movement & wandering, sprites are given 0x10 and it's decremented
    this.walkAnimationCounter = 0;

    // Keep sprites from wandering too far however it's noted that it's bugged
    // to begin with. Both are init to 0x8
    this.yDisp = 0;
    this.xDisp = 0;

    // Placement in 2x2 grid steps
    // Origin (Top or Left) has a value of 4
    this.mapY = 0;
    this.mapX = 0;

    // (0xFF not moving, 0xFE random movements, others unknown)
    this.movementByte = 0;

    // (0x80 in grass, 0x00 otherwise) - Prioritizing grass drawn around sprite
    this.grassPriority = 0;

    // Delay until next movement, counts downward and flags movementStatus ready
    // once reached
    this.movementDelay = 0;

    // Used to help compute imageIndex based on vram
    this.imageBaseOffset = 0;

    /**
       * Sprite data that applies to all non-player sprites
       * (All sprites that aren't sprite #0)
       */

    // How far a walking sprite can wander, or if still the facing direction
    // A walking sprite having a value of 0 faces all directions
    this.rangeDirByte = null;

    // Text id when this sprite is interacted with
    this.textID = null;

    // If this is an item sprite, the item id, otherwise the trainer class
    this.trainerClassOrItemID = null;

    // Trainer data id
    this.trainerSetID = null;

    /**
       * Sprite data that applies to all non-player missable sprites
       * (All sprites that aren't sprite #0 and have an associated index in the
       * global missable list which determine if it's rendered or not)
       */
    // If this is not null, then this sprite is a missable and it's appearance
    // is determined by the flag in the global missable index this points to
    this.missableIndex = null;

    if ((typeof savefile === "object") && savefile !== null)
      this.load(savefile, index);
    else if (savefile === true) {
      this.rangeDirByte = 0;
      this.textID = 0;
      this.trainerClassOrItemID = 0;
      this.trainerSetID = 0;
      this.missableIndex = -1;
    }
  }

  // Load data all sprites on the map have
  load(savefile, index) {

    // Grab sprite data 1 and 2 which applies to all sprites
    this.grabSpriteData1(savefile, index);
    this.grabSpriteData2(savefile, index);

    // If this isn't the player sprite then load additional non-player data
    // and check to see if it's missable
    if (index > 0) {
      this.grabSpriteDataNPC(savefile, index);
      this.checkMissable(savefile, index);
    }
  }

  // Don't correct index, this data starts at sprite 0
  grabSpriteData1(saveFile, index) {

    const it = saveFile.iterator.offsetTo((0x10 * index) + 0x2D2C);
    this.pictureID = it.getByte();
    this.movementStatus = it.getByte();
    this.imageIndex = it.getByte();
    this.yStepVector = it.getByte();
    this.yPixels = it.getByte();
    this.xStepVector = it.getByte();
    this.xPixels = it.getByte();
    this.intraAnimationFrameCounter = it.getByte();
    this.animFrameCounter = it.getByte();
    this.faceDir = it.getByte();
  }

  // Don't correct index, this data starts at sprite 0
  grabSpriteData2(saveFile, index) {

    const it = saveFile.iterator.offsetTo((0x10 * index) + 0x2E2C);
    this.walkAnimationCounter = it.getByte(1);
    this.yDisp = it.getByte();
    this.xDisp = it.getByte();
    this.mapY = it.getByte();
    this.mapX = it.getByte();
    this.movementByte = it.getByte();
    this.grassPriority = it.getByte();
    this.movementDelay = it.getByte(5);
    this.imageBaseOffset = it.getByte();
  }

  // Correct index, this data starts sprite 0 at sprite 1
  grabSpriteDataNPC(saveFile, index) {
    index--;

    // Init missable index to non-player value
    this.missableIndex = -1;

    const it = saveFile.iterator.offsetTo((2 * index) + 0x2790);
    this.rangeDirByte = it.getByte();
    this.textID = it.getByte();

    it.offsetTo((2 * index) + 0x27B0);
    this.trainerClassOrItemID = it.getByte();
    this.trainerSetID = it.getByte();
  }

  // Scan missable list to see if this sprite is in it
  // If a match is found load missable data making this sprite a missable
  // sprite and therefore have it's appearance controlled by the global
  // missable flags

  // Don't correct index, this data starts at sprite 1
  checkMissable(saveFile, index) {

    const it = saveFile.iterator;

    for (let i = 0; i < 17; i++) {
      it.offsetTo((0x2 * i) + 0x287A);
      const mId = it.getByte();
      const mIndex = it.getByte();

      // Stop when reach list terminator
      if (mId == 0xFF)
      break;

      // Skip if the id doesn't match this sprite
      if (mId !== index)
      continue;

      // Save bit index this missable refers to and add to missable
      // array for quick reference
      this.missableIndex = mIndex;
      break;
    }
  }

  // Missables will have to be called seperate
  save(savefile, index) {

    // Grab sprite data 1 and 2 which applies to all sprites
    this.saveSpriteData1(savefile, index);
    this.saveSpriteData2(savefile, index);

    // If this isn't the player sprite then save NPC data
    if (index > 0) {
      this.saveSpriteDataNPC(savefile, index);
    }
  }

  saveSpriteData1(saveFile, index) {

    const it = saveFile.iterator.offsetTo((0x10 * index) + 0x2D2C);
    it.setByte(this.pictureID)
    it.setByte(this.movementStatus)
    it.setByte(this.imageIndex)
    it.setByte(this.yStepVector)
    it.setByte(this.yPixels)
    it.setByte(this.xStepVector)
    it.setByte(this.xPixels)
    it.setByte(this.intraAnimationFrameCounter)
    it.setByte(this.animFrameCounter)
    it.setByte(this.faceDir)
  }

  saveSpriteData2(saveFile, index) {

    const it = saveFile.iterator.offsetTo((0x10 * index) + 0x2E2C);
    it.setByte(this.walkAnimationCounter, 1);
    it.setByte(this.yDisp)
    it.setByte(this.xDisp)
    it.setByte(this.mapY)
    it.setByte(this.mapX)
    it.setByte(this.movementByte)
    it.setByte(this.grassPriority)
    it.setByte(this.movementDelay, 5);
    it.setByte(this.imageBaseOffset)
  }

  saveSpriteDataNPC(saveFile, index) {

    // Correct index, this data starts sprite 0 at sprite 1
    index--;
    const it = saveFile.iterator.offsetTo((2 * index) + 0x2790);
    it.setByte(this.rangeDirByte);
    it.setByte(this.textID);

    it.offsetTo((2 * index) + 0x27B0);
    it.setByte(this.trainerClassOrItemID);
    it.setByte(this.trainerSetID);
  }

  // Because missables is somewhat tricky, it needs to be called seperately
  static saveMissables(saveFile, spriteData) {
    const it = saveFile.iterator;

    it.offsetTo(0x287A);
    for (let i = 0; i < spriteData.length && i < 16; i++) {
      const val = spriteData[i];

      // Skip all sprites that aren't missables
      if (val.missableIndex === null || !(val.missableIndex >= 0))
      continue;

      // Save sprite index that's missable
      // Don't correct index, this data starts at sprite 1
      it.setByte(i);

      // Save missable index this sprite connects to
      it.setByte(val.missableIndex);
    }
    it.setByte(0xFF);
  }
}
