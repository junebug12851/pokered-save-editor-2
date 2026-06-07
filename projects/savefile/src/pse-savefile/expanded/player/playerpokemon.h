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
#include "../fragments/pokemonparty.h"
#include "../../savefile_autoport.h"

class SaveFile;
class PlayerBasics;
class PokemonParty;

constexpr var8 maxParty = 6; ///< Maximum Pokemon in the active party.

/**
 * @brief The player's active party -- a specialized PokemonStorageBox.
 *
 * The party is stored just like a PC box but with party-only extras (live stats),
 * so it @e inherits PokemonStorageBox and overrides load()/save() to read/write
 * at the party's offset and slot count (@ref maxParty). Members are PokemonParty
 * entries (a PokemonBox plus the computed in-battle stats).
 *
 * @see PokemonStorageBox (base), PokemonParty (slot type), PlayerBasics
 *      (randomize() consults it for the starter).
 */
class SAVEFILE_AUTOPORT PlayerPokemon : public PokemonStorageBox
{
  Q_OBJECT

public:
  PlayerPokemon(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokemon();

  /// Expand the party from the save (@p boxOffset locates the party block).
  virtual void load(SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  /// Flatten the party back to the save (@p boxOffset locates the party block).
  virtual void save(SaveFile* saveFile, var16 boxOffset = 0);

  Q_INVOKABLE PokemonParty* partyAt(int ind); ///< Party member at @p ind (GC-protected return).

public slots:
  virtual void randomize(PlayerBasics* basics); ///< Randomize the party; @p basics supplies starter/OT context.
  virtual void pokemonNew();                    ///< Add/initialize a fresh party Pokemon.
};
