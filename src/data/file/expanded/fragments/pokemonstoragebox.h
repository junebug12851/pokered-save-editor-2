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
#ifndef POKEMONSTORAGEBOX_H
#define POKEMONSTORAGEBOX_H

#include <QVector>
#include "../../../../common/types.h"
class SaveFile;
class PokemonBox;
class PlayerBasics;

// Maximum pokemon that can fit into a box
constexpr var8 boxMaxPokemon = 20;

// Holds contents of a single Pokemon storage box
class PokemonStorageBox
{
public:
  PokemonStorageBox(SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  virtual ~PokemonStorageBox();

  void load(SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  void save(SaveFile* saveFile, var16 boxOffset = 0);
  void reset();
  void randomize(PlayerBasics* basics);

  QVector<PokemonBox*> pokemon;

private:
  void load(SaveFile *saveFile = nullptr);
  void save(SaveFile *saveFile);
  void randomize();
};

#endif // POKEMONSTORAGEBOX_H
