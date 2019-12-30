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
#ifndef STARTER_H
#define STARTER_H

#include "../common/types.h"
#include <QString>

struct PokemonEntry;

// Something I made, I hand-selected a ton of other starter options I thought
// would be good starters. This randomly selects among them.
// 1) They must all be base evolution if there is one
// 2) They musn't be legendary
// 3) Just lots of judgement calls from there, they must feel "startery"

class StarterPokemon
{
public:
  static void load();
  static void deepLink();

  static PokemonEntry* random3Starter();
  static PokemonEntry* randomAnyStarter();

  static QVector<QString>* starters;
  static QVector<PokemonEntry*>* toPokemon;
};

#endif // STARTER_H
