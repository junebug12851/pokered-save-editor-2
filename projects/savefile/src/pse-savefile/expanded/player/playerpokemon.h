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

#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../fragments/pokemonstoragebox.h"
#include "../../savefile_autoport.h"

class SaveFile;
class PlayerBasics;
class PokemonParty;

constexpr var8 maxParty = 6;

class SAVEFILE_AUTOPORT PlayerPokemon : public PokemonStorageBox
{
  Q_OBJECT

public:
  PlayerPokemon(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokemon();

  virtual void load(SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  virtual void save(SaveFile* saveFile, var16 boxOffset = 0);

  Q_INVOKABLE PokemonParty* partyAt(int ind);

public slots:
  virtual void randomize(PlayerBasics* basics);
  virtual void pokemonNew();
};

#endif // PLAYERPOKEMON_H
