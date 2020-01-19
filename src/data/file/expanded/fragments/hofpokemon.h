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
#ifndef HOFPOKEMON_H
#define HOFPOKEMON_H

#include <QString>
#include "../../../../common/types.h"
class SaveFile;
struct PokemonDBEntry;

class HoFPokemon
{
public:
  HoFPokemon(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0);
  virtual ~HoFPokemon();

  void load(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0);
  void save(SaveFile* saveFile, var16 recordOffset, var16 ind);
  void reset();
  void randomize();

  PokemonDBEntry* toSpecies();

  var8 species;
  var8 level;
  QString name;
};

#endif // HOFPOKEMON_H
