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
#include <string.h>

#include "./worldscripts.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/scripts.h>

WorldScripts::WorldScripts(SaveFile* saveFile)
{
  load(saveFile);
}

WorldScripts::~WorldScripts() {}

void WorldScripts::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto it = saveFile->iterator()->offsetTo(0x289C);

  for(auto scriptEntry : ScriptsDB::store) {
    var16 val;

    if(scriptEntry->size == 1)
      val = it->getByte();
    else
      val = it->getWord();

    curScripts[scriptEntry->ind] = val;

    if(scriptEntry->skip)
      it->skipPadding(*scriptEntry->skip);
  }

  curScriptsChanged();

  delete it;
}

void WorldScripts::save(SaveFile* saveFile)
{
  auto it = saveFile->iterator()->offsetTo(0x289C);

  for(auto scriptEntry : ScriptsDB::store) {
    var16 val = curScripts[scriptEntry->ind];

    if(scriptEntry->size == 1)
      it->setByte(val);
    else
      it->setWord(val);

    if(scriptEntry->skip)
      it->skipPadding(*scriptEntry->skip);
  }

  delete it;
}

int WorldScripts::scriptsCount()
{
  return scriptCount;
}

int WorldScripts::scriptsAt(int ind)
{
  return curScripts[ind];
}

void WorldScripts::scriptsSet(int ind, int val)
{
  curScripts[ind] = val;
  curScriptsChanged();
}

void WorldScripts::reset()
{
  memset(curScripts, 0, scriptCount * 2); // 2 * 8-bit widths
  curScriptsChanged();
}

// We want the player to start from the beginning
void WorldScripts::randomize()
{
  reset();
}
