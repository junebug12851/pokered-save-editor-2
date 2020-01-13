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

#include <QRandomGenerator>

#include "./areawarps.h"
#include "../fragments/warpdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"

AreaWarps::AreaWarps(SaveFile* saveFile)
{
  load(saveFile);
}

AreaWarps::~AreaWarps() {
  reset();
}

void AreaWarps::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  for (var8 i = 0; i < toolset->getByte(0x265A) && i < 32; i++) {
    warps.append(new WarpData(saveFile, i));
  }

  warpDest = toolset->getByte(0x26DB);
  dungeonWarpDestMap = toolset->getByte(0x29C9);
  specialWarpDestMap = toolset->getByte(0x29C6);
  whichDungeonWarp = toolset->getByte(0x29CA);
  scriptedWarp = toolset->getBit(0x29D9, 1, 3);
  isDungeonWarp = toolset->getBit(0x29D9, 1, 4);
  flyOrDungeonWarp = toolset->getBit(0x29DE, 1, 2);
  flyWarp = toolset->getBit(0x29DE, 1, 3);
  dungeonWarp = toolset->getBit(0x29DE, 1, 4);
  skipJoypadCheckWarps = toolset->getBit(0x29DF, 1, 2);
  warpedFromWarp = toolset->getByte(0x29E7);
  warpedfromMap = toolset->getByte(0x29E8);
}

void AreaWarps::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x265A, warps.size());

  for (var8 i = 0; i < warps.size() && i < 32; i++) {
    warps.at(i)->save(saveFile, i);
  }

  toolset->setByte(0x26DB, warpDest);
  toolset->setByte(0x29C9, dungeonWarpDestMap);
  toolset->setByte(0x29C6, specialWarpDestMap);
  toolset->setByte(0x29CA, whichDungeonWarp);
  toolset->setBit(0x29D9, 1, 3, scriptedWarp);
  toolset->setBit(0x29D9, 1, 4, isDungeonWarp);
  toolset->setBit(0x29DE, 1, 2, flyOrDungeonWarp);
  toolset->setBit(0x29DE, 1, 3, flyWarp);
  toolset->setBit(0x29DE, 1, 4, dungeonWarp);
  toolset->setBit(0x29DF, 1, 2, skipJoypadCheckWarps);
  toolset->setByte(0x29E7, warpedFromWarp);
  toolset->setByte(0x29E8, warpedfromMap);
}

void AreaWarps::reset()
{
  scriptedWarp = false;
  isDungeonWarp = false;
  skipJoypadCheckWarps = false;
  warpDest = 0xFF;
  dungeonWarpDestMap = 0;
  specialWarpDestMap = 0;
  flyOrDungeonWarp = false;
  flyWarp = false;
  dungeonWarp = false;
  whichDungeonWarp = 0;
  warpedFromWarp = 0;
  warpedfromMap = 0;

  for(auto warp : warps)
    delete warp;

  warps.clear();
}

void AreaWarps::randomize(MapDBEntry* map)
{
  reset();

  auto rnd = QRandomGenerator::global();

  // Pick random map that you came from
  auto dungeonWarp = MapsDB::randomGoodMap();

  // Assign index
  dungeonWarpDestMap = dungeonWarp->ind;

  // Pick another random map for special warp destination
  specialWarpDestMap = MapsDB::randomGoodMap()->ind;

  // Make up some random warps on said dungeon warp
  whichDungeonWarp = rnd->bounded(0, dungeonWarp->warpOut.size());
  warpedFromWarp = rnd->bounded(0, dungeonWarp->warpOut.size());

  // Re-create all map warps out, we can't blantly make-up stuff here
  // and have to be careful
  for(auto warpDataEntry : map->warpOut) {
    auto tmp = new WarpData(warpDataEntry);

    // Randomize it so long as it's not a return warp
    // Return warps return you back outside, for now we'll leave them as they
    // are
    if(tmp->destMap != 0xFF)
      tmp->randomize();

    warps.append(tmp);
  }
}

// Not to use
void AreaWarps::randomize() {}
