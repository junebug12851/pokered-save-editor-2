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
#ifndef POKEMONBOX_H
#define POKEMONBOX_H

#include <QVector>
#include <QString>
#include "../../../../common/types.h"
class SaveFile;
class SaveFileIterator;
struct PokemonDBEntry;
class PokemonDB;
struct MoveDBEntry;
class PlayerBasics;

enum class PokemonStats : var8
{
  Attack = 0,
  Defense,
  Speed,
  Special,
  HP
};

enum class PokemonRandom : var8
{
  Random_Starters3,
  Random_Starters,
  Random_Pokedex,
  Random_All
};

struct PokemonMove
{
  PokemonMove(var8 move = 0, var8 pp = 0, var8 ppUp = 0);
  MoveDBEntry* toMove();

  void randomize();

  bool isMaxPP();
  var8 getMaxPP();

  void maxPpUp();
  bool isMaxPpUps();

  void restorePP();

  var8 moveID;
  var8 pp;
  var8 ppUp;
};

class PokemonBox
{
public:
  PokemonBox(SaveFile* saveFile = nullptr,
             var16 startOffset = 0,
             var16 nicknameStartOffset = 0,
             var16 otNameStartOffset = 0,
             var8 index = 0,
             var8 recordSize = 0x21);

  virtual ~PokemonBox();

  // Creates a new Pokemon without a nickname and, if a saveFile is provided,
  // not traded. Depending on the species everything else is filled out
  // accordingly such as the chosen species type, catch rate, and initial moves.
  // It's level will be level 5. The random species chosen depends.

  // The first overloaded method allows you to get a random a species.
  // One of the 3 starters, A pokemon that feels "startery", any Pokemon from
  // the Pokedex, or any Pokemon at all including MissingNo's and glitch Pokemon

  // A "startery" Pokemon is one that's not legendary, a base evolution and
  // feels "startery"

  // The second overloaded method allows you to give a data record which will be
  // used.
  static PokemonBox* newPokemon(PokemonRandom list = PokemonRandom::Random_Starters, PlayerBasics* basics = nullptr);
  static PokemonBox* newPokemon(PokemonDBEntry* pkmnData, PlayerBasics* basics = nullptr);

  virtual SaveFileIterator* load(SaveFile* saveFile = nullptr,
            var16 startOffset = 0,
            var16 nicknameStartOffset = 0,
            var16 otNameStartOffset = 0,
            var8 index = 0,

            // Unless overridden, the record size for box data is 0x21
            var8 recordSize = 0x21);

  virtual SaveFileIterator* save(SaveFile* saveFile = nullptr,
            var16 startOffset = 0,
            svar32 speciesStartOffset = 0, // -1 if doesn't exist
            var16 nicknameStartOffset = 0,
            var16 otNameStartOffset = 0,
            var8 index = 0,

            // Unless overridden, the record size for box data is 0x21
            var8 recordSize = 0x21);

  virtual void reset();
  virtual void randomize(PlayerBasics* basics = nullptr);
  void clearMoves();

  // Is this a valid Pokemon? (Is it even in the Pokedex?)
  PokemonDBEntry* isValid();

  var32 levelToExp(svar8 level = -1);
  var32 expLevelRangeStart();
  var32 expLevelRangeEnd();
  float expLevelRangePercent();
  void resetExp();

  var8 hpDV(); // Get HP DV
  var16 hpStat(); // Get HP Stat
  var16 nonHpStat(PokemonStats stat); // Get Non-HP Stat

  // Re-calculate stats and resetting them to updated values
  // HP and Exp are optional because their values will be lost if updated
  // Type needs to be updated in certain cases but not always
  virtual void update(bool resetHp = false,
              bool resetExp = false,
              bool resetType = false,
              bool resetCatchRate = false);

  // Performs Pokecenter Heal
  bool isAfflicted();
  bool isHealed();
  bool isMaxHp();
  void heal();

  // Remove or Randomize nickname/ OT Data
  // Removing requires saveFile
  bool hasNickname();
  bool hasTradeStatus(PlayerBasics* basics = nullptr);
  void changeName(bool removeNickname = false);
  void changeOtData(bool removeOtData = false, PlayerBasics* basics = nullptr);
  void changeTrade(bool removeTradeStatus = false, PlayerBasics* basics = nullptr);

  bool hasEvolution();
  bool hasDeEvolution();
  void evolve();
  void deEvolve();

  bool isMaxLevel();
  void maxLevel();

  bool isMaxPP();

  bool isMaxPpUps();
  void maxPpUps();

  bool isMaxEVs();
  bool isMinEvs();
  void maxEVs();
  void resetEVs();

  bool isMaxDVs();
  void maxDVs();
  void reRollDVs();

  void maxOut();
  void randomizeMoves();

  bool isPokemonReset();
  void resetPokemon();

  virtual void copyFrom(PokemonBox* pkmn);

  PokemonDBEntry* toData();

  var16 atkStat();
  var16 defStat();
  var16 spdStat();
  var16 spStat();

  var8 species;
  var16 hp;
  var8 level;
  var8 status;
  var8 type1;
  var8 type2;
  var8 catchRate;
  var16 otID;
  var32 exp;
  var16 hpExp;
  var16 atkExp;
  var16 defExp;
  var16 spdExp;
  var16 spExp;
  var8 dv[4];
  QString otName;
  QString nickname;

  QVector<PokemonMove*> moves;

  // Sometimes type 2 is a duplicate of type 1 and
  // sometimes it's explicitly 0xFF, this is which one

  // Honestly this all started because I tried to load up a played through SAV
  // file from someone else which I didn't realize was tampered with. This was
  // one of the changes I made. After i realized it was tampered with I regret
  // adding in this feature because the real SAV file only saves types one way
  // never one or the other. Basically I've forgotten how the real save file
  // saves them so I leave it in.
  bool type2Explicit;
};

#endif // POKEMONBOX_H
