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
#ifndef POKEMONSTORAGESET_H
#define POKEMONSTORAGESET_H

#include "../../../../common/types.h"
class SaveFile;
class PokemonStorageBox;
class PlayerBasics;

// Maximum boxes that can fit into a set
constexpr var8 setMaxBoxes = 6;

// Holds contents of a single box set, basically a row or array of boxes
// each holding Pokemon
class PokemonStorageSet
{
public:
  PokemonStorageSet(SaveFile* saveFile = nullptr, var16 boxesOffset = 0, svar8 skipInd = -1);
  virtual ~PokemonStorageSet();

  // Auto load or save boxes 1-6 from a single address and skip a box if it's
  // the current box
  void load(SaveFile* saveFile = nullptr, var16 boxesOffset = 0, svar8 skipInd = -1);
  void save(SaveFile* saveFile, var16 boxesOffset, svar8 skipInd = -1);
  void reset();
  void randomize(PlayerBasics* basics);

  // Load or save a specific box at a specific address to a box here overwriting
  // current box contents
  void loadSpecific(SaveFile* saveFile = nullptr, var16 offset = 0, var8 toBox = 0);
  void saveSpecific(SaveFile* saveFile = nullptr, var16 offset = 0, var8 fromBox = 0);

  // There are never any more or less than exactly a set amount of boxes in a
  // set, there's no need for this to be a Vector
  PokemonStorageBox* boxes[setMaxBoxes];
};

#endif // POKEMONSTORAGESET_H
