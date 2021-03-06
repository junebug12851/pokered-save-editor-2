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
#include "./area.h"
#include "./areaaudio.h"
#include "./arealoadedsprites.h"
#include "./areageneral.h"
#include "./areamap.h"
#include "./areanpc.h"
#include "./areaplayer.h"
#include "./areapokemon.h"
#include "./areasign.h"
#include "./areasprites.h"
#include "./areatileset.h"
#include "./areawarps.h"
#include "../../savefile.h"
#include <pse-db/mapsdb.h>
#include <pse-db/util/mapsearch.h>
#include <pse-common/random.h>

Area::Area(SaveFile* saveFile)
{
  audio = new AreaAudio;
  preloadedSprites = new AreaLoadedSprites;
  general = new AreaGeneral;
  map = new AreaMap;
  npc = new AreaNPC;
  player = new AreaPlayer;
  pokemon = new AreaPokemon;
  signs = new AreaSign;
  sprites = new AreaSprites;
  tileset = new AreaTileset;
  warps = new AreaWarps;

  load(saveFile);
}

Area::~Area()
{
  audio->deleteLater();
  preloadedSprites->deleteLater();
  general->deleteLater();
  map->deleteLater();
  npc->deleteLater();
  player->deleteLater();
  pokemon->deleteLater();
  signs->deleteLater();
  sprites->deleteLater();
  tileset->deleteLater();
  warps->deleteLater();
}

void Area::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  audio->load(saveFile);
  preloadedSprites->load(saveFile);
  general->load(saveFile);
  map->load(saveFile);
  npc->load(saveFile);
  player->load(saveFile);
  pokemon->load(saveFile);
  signs->load(saveFile);
  sprites->load(saveFile);
  tileset->load(saveFile);
  warps->load(saveFile);
}

void Area::save(SaveFile* saveFile)
{
  audio->save(saveFile);
  preloadedSprites->save(saveFile);
  general->save(saveFile);
  map->save(saveFile);
  npc->save(saveFile);
  player->save(saveFile);
  pokemon->save(saveFile);
  signs->save(saveFile);
  sprites->save(saveFile);
  tileset->save(saveFile);
  warps->save(saveFile);
}

void Area::reset()
{
  audio->reset();
  preloadedSprites->reset();
  general->reset();
  map->reset();
  npc->reset();
  player->reset();
  pokemon->reset();
  signs->reset();
  sprites->reset();
  tileset->reset();
  warps->reset();
}

void Area::randomize()
{
  // A Gameboy area is much more complicated to randomize and get right
  // so that it's playable
  // Pre-pick a random area here and pass to other area classes who need it

  // Grab a random "good" map, a good map to throw the player on
  auto map = MapsDB::search()->isGood()->pickRandom();

  // Now pick out a random warp in and use those coordinates for the player
  // coordinates
  auto warpIn = map->warpIn.at(Random::rangeExclusive(0, map->warpIn.size()));

  // X & Y coordinates to place player
  int x = warpIn->x;
  int y = warpIn->y;

  // Do the individual area randomizations and pass along map data where needed
  audio->randomize();
  general->randomize();
  npc->randomize();
  pokemon->randomize();

  preloadedSprites->randomize(map, x, y);
  this->map->randomize(map, x, y);
  player->randomize(x, y);
  signs->randomize(map);
  sprites->randomize(map->sprites);
  tileset->loadFromData(map, true);
  warps->randomize(map);
}

void Area::setTo(MapDBEntry* map)
{
  // Now pick out a random warp in and use those coordinates for the player
  // coordinates
  auto warpIn = (map == nullptr || map->warpIn.size() == 0)
      ? nullptr
      : map->warpIn.at(Random::rangeExclusive(0, map->warpIn.size()));

  // X & Y coordinates to place player
  int x = (warpIn == nullptr)
      ? 0
      : warpIn->x;

  int y = (warpIn == nullptr)
      ? 0
      : warpIn->y;

  // Do the individual area randomizations and pass along map data where needed
  audio->setTo(map);
  general->setTo(map);
  npc->setTo(map);
  pokemon->setTo(map);
  preloadedSprites->setTo(map, x, y);
  this->map->setTo(map, x, y);
  player->setTo(map, x, y);
  signs->setTo(map);
  sprites->setTo(map);
  tileset->loadFromData(map, false);
  warps->setTo(map);
}
