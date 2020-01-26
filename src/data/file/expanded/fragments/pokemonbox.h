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

struct PokemonStats : public QObject
{
  Q_OBJECT
  Q_ENUMS(PokemonStats_)

public:
  enum PokemonStats_ : var8
  {
    Attack = 0,
    Defense,
    Speed,
    Special,
    HP
  };
};

struct PokemonRandom : public QObject
{
  Q_OBJECT
  Q_ENUMS(PokemonRandom_)

public:
  enum PokemonRandom_ : var8
  {
    Random_Starters3,
    Random_Starters,
    Random_Pokedex,
    Random_All
  };
};

class PokemonMove : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int moveID MEMBER moveID NOTIFY moveIDChanged)
  Q_PROPERTY(int pp MEMBER pp NOTIFY ppChanged)
  Q_PROPERTY(int ppUp MEMBER ppUp NOTIFY ppUpChanged)

public:
  PokemonMove(var8 move = 0, var8 pp = 0, var8 ppUp = 0);

  MoveDBEntry* toMove();

  Q_INVOKABLE bool isMaxPP();
  Q_INVOKABLE int getMaxPP();
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
  int moveID;
  int pp;
  int ppUp;
};

constexpr var8 maxMoves = 4;
constexpr var8 maxDV = 4;

class PokemonBox : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int species MEMBER species NOTIFY speciesChanged)
  Q_PROPERTY(int hp MEMBER hp NOTIFY hpChanged)
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged)
  Q_PROPERTY(int status MEMBER status NOTIFY statusChanged)
  Q_PROPERTY(int type1 MEMBER type1 NOTIFY type1Changed)
  Q_PROPERTY(int type2 MEMBER type2 NOTIFY type2Changed)
  Q_PROPERTY(int catchRate MEMBER catchRate NOTIFY catchRateChanged)
  Q_PROPERTY(int otID MEMBER otID NOTIFY otIDChanged)
  Q_PROPERTY(unsigned int exp MEMBER exp NOTIFY expChanged)
  Q_PROPERTY(int hpExp MEMBER hpExp NOTIFY hpExpChanged)
  Q_PROPERTY(int atkExp MEMBER atkExp NOTIFY atkExpChanged)
  Q_PROPERTY(int defExp MEMBER defExp NOTIFY defExpChanged)
  Q_PROPERTY(int spdExp MEMBER spdExp NOTIFY spdExpChanged)
  Q_PROPERTY(int spExp MEMBER spExp NOTIFY spExpChanged)
  Q_PROPERTY(QString otName MEMBER otName NOTIFY otNameChanged)
  Q_PROPERTY(QString nickname MEMBER nickname NOTIFY nicknameChanged)
  Q_PROPERTY(bool type2Explicit MEMBER type2Explicit NOTIFY type2ExplicitChanged)

public:
  PokemonBox(SaveFile* saveFile = nullptr,
             var16 startOffset = 0,
             var16 nicknameStartOffset = 0,
             var16 otNameStartOffset = 0,
             var8 index = 0,
             var8 recordSize = 0x21);

  virtual ~PokemonBox();

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
  static PokemonBox* newPokemon(PokemonRandom::PokemonRandom_ list = PokemonRandom::Random_Starters, PlayerBasics* basics = nullptr);
  static PokemonBox* newPokemon(PokemonDBEntry* pkmnData, PlayerBasics* basics = nullptr);

  // Is this a valid Pokemon? (Is it even in the Pokedex?)
  PokemonDBEntry* isValid();
  Q_INVOKABLE bool isValidBool();

  Q_INVOKABLE unsigned int levelToExp(int level = -1);
  Q_INVOKABLE unsigned int expLevelRangeStart();
  Q_INVOKABLE unsigned int expLevelRangeEnd();
  Q_INVOKABLE float expLevelRangePercent();

  Q_INVOKABLE int hpDV(); // Get HP DV
  Q_INVOKABLE int hpStat(); // Get HP Stat
  Q_INVOKABLE int nonHpStat(PokemonStats::PokemonStats_ stat); // Get Non-HP Stat

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

  virtual void copyFrom(PokemonBox* pkmn);
  PokemonDBEntry* toData();

  Q_INVOKABLE int atkStat();
  Q_INVOKABLE int defStat();
  Q_INVOKABLE int spdStat();
  Q_INVOKABLE int spStat();

  Q_INVOKABLE int movesCount();
  Q_INVOKABLE int movesMax();
  Q_INVOKABLE PokemonMove* movesAt(int ind);
  Q_INVOKABLE void movesSwap(int from, int to);
  Q_INVOKABLE void movesRemove(int ind);
  Q_INVOKABLE void movesNew();

  Q_INVOKABLE int dvCount();
  Q_INVOKABLE int dvAt(int ind);
  Q_INVOKABLE void dvSet(int ind, int val);
  Q_INVOKABLE void dvSwap(int from, int to);

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
  int species;
  int hp;
  int level;
  int status;
  int type1;
  int type2;
  int catchRate;
  int otID;
  unsigned int exp;
  int hpExp;
  int atkExp;
  int defExp;
  int spdExp;
  int spExp;
  var8 dv[maxDV];
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
