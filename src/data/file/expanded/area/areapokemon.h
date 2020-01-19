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
#ifndef AREAPOKEMON_H
#define AREAPOKEMON_H

#include <QVector>
#include "../../../../common/types.h"

class SaveFile;
class SaveFileIterator;

constexpr var8 wildMonsCount = 10;

struct AreaPokemonWild {
  AreaPokemonWild(var8 index = 0, var8 level = 0);
  AreaPokemonWild(bool random);

  bool operator<(const AreaPokemonWild& a);
  bool operator>(const AreaPokemonWild& a);

  // Generates a random Pokemon from any dex entry and level
  void randomize();
  void reset();
  void load(var8 index, var8 level);
  void load(SaveFileIterator* it);
  void save(SaveFileIterator* it);

  // Pokemon index number and level
  var8 index;
  var8 level;
};

/**
     * Rate is how likely to encounter Pokemon
     * higher number = higher chance
     * A rate of 0 means no wild pokemon on map
     *
     * The Pokemon list is in order from most common to most rare
     * Pokemon 0: 19.9% chance
     * Pokemon 1: 19.9% chance
     * Pokemon 2: 15.2% chance
     * Pokemon 3: 9.8% chance
     * Pokemon 4: 9.8% chance
     * Pokemon 5: 9.8% chance
     * Pokemon 6: 5.1% chance
     * Pokemon 7: 5.1% chance
     * Pokemon 8: 4.3% chance
     * Pokemon 9: 1.2% chance
     */
class AreaPokemon
{
public:
  AreaPokemon(SaveFile* saveFile = nullptr);
  virtual ~AreaPokemon();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  // There are exactly 10 wild Pokemon in areas that have wild Pokemon
  // Create 10 entries each, no more or less

  var8 grassRate;
  AreaPokemonWild* grassMons[wildMonsCount];

  var8 waterRate;
  AreaPokemonWild* waterMons[wildMonsCount];

  bool pauseMons3Steps;
};

#endif // AREAPOKEMON_H
