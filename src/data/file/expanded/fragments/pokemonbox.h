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
#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;
class SaveFileIterator;
struct PokemonEntry;
class Pokemon;
struct MoveEntry;

enum class PokemonStats : var8
{
  Attack = 0,
  Defense,
  Speed,
  Special,
  HP
};

struct PokemonMove
{
  PokemonMove(var8 move, var8 pp, var8 ppUp);
  MoveEntry* toMove();

  var8 moveID;
  var8 pp;
  var8 ppUp;
};

class PokemonBox : ExpandedInterface
{
public:
  PokemonBox(SaveFile* saveFile = nullptr,
             var16 startOffset = 0,
             var16 nicknameStartOffset = 0,
             var16 otNameStartOffset = 0,
             var8 index = 0,
             var8 recordSize = 0x21);

  virtual ~PokemonBox();

  // Creates a new Pokemon of a random starter-like species without a nickname
  // and, if a saveFile is provided, not traded. Depending on the species
  // everything else is filled out accordingly such as the chosen species type,
  // catch rate, and initial moves. It's level will be level 5
  // The random species chosen is a base evolution species that's not legendary
  // and feels "startery"
  static PokemonBox* newPokemon(SaveFile* saveFile = nullptr);

  SaveFileIterator* load(SaveFile* saveFile = nullptr,
            var16 startOffset = 0,
            var16 nicknameStartOffset = 0,
            var16 otNameStartOffset = 0,
            var8 index = 0,

            // Unless overridden, the record size for box data is 0x21
            var8 recordSize = 0x21);

  SaveFileIterator* save(SaveFile* saveFile = nullptr,
            var16 startOffset = 0,
            svar32 speciesStartOffset = 0, // -1 if doesn't exist
            var16 nicknameStartOffset = 0,
            var16 otNameStartOffset = 0,
            var8 index = 0,

            // Unless overridden, the record size for box data is 0x21
            var8 recordSize = 0x21);

  void reset();
  void randomize();

  // Is this a valid Pokemon? (Is it even in the Pokedex?)
  PokemonEntry* isValid();
  var32 levelToExp(svar8 level = -1);
  var32 expLevelRangeStart();
  var32 expLevelRangeEnd();
  float expLevelRangePercent();
  void resetExp();
  var8 hpDV(); // Get HP DV
  var16 hpStat(); // Get HP Stat
  var16 nonHpStat(PokemonStats stat); // Get Non-HP Stat

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

  QVector<PokemonMove*>* moves = nullptr;

  // Sometimes type 2 is a duplicate of type 1 and
  // sometimes it's explicitly 0xFF, this is which one
  bool type2Explicit;

protected:
  SaveFileIterator* it = nullptr;

private:
  // To surpress warnings with using the ExpandedInterface contract
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
};

#endif // POKEMONBOX_H
