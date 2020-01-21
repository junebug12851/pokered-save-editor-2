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

#include <QObject>
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

class PokemonMove : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 moveID_ MEMBER moveID NOTIFY moveIDChanged)
  Q_PROPERTY(var8 pp_ MEMBER pp NOTIFY ppChanged)
  Q_PROPERTY(var8 ppUp_ MEMBER ppUp NOTIFY ppUpChanged)

public:
  PokemonMove(var8 move = 0, var8 pp = 0, var8 ppUp = 0);

  Q_INVOKABLE MoveDBEntry* toMove();
  Q_INVOKABLE bool isMaxPP();
  Q_INVOKABLE var8 getMaxPP();
  Q_INVOKABLE bool isMaxPpUps();

signals:
  void moveIDChanged();
  void ppChanged();
  void ppUpChanged();

public slots:
  void randomize();
  void maxPpUp();
  void restorePP();

public:
  var8 moveID;
  var8 pp;
  var8 ppUp;
};

class PokemonBox : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 species_ MEMBER species NOTIFY speciesChanged)
  Q_PROPERTY(var16 hp_ MEMBER hp NOTIFY hpChanged)
  Q_PROPERTY(var8 level_ MEMBER level NOTIFY levelChanged)
  Q_PROPERTY(var8 status_ MEMBER status NOTIFY statusChanged)
  Q_PROPERTY(var8 type1_ MEMBER type1 NOTIFY type1Changed)
  Q_PROPERTY(var8 type2_ MEMBER type2 NOTIFY type2Changed)
  Q_PROPERTY(var8 catchRate_ MEMBER catchRate NOTIFY catchRateChanged)
  Q_PROPERTY(var16 otID_ MEMBER otID NOTIFY otIDChanged)
  Q_PROPERTY(var32 exp_ MEMBER exp NOTIFY expChanged)
  Q_PROPERTY(var16 hpExp_ MEMBER hpExp NOTIFY hpExpChanged)
  Q_PROPERTY(var16 atkExp_ MEMBER atkExp NOTIFY atkExpChanged)
  Q_PROPERTY(var16 defExp_ MEMBER defExp NOTIFY defExpChanged)
  Q_PROPERTY(var16 spdExp_ MEMBER spdExp NOTIFY spdExpChanged)
  Q_PROPERTY(var16 spExp_ MEMBER spExp NOTIFY spExpChanged)
  Q_PROPERTY(QString otName_ MEMBER otName NOTIFY otNameChanged)
  Q_PROPERTY(QString nickname_ MEMBER nickname NOTIFY nicknameChanged)
  Q_PROPERTY(QVector<PokemonMove*> moves_ MEMBER moves NOTIFY movesChanged)
  Q_PROPERTY(bool type2Explicit_ MEMBER type2Explicit NOTIFY type2ExplicitChanged)

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
  Q_INVOKABLE static PokemonBox* newPokemon(PokemonRandom list = PokemonRandom::Random_Starters, PlayerBasics* basics = nullptr);
  Q_INVOKABLE static PokemonBox* newPokemon(PokemonDBEntry* pkmnData, PlayerBasics* basics = nullptr);

  // Is this a valid Pokemon? (Is it even in the Pokedex?)
  Q_INVOKABLE PokemonDBEntry* isValid();

  Q_INVOKABLE var32 levelToExp(svar8 level = -1);
  Q_INVOKABLE var32 expLevelRangeStart();
  Q_INVOKABLE var32 expLevelRangeEnd();
  Q_INVOKABLE float expLevelRangePercent();

  Q_INVOKABLE var8 hpDV(); // Get HP DV
  Q_INVOKABLE var16 hpStat(); // Get HP Stat
  Q_INVOKABLE var16 nonHpStat(PokemonStats stat); // Get Non-HP Stat

  // Performs Pokecenter Heal
  Q_INVOKABLE bool isAfflicted();
  Q_INVOKABLE bool isHealed();
  Q_INVOKABLE bool isMaxHp();
  Q_INVOKABLE bool hasNickname();
  Q_INVOKABLE bool hasTradeStatus(PlayerBasics* basics = nullptr);

  Q_INVOKABLE bool hasEvolution();
  Q_INVOKABLE bool hasDeEvolution();
  Q_INVOKABLE bool isMaxLevel();
  Q_INVOKABLE bool isMaxPP();
  Q_INVOKABLE bool isMaxPpUps();
  Q_INVOKABLE bool isMaxEVs();
  Q_INVOKABLE bool isMinEvs();
  Q_INVOKABLE bool isMaxDVs();
  Q_INVOKABLE bool isPokemonReset();

  Q_INVOKABLE virtual void copyFrom(PokemonBox* pkmn);
  Q_INVOKABLE PokemonDBEntry* toData();

  Q_INVOKABLE var16 atkStat();
  Q_INVOKABLE var16 defStat();
  Q_INVOKABLE var16 spdStat();
  Q_INVOKABLE var16 spStat();

signals:
  void speciesChanged();
  void hpChanged();
  void levelChanged();
  void statusChanged();
  void type1Changed();
  void type2Changed();
  void catchRateChanged();
  void otIDChanged();
  void expChanged();
  void hpExpChanged();
  void atkExpChanged();
  void defExpChanged();
  void spdExpChanged();
  void spExpChanged();
  void dvChanged();
  void otNameChanged();
  void nicknameChanged();
  void movesChanged();
  void type2ExplicitChanged();

public slots:
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
  void resetExp();

  // Re-calculate stats and resetting them to updated values
  // HP and Exp are optional because their values will be lost if updated
  // Type needs to be updated in certain cases but not always
  virtual void update(bool resetHp = false,
              bool resetExp = false,
              bool resetType = false,
              bool resetCatchRate = false);

  void heal();

  // Remove or Randomize nickname/ OT Data
  // Removing requires saveFile
  void changeName(bool removeNickname = false);
  void changeOtData(bool removeOtData = false, PlayerBasics* basics = nullptr);
  void changeTrade(bool removeTradeStatus = false, PlayerBasics* basics = nullptr);

  void evolve();
  void deEvolve();
  void maxLevel();
  void maxPpUps();
  void maxEVs();
  void resetEVs();
  void maxDVs();
  void reRollDVs();
  void maxOut();
  void randomizeMoves();
  void resetPokemon();

public:
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
