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

#include "./areawarps.h"
#include "../fragments/warpdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/maps.h"
#include "../../../db/mapsearch.h"
#include "../../../../random.h"

AreaWarps::AreaWarps(SaveFile* saveFile)
{
  load(saveFile);
}

AreaWarps::~AreaWarps() {
  reset();
}

int AreaWarps::warpCount()
{
  return warps.size();
}

int AreaWarps::warpMax()
{
  return maxWarps;
}

WarpData* AreaWarps::warpAt(int ind)
{
  return warps.at(ind);
}

void AreaWarps::warpSwap(int from, int to)
{
  auto eFrom = warps.at(from);
  auto eTo = warps.at(to);

  warps.replace(from, eTo);
  warps.replace(to, eFrom);

  warpsChanged();
}

void AreaWarps::warpRemove(int ind)
{
  if(warps.size() <= 0)
    return;

  warps.removeAt(ind);
}

void AreaWarps::warpNew()
{
  if(warps.size() >= maxWarps)
    return;

  warps.append(new WarpData);
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

  warpsChanged();

  warpDest = toolset->getByte(0x26DB);
  warpDestChanged();

  dungeonWarpDestMap = toolset->getByte(0x29C9);
  dungeonWarpDestMapChanged();

  specialWarpDestMap = toolset->getByte(0x29C6);
  specialWarpDestMapChanged();

  whichDungeonWarp = toolset->getByte(0x29CA);
  whichDungeonWarpChanged();

  scriptedWarp = toolset->getBit(0x29D9, 1, 3);
  scriptedWarpChanged();

  isDungeonWarp = toolset->getBit(0x29D9, 1, 4);
  isDungeonWarpChanged();

  flyOrDungeonWarp = toolset->getBit(0x29DE, 1, 2);
  flyOrDungeonWarpChanged();

  flyWarp = toolset->getBit(0x29DE, 1, 3);
  flyWarpChanged();

  dungeonWarp = toolset->getBit(0x29DE, 1, 4);
  dungeonWarpChanged();

  skipJoypadCheckWarps = toolset->getBit(0x29DF, 1, 2);
  skipJoypadCheckWarpsChanged();

  warpedFromWarp = toolset->getByte(0x29E7);
  warpedFromWarpChanged();

  warpedfromMap = toolset->getByte(0x29E8);
  warpedfromMapChanged();
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
  scriptedWarpChanged();

  isDungeonWarp = false;
  isDungeonWarpChanged();

  skipJoypadCheckWarps = false;
  skipJoypadCheckWarpsChanged();

  warpDest = 0xFF;
  warpDestChanged();

  dungeonWarpDestMap = 0;
  dungeonWarpDestMapChanged();

  specialWarpDestMap = 0;
  specialWarpDestMapChanged();

  flyOrDungeonWarp = false;
  flyOrDungeonWarpChanged();

  flyWarp = false;
  flyWarpChanged();

  dungeonWarp = false;
  dungeonWarpChanged();

  whichDungeonWarp = 0;
  whichDungeonWarpChanged();

  warpedFromWarp = 0;
  warpedFromWarpChanged();

  warpedfromMap = 0;
  warpedfromMapChanged();

  for(auto warp : warps)
    delete warp;

  warps.clear();
  warpsChanged();
}

void AreaWarps::randomize(MapDBEntry* map)
{
  reset();

  // Pick random map that you came from
  auto dungeonWarp = MapsDB::search()->isGood()->isType("Cave")->pickRandom();

  // Assign index
  dungeonWarpDestMap = dungeonWarp->ind;
  dungeonWarpDestMapChanged();

  // Pick another random map for special warp destination
  specialWarpDestMap = MapsDB::search()->isGood()->pickRandom()->ind;
  specialWarpDestMapChanged();

  // Make up some random warps on said dungeon warp
  whichDungeonWarp = Random::rangeExclusive(0, dungeonWarp->warpOut.size());
  whichDungeonWarpChanged();

  warpedFromWarp = Random::rangeExclusive(0, dungeonWarp->warpOut.size());
  warpedFromWarpChanged();

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

  warpsChanged();
}
