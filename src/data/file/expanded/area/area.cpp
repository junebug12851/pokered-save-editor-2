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
#include "../../savefile.h"
#include "../../../db/maps.h"

#include <QRandomGenerator>

Area::Area(SaveFile* saveFile)
{
  areaAudio = new AreaAudio;
  areaLoadedSprites = new AreaLoadedSprites;

  load(saveFile);
}

Area::~Area()
{
  delete areaAudio;
  delete areaLoadedSprites;
}

void Area::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  areaAudio->load(saveFile);
  areaLoadedSprites->load(saveFile);
}

void Area::save(SaveFile* saveFile)
{
  areaAudio->save(saveFile);
  areaLoadedSprites->save(saveFile);
}

void Area::reset()
{
  areaAudio->reset();
  areaLoadedSprites->reset();
}

void Area::randomize()
{
  // A Gameboy area is much more complicated to randomize and get right
  // so that it's playable
  // Pre-pick a random area here and pass to other area classes who need it
  auto rnd = QRandomGenerator::global();

  // Get a random map
  auto map = MapsDB::store.at(rnd->bounded(0, MapsDB::store.size()));

  // Keep going through maps until we find a normal map that has at least one
  // warp in or out and is not the strange elevator that has an invalid warp
  while(map->glitch ||
        map->special ||
        map->incomplete != "" ||
        map->warpIn.size() == 0 ||
        map->warpOut.size() == 0 ||
        map->name == "Silph Co Elevator")
    map = MapsDB::store.at(rnd->bounded(0, MapsDB::store.size() - 1));

  // These don't really need the pre-chosen map
  // Loaded Sprites kind of does but we'll see how it goes with it being random
  areaAudio->randomize();
  areaLoadedSprites->randomize();
}
