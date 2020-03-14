/*
  * Copyright 2019 June Hanabi
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
#include "savefileexpanded.h"
#include "../savefile.h"

#include "./player/player.h"
#include "./area/area.h"
#include "./world/world.h"
#include "./daycare.h"
#include "./halloffame.h"
#include "./rival.h"
#include "./storage.h"

SaveFileExpanded::SaveFileExpanded(SaveFile* saveFile)
{
  player = new Player;
  area = new Area;
  world = new World;
  daycare = new Daycare;
  hof = new HallOfFame;
  rival = new Rival;
  storage = new Storage;

  load(saveFile);
}

SaveFileExpanded::~SaveFileExpanded()
{
  player->deleteLater();
  area->deleteLater();
  world->deleteLater();
  daycare->deleteLater();
  hof->deleteLater();
  rival->deleteLater();
  storage->deleteLater();
}

void SaveFileExpanded::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  player->load(saveFile);
  area->load(saveFile);
  world->load(saveFile);
  daycare->load(saveFile);
  hof->load(saveFile);
  rival->load(saveFile);
  storage->load(saveFile);
}

void SaveFileExpanded::save(SaveFile* saveFile)
{
  player->save(saveFile);
  area->save(saveFile);
  world->save(saveFile);
  daycare->save(saveFile);
  hof->save(saveFile);
  rival->save(saveFile);
  storage->save(saveFile);
}

void SaveFileExpanded::reset()
{
  player->reset();
  area->reset();
  world->reset();
  daycare->reset();
  hof->reset();
  rival->reset();
  storage->reset();
}

void SaveFileExpanded::randomize()
{
  player->randomize();
  area->randomize();
  world->randomize();
  daycare->randomize(player->basics);
  hof->randomize();
  rival->randomize();
  storage->randomize(player->basics);
}
