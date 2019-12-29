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
#ifndef POKEMONNAMES_H
#define POKEMONNAMES_H

#include "../../common/types.h"

// Types in order
enum class TypesList : var8 {
  NORMAL = 0,
  FIGHTING = 1,
  FLYING = 2,
  POISON = 3,
  GROUND = 4,
  ROCK = 5,
  BUG = 7,
  GHOST = 8,
  FIRE = 20,
  WATER = 21,
  GRASS = 22,
  ELECTRIC = 23,
  PSYCHIC = 24,
  ICE = 25,
  DRAGON = 26,
};

#endif // POKEMONNAMES_H
