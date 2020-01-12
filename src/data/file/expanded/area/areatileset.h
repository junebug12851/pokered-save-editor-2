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
#ifndef AREATILESET_H
#define AREATILESET_H

#include "../expandedinterface.h"
#include "../../../../common/types.h"

class SaveFile;
struct MapDBEntry;

class AreaTileset : ExpandedInterface
{
public:
  AreaTileset(SaveFile* saveFile = nullptr);
  virtual ~AreaTileset();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();
  void loadFromData(MapDBEntry* map, bool randomType = false);

  // Which tileset to use. Changing this will make the map
  // unplayable unless you know what your doing.
  var8 current;

  // Which tiles can you talk over when 1 tile sits between you and an NPC
  // Mainly used for Pokemon Centers
  var8 talkingOverTiles[3];

  // Which tile is a grass tile? In testing I got odd results changing this
  var8 grassTile;

  // These are somewhat abstract, they're used when your adjancent to boulders
  // or when your interacting/interacted with boulders. I don't think I've seen
  // them used in a sav file
  var8 boulderIndex;
  var8 boulderColl;

  // This is something that can be freely changed, there are 3 tilesets and they
  // are treated differently on gameplay. "Outside" animates flower and water
  // tiles replacing whatever tiles were there so changing SS anne to outside
  // causes the windows to be animated flowers ~_^ but the types do other stuff
  // as well
  var8 type;

  // Tileset location, bank and pointers. Changing these will make the map
  // unplayable unless you know what your doing.
  // For a tileset
  // * GFX & Blocks are always on the same bank, the bank number tells you where
  // * Collision is always in bank 0 (Home Bank)
  var8 bank;
  var16 blockPtr;
  var16 gfxPtr;
  var16 collPtr;
};

#endif // AREATILESET_H
