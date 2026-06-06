/*
  * Copyright 2020 Twilight
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
#pragma once
#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../fragments/pokemonstoragebox.h"
#include "../../savefile_autoport.h"
// PokemonParty is returned by the Q_INVOKABLE partyAt() below — full include so
// QML receives a real (traversable) PokemonParty, not an opaque value.
#include "../fragments/pokemonparty.h"

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
  virtual v