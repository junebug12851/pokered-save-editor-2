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

#include "./pokemonstorageset.h"
#include "./pokemonstoragebox.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

PokemonStorageSet::PokemonStorageSet(SaveFile* saveFile, var16 boxesOffset, svar8 skipInd)
{
  for(var8 i = 0; i < setMaxBoxes; i++)
    boxes[i] = new PokemonStorageBox;

  load(saveFile, boxesOffset, skipInd);
}

PokemonStorageSet::~PokemonStorageSet()
{
  for(var8 i = 0; i < setMaxBoxes; i++)
    delete boxes[i];
}

void PokemonStorageSet::load(SaveFile* saveFile, var16 boxesOffset, svar8 skipInd)
{
  reset();

  if(saveFile == nullptr)
    return;

  for (var8 i = 0; i < setMaxBoxes; i++) {

    if(skipInd >= 0 && i == skipInd)
      continue;

    boxes[i]->load(
          saveFile,
          (i * 0x462) + boxesOffset);
  }
}

void PokemonStorageSet::save(SaveFile* saveFile, var16 boxesOffset, svar8 skipInd)
{
  for (var8 i = 0; i < setMaxBoxes; i++) {

    if(skipInd >= 0 && i == skipInd)
      continue;

    boxes[i]->save(
          saveFile,
          (i * 0x462) + boxesOffset);
  }
}

void PokemonStorageSet::reset()
{
  for(var8 i = 0; i < setMaxBoxes; i++)
    boxes[i]->reset();
}

void PokemonStorageSet::randomize(PlayerBasics* basics)
{
  reset();

  for(var8 i = 0; i < setMaxBoxes; i++)
    boxes[i]->randomize(basics);
}

void PokemonStorageSet::loadSpecific(SaveFile* saveFile, var16 offset, var8 toBox)
{
  boxes[toBox]->load(
        saveFile,
        offset);
}

void PokemonStorageSet::saveSpecific(SaveFile* saveFile, var16 offset, var8 fromBox)
{
  boxes[fromBox]->save(
        saveFile,
        offset);
}

int PokemonStorageSet::boxCount()
{
  return setMaxBoxes;
}

PokemonStorageBox* PokemonStorageSet::boxAt(int ind)
{
  return boxes[ind];
}

void PokemonStorageSet::boxSwap(int from, int to)
{
  auto eFrom = boxes[from];
  auto eTo = boxes[to];

  boxes[from] = eTo;
  boxes[to] = eFrom;

  boxesChanged();
}
