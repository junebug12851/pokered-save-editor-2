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
#ifndef PLAYERPOKEMON_H
#define PLAYERPOKEMON_H

#include <QVector>
#include "../../../../common/types.h"
class SaveFile;
class PlayerBasics;
class PokemonParty;

class PlayerPokemon
{
public:
  PlayerPokemon(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokemon();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(PlayerBasics* basics);

  QVector<PokemonParty*> party;

private:
  void randomize();
};

#endif // PLAYERPOKEMON_H
