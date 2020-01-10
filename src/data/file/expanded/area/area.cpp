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
#include "../../savefile.h"
#include "../../../db/maps.h"

#include <QRandomGenerator>

Area::Area(SaveFile* saveFile)
{
  audio = new AreaAudio;
  preloadedSprites = new AreaLoadedSprites;
  general = new AreaGeneral;
  map = new AreaMap;
  npc = new AreaNPC;
  player = new AreaPlayer;
  pokemon = new AreaPokemon;

  load(saveFile);
}

Area::~Area()
{
  delete audio;
  delete preloadedSprites;
  delete general;
  delete map;
  delete npc;
  delete player;
  delete pokemon;
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
}

void Area::randomize()
{
  // A Gameboy area is much more complicated to randomize and get right
  // so that it's playable
  // Pre-pick a random area here and pass to other area classes who need it
  auto rnd = QRandomGenerator::global();

  // Grab a random map
  auto map = MapsDB::store.at(rnd->bounded(0, MapsDB::store.size()));

  // Keep going through maps until we find:
  // * A normal non-special or glitch map
  // * A map that's complete (Not an incomplete map)
  // * Has at least one warp in and out (You have to be able to enter and leave)
  //     > We'll use this to position the player correctly on the map
  // * Is not the strange elevator that has an invalid warp
  while(map->glitch ||
        map->special ||
        map->incomplete != "" ||
        map->warpIn.size() == 0 ||
        map->warpOut.size() == 0 ||
        map->name == "Silph Co Elevator")
    map = MapsDB::store.at(rnd->bounded(0, MapsDB::store.size() - 1));

  // Now pick out a random warp in and use those coordinates for the player
  // coordinates
  auto warpIn = map->warpIn.at(rnd->bounded(0, map->warpIn.size()));

  // X & Y coordinates to place player
  var8 x = warpIn->x;
  var8 y = warpIn->y;

  // Do the individual area randomizations and pass along map data where needed
  audio->randomize();
  general->randomize();
  npc->randomize();
  pokemon->randomize();

  preloadedSprites->randomize(map, x, y);
  this->map->randomize(map, x, y);
  player->randomize(x, y);
}
