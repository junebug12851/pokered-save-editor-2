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
 * @file areawarps.cpp
 * @brief Implementation of AreaWarps -- the map's doors plus the live warp state.
 *        See areawarps.h for the documented API, and notes/reference/warps.md for what
 *        every one of these bytes actually is.
 */

#include "./areawarps.h"
#include "../../qmlownership.h"
#include "../fragments/warpdata.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/util/mapsearch.h>
#include <pse-common/random.h>

// ── The two ROM tables, transcribed ────────────────────────────────────────────────────
//
// data/maps/special_warps.asm. Both are read by PrepareForSpecialWarp with NO bounds check,
// and FlyWarpDataPtr has no terminator at all -- so these are not "recommended" values, they
// are the only ones a console has an answer for. Map ids from constants/map_constants.asm.
//
// tst_warps cross-checks every id below against maps.json by NAME, so a typo here cannot hide.

/// `FlyWarpDataPtr` -- the 13 maps a Fly / special warp may name.
static const QVector<int> kFlyMaps = {
  0,   // PALLET_TOWN
  1,   // VIRIDIAN_CITY
  2,   // PEWTER_CITY
  3,   // CERULEAN_CITY
  4,   // LAVENDER_TOWN
  5,   // VERMILION_CITY
  6,   // CELADON_CITY
  7,   // FUCHSIA_CITY
  8,   // CINNABAR_ISLAND
  9,   // INDIGO_PLATEAU
  10,  // SAFFRON_CITY
  15,  // ROUTE_4   -- the Mt. Moon Poke Center. The game Flies you to a ROUTE, not a town.
  21,  // ROUTE_10  -- the Rock Tunnel Poke Center.
};

/// `DungeonWarpList` -- the 12 legal (map, hole) pairs. Hole numbers are **1-based**.
static const QVector<QPair<int, int>> kDungeonWarps = {
  {159, 1}, {159, 2},  // SEAFOAM_ISLANDS_B1F
  {160, 1}, {160, 2},  // SEAFOAM_ISLANDS_B2F
  {161, 1}, {161, 2},  // SEAFOAM_ISLANDS_B3F
  {162, 1}, {162, 2},  // SEAFOAM_ISLANDS_B4F
  {194, 2},            // VICTORY_ROAD_2F   -- note: a hole 2 and NO hole 1.
  {165, 1}, {165, 2},  // POKEMON_MANSION_1F
  {214, 3},            // POKEMON_MANSION_2F
};

const QVector<int>& AreaWarps::legalFlyMaps()
{
  return kFlyMaps;
}

bool AreaWarps::isLegalFlyMap(int map)
{
  return kFlyMaps.contains(map);
}

const QVector<QPair<int, int>>& AreaWarps::legalDungeonWarps()
{
  return kDungeonWarps;
}

bool AreaWarps::isLegalDungeonWarp(int map, int which)
{
  return kDungeonWarps.contains(QPair<int, int>(map, which));
}

AreaWarps::AreaWarps(SaveFile* saveFile)
{
  load(saveFile);
}

AreaWarps::~AreaWarps() {
  for(auto warp : warps)
    warp->deleteLater();
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
  return qmlCppOwned(warps.at(ind));
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

  warps.at(ind)->deleteLater();
  warps.removeAt(ind);
  warpsChanged();
}

void AreaWarps::warpNew()
{
  if(warps.size() >= maxWarps)
    return;

  warps.append(new WarpData);
  warpsChanged();
}

void AreaWarps::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // wNumberOfWarps ($D3AE), then wWarpEntries ($D3AF): 32 x { Y, X, destWarp, destMap }.
  for (var8 i = 0; i < toolset->getByte(0x265A) && i < maxWarps; i++) {
    warps.append(new WarpData(saveFile, i));
  }

  warpsChanged();

  warpDest = toolset->getByte(0x26DB);          // wDestinationWarpID   $D42F
  warpDestChanged();

  dungeonWarpDestMap = toolset->getByte(0x29C9); // wDungeonWarpDestinationMap $D71D
  dungeonWarpDestMapChanged();

  specialWarpDestMap = toolset->getByte(0x29C6); // wDestinationMap      $D71A
  specialWarpDestMapChanged();

  whichDungeonWarp = toolset->getByte(0x29CA);   // wWhichDungeonWarp    $D71E
  whichDungeonWarpChanged();

  // wStatusFlags3 ($D72D) -- ⚠️ the console zeroes this WHOLE byte on every save load.
  scriptedWarp = toolset->getBit(0x29D9, 1, 3);  // BIT_WARP_FROM_CUR_SCRIPT
  scriptedWarpChanged();

  isDungeonWarp = toolset->getBit(0x29D9, 1, 4); // BIT_ON_DUNGEON_WARP
  isDungeonWarpChanged();

  // wStatusFlags6 ($D732)
  flyOrDungeonWarp = toolset->getBit(0x29DE, 1, 2); // BIT_FLY_OR_DUNGEON_WARP
  flyOrDungeonWarpChanged();

  flyWarp = toolset->getBit(0x29DE, 1, 3);       // BIT_FLY_WARP
  flyWarpChanged();

  dungeonWarp = toolset->getBit(0x29DE, 1, 4);   // BIT_DUNGEON_WARP
  dungeonWarpChanged();

  escapeWarp = toolset->getBit(0x29DE, 1, 6);    // BIT_ESCAPE_WARP -- Dig / Escape Rope / blackout.
  escapeWarpChanged();                           // (Was AreaMap::blackoutDest. It never was one.)

  // wStatusFlags7 ($D733)
  forcedWarp = toolset->getBit(0x29DF, 1, 2);    // BIT_FORCED_WARP
  forcedWarpChanged();

  // 💀 Both written by the game on every warp; read by nothing, anywhere.
  warpedFromWarp = toolset->getByte(0x29E7);     // wWarpedFromWhichWarp $D73B
  warpedFromWarpChanged();

  warpedfromMap = toolset->getByte(0x29E8);      // wWarpedFromWhichMap  $D73C
  warpedfromMapChanged();
}

void AreaWarps::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x265A, warps.size());

  for (var8 i = 0; i < warps.size() && i < maxWarps; i++) {
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
  toolset->setBit(0x29DE, 1, 6, escapeWarp);
  toolset->setBit(0x29DF, 1, 2, forcedWarp);
  toolset->setByte(0x29E7, warpedFromWarp);
  toolset->setByte(0x29E8, warpedfromMap);
}

void AreaWarps::reset()
{
  scriptedWarp = false;
  scriptedWarpChanged();

  isDungeonWarp = false;
  isDungeonWarpChanged();

  forcedWarp = false;
  forcedWarpChanged();

  warpDest = 0xFF; // "don't move the player" -- what PrepareForSpecialWarp itself writes.
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

  escapeWarp = false;
  escapeWarpChanged();

  whichDungeonWarp = 0;
  whichDungeonWarpChanged();

  warpedFromWarp = 0;
  warpedFromWarpChanged();

  warpedfromMap = 0;
  warpedfromMapChanged();

  for(auto warp : warps)
    warp->deleteLater();

  warps.clear();
  warpsChanged();
}

/**
 * @brief Rebuild the map's doors from @p map. **Not one state byte is touched.**
 *
 * ⚠️ This used to invent `dungeonWarpDestMap`, `specialWarpDestMap`, `whichDungeonWarp` and
 * `warpedFromWarp` **at random** -- and three of the four were values no console has a table
 * entry for (an arbitrary cave, an arbitrary map, a 0-based hole index where the game's are
 * 1-based). It was dormant only because MapsDB is never deep-linked at boot.
 *
 * A map has **no opinion** about where your last Fly went, which hole you last fell down, or
 * where you blacked out. Those are the *player's* state, they belong to whatever save is
 * loaded, and "place the player on this map" has no business rewriting them. So it doesn't.
 *
 * @see notes/reference/warps.md §5.
 */
void AreaWarps::setTo(MapDBEntry* map)
{
  // Clear the DOORS only. The warp-transition state is the player's, not the map's.
  for(auto warp : warps)
    warp->deleteLater();

  warps.clear();

  if(map == nullptr) {
    warpsChanged();
    return;
  }

  for(auto warpDataEntry : map->getWarpOut())
    warps.append(new WarpData(warpDataEntry));

  warpsChanged();
}

/**
 * @brief Randomize where this map's doors lead -- and **only** legal values, everywhere.
 *
 * The warp list is re-aimed (a "return" warp -- `destMap == 0xFF`, "back outside" -- is left
 * alone, because rewriting it would strand the player indoors). The warp *state* is left
 * exactly as `setTo()` leaves it: untouched. @see setTo.
 */
void AreaWarps::randomize(MapDBEntry* map)
{
  setTo(map);

  for(auto warp : warps) {
    // Return warps take you back outside. Leave them be.
    if(warp->destMap != 0xFF)
      warp->randomize();
  }

  warpsChanged();
}
