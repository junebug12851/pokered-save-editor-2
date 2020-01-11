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

enum class SpriteMovementStatus : var8
{
  UnInit = 0,
  Ready,
  Delayed,
  Moving
};

enum class SpriteFacing : var8
{
  Down = 4 * 0,
  Up = 4 * 1,
  Left = 4 * 2,
  Right = 4 * 3
};

class SpriteData : ExpandedInterface
{
public:
  SpriteData(bool blankNPC = false,
             SaveFile* saveFile = nullptr,
             var8 index = 0);

  virtual ~SpriteData();

  void load(bool blankNPC = false,
            SaveFile* saveFile = nullptr,
            var8 index = 0);

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
  void randomize();

  SpriteDBEntry* toSprite();

  var8 pictureID;
  var8 movementStatus;
  var8 imageIndex;
  var8 yStepVector;
  var8 yPixels;
  var8 xStepVector;
  var8 xPixels;
  var8 intraAnimationFrameCounter;
  var8 animFrameCounter;
  var8 faceDir;
  var8 walkAnimationCounter;
  var8 yDisp;
  var8 xDisp;
  var8 mapY;
  var8 mapX;
  var8 movementByte;
  var8 grassPriority;
  var8 movementDelay;
  var8 imageBaseOffset;
  std::optional<var8> rangeDirByte;
  std::optional<var8> textID;
  std::optional<var8> trainerClassOrItemID;
  std::optional<var8> trainerSetID;
  std::optional<var8> missableIndex;

private:
  // To surpress warnings with using the ExpandedInterface contract
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
};

#endif // SPRITEDATA_H
