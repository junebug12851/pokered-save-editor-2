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

SaveFileExpanded::SaveFileExpanded(SaveFile* saveFile)
{
  player = new Player;
  area = new Area;

  load(saveFile);
}

SaveFileExpanded::~SaveFileExpanded()
{
  delete player;
  delete area;
}

void SaveFileExpanded::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    reset();

  player->load(saveFile);
  area->load(saveFile);
}

void SaveFileExpanded::save(SaveFile* saveFile)
{
  player->save(saveFile);
  area->save(saveFile);
}

void SaveFileExpanded::reset()
{
  player->reset();
  area->reset();
}

void SaveFileExpanded::randomize()
{
  player->randomize();
  area->randomize();
}
