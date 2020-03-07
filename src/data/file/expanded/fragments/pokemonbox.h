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

// Natures were not in gen 1 but Pokemon has released a formula for determining
// gen 1 natures mainly for bank on the virtual console
struct PokemonNatures : public QObject
{
  Q_OBJECT
  Q_ENUMS(PokemonNatures_)

public:
  enum PokemonNatures_ : var8
  {
    Hardy = 0,
    Lonely,
    Brave,
    Adamant,
    Naughty,
    Bold,
    Docile,
    Relaxed,
    Impish,
    Lax,
    Timid,
    Hasty,
    Serious,
    Jolly,
    Naive,
    Modest,
    Mild,
    Quiet,
    Bashful,
    Rash,
    Calm,
    Gentle,
    Sassy,
    Careful,
    Quirky
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

  Q_PROPERTY(bool isMaxPP READ isMaxPP NOTIFY ppCapChanged)
  Q_PROPERTY(int getMaxPP READ getMaxPP NOTIFY ppCapChanged)
  Q_PROPERTY(bool isMaxPpUps READ isMaxPpUps NOTIFY ppUpChanged)

  Q_PROPERTY(bool isInvalid READ isInvalid NOTIFY moveIDChanged)

public:
  PokemonMove(var8 move = 0, var8 pp = 0, var8 ppUp = 0);

  MoveDBEntry* toMove();

  bool isMaxPP();
  int getMaxPP();
  bool isMaxPpUps();
  bool isInvalid();

signals:
  void moveIDChanged();
  void ppChanged();
  void ppUpChanged();
  void ppCapChanged();
  void invalidChanged();

public slots:
  void randomize();
  void maxPpUp();
  void restorePP();
  void changeMove(int move = 0, int pp = 0, int ppUp = 0);

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

  Q_PROPERTY(bool isValidBool READ isValidBool NOTIFY speciesChanged)
  Q_PROPERTY(unsigned int expLevelRangeStart READ expLevelRangeStart NOTIFY expRangeChanged)
  Q_PROPERTY(unsigned int expLevelRangeEnd READ expLevelRangeEnd NOTIFY expRangeChanged)
  Q_PROPERTY(float expLevelRangePercent READ expLevelRangePercent NOTIFY expRangeChanged)

  Q_PROPERTY(int hpDV READ hpDV NOTIFY dvChanged)
  Q_PROPERTY(int hpStat READ hpStat NOTIFY statChanged)

  Q_PROPERTY(bool isAfflicted READ isAfflicted NOTIFY statusChanged)
  Q_PROPERTY(bool isHealed READ isHealed NOTIFY healedChanged)
  Q_PROPERTY(bool isMaxHp READ isMaxHp NOTIFY statChanged)
  Q_PROPERTY(bool hasNickname READ hasNickname NOTIFY hasNicknameChanged)

  Q_PROPERTY(bool hasEvolution READ hasEvolution NOTIFY speciesChanged)
  Q_PROPERTY(bool hasDeEvolution READ hasDeEvolution NOTIFY speciesChanged)
  Q_PROPERTY(bool isMaxLevel READ isMaxLevel NOTIFY levelChanged)
  Q_PROPERTY(bool isMaxPP READ isMaxPP NOTIFY movesChanged)
  Q_PROPERTY(bool isMaxPpUps READ isMaxPpUps NOTIFY movesChanged)
  Q_PROPERTY(bool isMaxEVs READ isMaxEVs NOTIFY evChanged)
  Q_PROPERTY(bool isMinEvs READ isMinEvs NOTIFY evChanged)
  Q_PROPERTY(bool isMaxDVs READ isMaxDVs NOTIFY dvChanged)
  Q_PROPERTY(bool isMinDVs READ isMinDVs NOTIFY dvChanged)
  Q_PROPERTY(bool isPokemonReset READ isPokemonReset NOTIFY pokemonResetChanged)
  Q_PROPERTY(bool isMaxedOut READ isMaxedOut NOTIFY pokemonResetChanged)
  Q_PROPERTY(bool isCorrected READ isCorrected NOTIFY pokemonResetChanged)

  Q_PROPERTY(bool isShiny READ isShiny NOTIFY dvChanged)
  Q_PROPERTY(int getNature READ getNature NOTIFY expChanged)

  Q_PROPERTY(int atkStat READ atkStat NOTIFY statChanged)
  Q_PROPERTY(int defStat READ defStat NOTIFY statChanged)
  Q_PROPERTY(int spdStat READ spdStat NOTIFY statChanged)
  Q_PROPERTY(int spStat READ spStat NOTIFY statChanged)

  Q_PROPERTY(int dexNum READ dexNum NOTIFY speciesChanged)
  Q_PROPERTY(QString speciesName READ speciesName NOTIFY speciesChanged)

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
  bool isValidBool();

  Q_INVOKABLE unsigned int levelToExp(int level = -1);
  unsigned int expLevelRangeStart();
  unsigned int expLevelRangeEnd();
  float expLevelRangePercent();

  int hpDV(); // Get HP DV
  int hpStat(); // Get HP Stat
  Q_INVOKABLE int nonHpStat(PokemonStats::PokemonStats_ stat); // Get Non-HP Stat

  // Performs Pokecenter Heal
  bool isAfflicted();
  bool isHealed();
  bool isMaxHp();
  bool hasNickname();
  Q_INVOKABLE bool hasTradeStatus(PlayerBasics* basics = nullptr);

  bool hasEvolution();
  bool hasDeEvolution();
  bool isMaxLevel();
  bool isMaxPP();
  bool isMaxPpUps();
  bool isMaxEVs();
  bool isMinEvs();
  bool isMaxDVs();
  bool isMinDVs();
  bool isPokemonReset();

  bool isMaxedOut();
  bool isCorrected();

  int dexNum();
  QString speciesName();

  // Gen 1 does not have shinies or natures
  // However Pokemon has released a formula for determining them in gen 1
  // This mainly applies to the bank for the virtual consoles

  // It's important to note that this program is not designed or intended
  // to be used to modify vc versions especially for bank. If you choose to
  // use it for that then I take no responsibility for any reprocussions
  // Any issues that may come up from using it for that I'm not going to fix
  // because that's not the purpose of this program
  bool isShiny();
  int getNature(); // Use nature enum

  virtual void copyFrom(PokemonBox* pkmn);
  PokemonDBEntry* toData();

  int atkStat();
  int defStat();
  int spdStat();
  int spStat();

  int movesCount();
  Q_INVOKABLE PokemonMove* movesAt(int ind);

  int dvCount();
  Q_INVOKABLE int dvAt(int ind);
  Q_INVOKABLE void dvSet(int ind, int val);

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

  void expRangeChanged();
  void statChanged();
  void healedChanged();
  void hasNicknameChanged();
  void evChanged();

  void pokemonResetChanged();

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
  void reRollEVs();
  void maxDVs();
  void reRollDVs();
  void resetDVs();
  void maxOut();
  void randomizeMoves();
  void resetPokemon();
  void rollShiny();
  void rollNonShiny();
  void makeShiny();
  void unmakeShiny();
  void setNature(int nature); // Use nature enum

  // It's critical that party mon are not added into box mon and vice-versa
  // This directly says if the class is actually has the extra party mon data
  // and methods or if it's a pure box mon
  // Box mon have to be in box data and party mon have to be in party data
  virtual bool isBoxMon();

  void changeMove(int ind, int moveID = 0, int pp = 0, int ppUp = 0);

  void manualSpeciesChanged();
  void manualLevelChanged();

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

  PokemonMove* moves[4];

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
