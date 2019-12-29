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

// Cities that can be flown to and in fly order
enum class CityList : var8 {
  PALLET_TOWN = 0,
  VIRIDIAN_CITY = 1,
  PEWTER_CITY = 2,
  CERULEAN_CITY = 3,
  VERMILLION_CITY = 4,
  LAVENDER_TOWN = 5,
  CELADON_CITY = 6,
  SAFFRON_CITY = 7,
  FUCHSIA_CITY = 8,
  CINNABAR_ISLAND = 9,
  INDIGO_PLATEAU = 10,
};

#endif // POKEMONNAMES_H
