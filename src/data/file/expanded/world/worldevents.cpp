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

#include "./worldevents.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

WorldEvents::WorldEvents(SaveFile* saveFile)
{
  load(saveFile);
}

WorldEvents::~WorldEvents() {}

void WorldEvents::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  completedEvents = toolset->getBitField(0x29F3, 320);
}

void WorldEvents::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;
  toolset->setBitField(0x29F3, 320, completedEvents);
}

void WorldEvents::reset()
{
  completedEvents.clear();
}

// Competed world events is too complicated to make happen randomly right now
// Furthermore we do want the player to still play the game fully when
// randomized
void WorldEvents::randomize() {}
