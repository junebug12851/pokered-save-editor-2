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
#include "./world.h"
#include "./worldcompleted.h"
#include "./worldevents.h"
#include "../../savefile.h"

World::World(SaveFile* saveFile)
{
  completed = new WorldCompleted;
  events = new WorldEvents;

  load(saveFile);
}

World::~World()
{
  delete completed;
  delete events;
}

void World::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  completed->load(saveFile);
  events->load(saveFile);
}

void World::save(SaveFile* saveFile)
{
  completed->save(saveFile);
  events->save(saveFile);
}

void World::reset()
{
  completed->reset();
  events->reset();
}

void World::randomize()
{
  completed->randomize();
  events->randomize();
}
