/*
  * Copyright 2020 Fairy Fox
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
#include "./pokemonbox.h"
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

/**
 * @brief A party Pokemon: a PokemonBox plus the five pre-generated battle stats.
 *
 * Box records don't store final stats (they're recomputed), but party records DO
 * carry them on disk, so PokemonParty extends PokemonBox with @ref maxHP /
 * @ref attack / @ref defense / @ref speed / @ref special and a larger record size
 * (0x2C vs the box 0x21). It overrides load()/save()/update()/reset()/randomize()
 * to handle those extra stat bytes, and provides conversions to/from a plain box
 * record (e.g. when moving a mon between the party and a PC box).
 *
 * @see PokemonBox (base), PlayerPokemon (the party container), PokemonStorageBox.
 */
class SAVEFILE_AUTOPORT PokemonParty : public PokemonBox
{
  Q_OBJECT

  Q_PROPERTY(int maxHP MEMBER maxHP NOTIFY maxHPChanged)       ///< Stored max HP.
  Q_PROPERTY(int attack MEMBER attack NOTIFY attackChanged)    ///< Stored Attack stat.
  Q_PROPERTY(int defense MEMBER defense NOTIFY defenseChanged) ///< Stored Defense stat.
  Q_PROPERTY(int speed MEMBER speed NOTIFY speedChanged)       ///< Stored Speed stat.
  Q_PROPERTY(int special MEMBER special NOTIFY specialChanged) ///< Stored Special stat.

public:
  PokemonParty(SaveFile* saveFile = nullptr,
               var16 offset = 0,
               var16 nicknameStartOffset = 0,
               var16 otNameStartOffset = 0,
               var8 index = 0);

  virtual ~PokemonParty();

  /// Expand a party record (record size defaults to the party's 0x2C).
  virtual SaveFileIterator* load(SaveFile* saveFile = nullptr,
                         var16 offset = 0,
                         var16 nicknameStartOffset = 0,
                         var16 otNameStartOffset = 0,
                         var8 index = 0,
                         var8 recordSize = 0x2C) override;

  /// Flatten a party record back to the save.
  virtual SaveFileIterator* save(SaveFile* saveFile,
                         var16 offset,
                         svar32 speciesStartOffset,
                         var16 nicknameStartOffset,
                         var16 otNameStartOffset,
                         var8 index,
                         var8 recordSize = 0x2C) override;

  virtual void copyFrom(PokemonBox* pkmn) override; ///< Copy values from another mon (box or party).

  static PokemonBox* convertToBox(PokemonParty* data);    ///< New box record from a party mon (drops stored stats).
  static PokemonParty* convertToParty(PokemonBox* data);  ///< New party mon from a box record (regenerates stats).
  PokemonBox* toBoxData();                                ///< This mon as a plain box record.

signals:
  void maxHPChanged();
  void attackChanged();
  void defenseChanged();
  void speedChanged();
  void specialChanged();

public slots:
  virtual void reset() override;                                ///< Blank, including stored stats.
  virtual void randomize(PlayerBasics* basics = nullptr) override; ///< Randomize, then regen stats.
  virtual void update(bool resetHp = false,
                      bool resetExp = false,
                      bool resetType = false,
                      bool resetCatchRate = false,
                      bool correctMoves = false) override;        ///< Recompute; also refreshes stored stats.
  virtual bool isBoxMon() override;                              ///< False -- this is a party mon.

  void regenStats(); ///< Recompute the five stored stats from species/level/DVs/EVs.

public:
  // Pre-generated stats when not in box
  int maxHP;   ///< @see maxHP property.
  int attack;  ///< @see attack property.
  int defense; ///< @see defense property.
  int speed;   ///< @see speed property.
  int special; ///< @see special property.
};
