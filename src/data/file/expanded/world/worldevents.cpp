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
#include <string.h>

#include "./worldevents.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/events.h"

WorldEvents::WorldEvents(SaveFile* saveFile)
{
  load(saveFile);
}

WorldEvents::~WorldEvents() {}

int WorldEvents::eventsCount()
{
  return eventCount;
}

bool WorldEvents::eventsAt(int ind)
{
  return completedEvents[ind];
}

void WorldEvents::eventsSet(int ind, bool val)
{
  completedEvents[ind] = val;
  completedEventsChanged();
}

void WorldEvents::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  for(var16 i = 0; i < EventsDB::store.size(); i++) {
    auto event = EventsDB::store.at(i);
    completedEvents[i] = toolset->getBit(event->byte, 1, event->bit);
  }

  completedEventsChanged();
}

void WorldEvents::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  for(var16 i = 0; i < EventsDB::store.size(); i++) {
    auto event = EventsDB::store.at(i);
    toolset->setBit(event->byte, 1, event->bit, completedEvents[i]);
  }
}

void WorldEvents::reset()
{
  memset(completedEvents, 0, eventCount);
  completedEventsChanged();
}

// Competed world events is too complicated to make happen randomly right now
// Furthermore we do want the player to still play the game fully when
// randomized
void WorldEvents::randomize() {
  reset();
}
