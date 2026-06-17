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
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

// PlayerBasics is used as a slot/Q_INVOKABLE parameter below; include the full
// type so its QMetaType resolves now that it is no longer
// Q_DECLARE_OPAQUE_POINTER'd.
#include "../player/playerbasics.h"

class SaveFile;
class SaveFileIterator;
struct PokemonDBEntry;
class PokemonDB;
struct MoveDBEntry;
class PlayerBasics;

/**
 * @brief The five battle stats, in their save/index order, exposed to QML.
 *
 * QObject only so the `PokemonStats_` enum is QML-visible (registered creatable
 * in bootQmlLinkage). Order matches the game's stat layout.
 */
struct SAVEFILE_AUTOPORT PokemonStats : public QObject
{
  Q_OBJECT
  Q_ENUMS(PokemonStats_)

public:
  enum PokemonStats_ : var8
  {
    Attack = 0, ///< Physical attack.
    Defense,    ///< Physical defense.
    Speed,      ///< Speed.
    Special,    ///< Special (single stat in Gen 1).
    HP          ///< Hit points.
  };
};

// Natures were not in gen 1 but Pokemon has released a formula for determining
// gen 1 natures mainly for bank on the virtual console
/**
 * @brief The 25 natures, QML-visible.
 *
 * Natures were not in Gen 1; Pokemon later published a formula deriving a Gen 1
 * nature (mainly for Pokemon Bank on the Virtual Console). See PokemonBox::getNature().
 */
struct SAVEFILE_AUTOPORT PokemonNatures : public QObject
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

/**
 * @brief Scope selector for "new random Pokemon", QML-visible.
 * @see PokemonBox::newPokemon()
 */
struct SAVEFILE_AUTOPORT PokemonRandom : public QObject
{
  Q_OBJECT
  Q_ENUMS(PokemonRandom_)

public:
  enum PokemonRandom_ : var8
  {
    Random_Starters3, ///< One of the three canonical starters.
    Random_Starters,  ///< A "startery"-feeling Pokemon (non-legendary base evo).
    Random_Pokedex,   ///< Any Pokedex species.
    Random_All        ///< Any species at all, including MissingNo / glitch mons.
  };
};

class PokemonBox;

/**
 * @brief One of a Pokemon's four move slots: move id, PP, and PP-Ups.
 *
 * Carries the raw values plus a wall of computed QML properties (max-PP checks,
 * validity, duplicate detection, type). Holds a back-pointer to its owning
 * @ref parentMon so it can validate against the Pokemon's full move set.
 *
 * @see PokemonBox, MoveDBEntry
 */
class SAVEFILE_AUTOPORT PokemonMove : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int moveID MEMBER moveID NOTIFY moveIDChanged) ///< Move id (indexes the moves DB).
  Q_PROPERTY(int pp MEMBER pp NOTIFY ppChanged)             ///< Current PP.
  Q_PROPERTY(int ppUp MEMBER ppUp NOTIFY ppUpChanged)       ///< PP-Ups applied (0-3).

  Q_PROPERTY(bool isMaxPP READ isMaxPP NOTIFY ppCapChanged)       ///< Is PP at its current cap?
  Q_PROPERTY(int getMaxPP READ getMaxPP NOTIFY ppCapChanged)      ///< PP cap given PP-Ups.
  Q_PROPERTY(bool isMaxPpUps READ isMaxPpUps NOTIFY ppCapChanged) ///< Are PP-Ups maxed (3)?

  Q_PROPERTY(bool isInvalid READ isInvalid NOTIFY moveIDChanged)            ///< Is this move id invalid?
  Q_PROPERTY(bool isDuplicateMove READ isDuplicateMove NOTIFY moveIDChanged) ///< Does the mon already know this move?
  Q_PROPERTY(QString moveType READ moveType NOTIFY moveIDChanged)           ///< The move's type name.

public:
  /// @param parentMon Owning Pokemon. @param move/pp/ppUp Initial raw values.
  PokemonMove(PokemonBox* parentMon, var8 move = 0, var8 pp = 0, var8 ppUp = 0);

  MoveDBEntry* toMove(); ///< Resolve @ref moveID to its DB entry.

  bool isMaxPP();    ///< Is current PP at the cap?
  int getMaxPP();    ///< PP cap for this move given PP-Ups.
  bool isMaxPpUps(); ///< Are PP-Ups at max?
  bool isInvalid();  ///< Is the move id out of range / not a real move?

  QString moveType(); ///< The move's elemental type name.

  void onMoveIdChanged(); ///< Recompute derived state after the move id changes.

  QVector<int> allValidMoves();  ///< Every legal move id for this slot.
  QVector<int> validMovesLeft(); ///< Legal moves not already used by the mon.
  bool isDuplicateMove();        ///< Is this move a duplicate within the mon's set?

signals:
  void moveIDChanged();
  void ppChanged();
  void ppUpChanged();
  void ppCapChanged();
  void invalidChanged();

public slots:
  void randomize();   ///< Pick a random valid move.
  void maxPpUp();     ///< Set PP-Ups to max.
  void raisePpUp();   ///< +1 PP-Up.
  void lowerPpUp();   ///< -1 PP-Up.
  void resetPpUp();   ///< PP-Ups to 0.
  void restorePP();   ///< Refill PP to the cap.
  void changeMove(int move = 0, int pp = 0, int ppUp = 0); ///< Replace this slot's values.
  void correctMove(); ///< Clamp/repair inconsistent values.

public:
  int moveID;                  ///< Move id (backs property).
  int pp;                      ///< Current PP (backs property).
  int ppUp;                    ///< PP-Ups (backs property).
  PokemonBox* parentMon = nullptr; ///< Owning Pokemon (for cross-slot validation).
};

constexpr var8 maxMoves = 4; ///< Move slots per Pokemon.
constexpr var8 maxDV = 4;    ///< DV entries stored (Atk/Def/Spd/Spc; HP DV is derived).

/**
 * @brief A single Pokemon record -- the most property-rich object in the tree.
 *
 * Models one stored Pokemon (box format by default; PokemonParty extends it with
 * party-only stats). The stored fields are the raw save values (species, level,
 * EXP, the four EVs/"exp" stats, DVs, OT name/ID, nickname, moves). On top of
 * those sit a large set of @e computed Q_PROPERTYs the UI binds to: derived stats,
 * validity/heal/min-max checks, evolution availability, shininess and nature, dex
 * number and species name. Editing a raw field and calling update() recomputes the
 * derived values.
 *
 * Construction/serialisation take explicit byte offsets because a Pokemon's data
 * is split across regions in the save (species list, the record itself, the
 * nickname table, and the OT-name table). See load()/save().
 *
 * @see PokemonStorageBox / PokemonParty (containers), PokemonMove (move slots),
 *      PokemonDBEntry (species data), PlayerBasics (OT/trade context).
 */
class SAVEFILE_AUTOPORT PokemonBox : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int species MEMBER species NOTIFY speciesChanged)   ///< Species id (raw save value).
  Q_PROPERTY(int hp MEMBER hp NOTIFY hpChanged)                  ///< Current HP.
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged)         ///< Level.
  Q_PROPERTY(int status MEMBER status NOTIFY statusChanged)      ///< Status-condition bits.
  Q_PROPERTY(int type1 MEMBER type1 NOTIFY type1Changed)         ///< Primary type id.
  Q_PROPERTY(int type2 MEMBER type2 NOTIFY type2Changed)         ///< Secondary type id.
  Q_PROPERTY(int catchRate MEMBER catchRate NOTIFY catchRateChanged) ///< Catch-rate/held byte.
  Q_PROPERTY(int otID MEMBER otID NOTIFY otIDChanged)            ///< Original-trainer ID.
  Q_PROPERTY(unsigned int exp MEMBER exp NOTIFY expChanged)      ///< Experience points.
  Q_PROPERTY(int hpExp MEMBER hpExp NOTIFY hpExpChanged)         ///< HP stat-exp (EV equivalent).
  Q_PROPERTY(int atkExp MEMBER atkExp NOTIFY atkExpChanged)      ///< Attack stat-exp.
  Q_PROPERTY(int defExp MEMBER defExp NOTIFY defExpChanged)      ///< Defense stat-exp.
  Q_PROPERTY(int spdExp MEMBER spdExp NOTIFY spdExpChanged)      ///< Speed stat-exp.
  Q_PROPERTY(int spExp MEMBER spExp NOTIFY spExpChanged)         ///< Special stat-exp.
  Q_PROPERTY(QString otName MEMBER otName NOTIFY otNameChanged)  ///< Original-trainer name.
  Q_PROPERTY(QString nickname MEMBER nickname NOTIFY nicknameChanged) ///< Nickname (may be empty).
  Q_PROPERTY(bool type2Explicit MEMBER type2Explicit NOTIFY type2ExplicitChanged) ///< How type2 is stored (see note at the field).

  Q_PROPERTY(bool isValidBool READ isValidBool NOTIFY speciesChanged)                  ///< Is the species a real Pokedex entry?
  Q_PROPERTY(unsigned int expLevelRangeStart READ expLevelRangeStart NOTIFY expRangeChanged) ///< EXP at the current level's start.
  Q_PROPERTY(unsigned int expLevelRangeEnd READ expLevelRangeEnd NOTIFY expRangeChanged)     ///< EXP at the next level.
  Q_PROPERTY(float expLevelRangePercent READ expLevelRangePercent NOTIFY expRangeChanged)    ///< Progress through the current level (0..1).

  Q_PROPERTY(int hpDV READ hpDV NOTIFY dvChanged)     ///< Derived HP DV (from the four stored DVs).
  Q_PROPERTY(int hpStat READ hpStat NOTIFY statChanged) ///< Computed max HP.

  Q_PROPERTY(bool isAfflicted READ isAfflicted NOTIFY statusChanged) ///< Has a status condition?
  Q_PROPERTY(bool isHealed READ isHealed NOTIFY healedChanged)       ///< Fully healed (HP + status)?
  Q_PROPERTY(bool isMaxHp READ isMaxHp NOTIFY statChanged)           ///< HP at its computed max?
  Q_PROPERTY(bool hasNickname READ hasNickname NOTIFY hasNicknameChanged) ///< Has a non-default nickname?

  Q_PROPERTY(bool hasEvolution READ hasEvolution NOTIFY speciesChanged)   ///< Can evolve further?
  Q_PROPERTY(bool hasDeEvolution READ hasDeEvolution NOTIFY speciesChanged) ///< Has a prior evolution (de-evolve)?
  Q_PROPERTY(bool isMaxLevel READ isMaxLevel NOTIFY levelChanged)         ///< Level 100?
  Q_PROPERTY(bool isMaxPP READ isMaxPP NOTIFY movesChanged)              ///< All moves at max PP?
  Q_PROPERTY(bool isMaxPpUps READ isMaxPpUps NOTIFY movesChanged)        ///< All moves at max PP-Ups?
  Q_PROPERTY(bool isMaxEVs READ isMaxEVs NOTIFY evChanged)               ///< All stat-exp maxed?
  Q_PROPERTY(bool isMinEvs READ isMinEvs NOTIFY evChanged)               ///< All stat-exp zero?
  Q_PROPERTY(bool isMaxDVs READ isMaxDVs NOTIFY dvChanged)               ///< All DVs maxed?
  Q_PROPERTY(bool isMinDVs READ isMinDVs NOTIFY dvChanged)               ///< All DVs zero?
  Q_PROPERTY(bool isPokemonReset READ isPokemonReset NOTIFY pokemonResetChanged) ///< Matches a freshly-reset state?
  Q_PROPERTY(bool isMaxedOut READ isMaxedOut NOTIFY pokemonResetChanged) ///< Fully maxed (level/EV/DV/PP)?
  Q_PROPERTY(bool isCorrected READ isCorrected NOTIFY pokemonResetChanged) ///< Values are internally consistent?

  Q_PROPERTY(bool isShiny READ isShiny NOTIFY dvChanged)   ///< Shiny by the VC-era DV formula (see disclaimer below).
  Q_PROPERTY(int getNature READ getNature NOTIFY expChanged) ///< Derived nature (see PokemonNatures).

  Q_PROPERTY(int atkStat READ atkStat NOTIFY statChanged) ///< Computed Attack stat.
  Q_PROPERTY(int defStat READ defStat NOTIFY statChanged) ///< Computed Defense stat.
  Q_PROPERTY(int spdStat READ spdStat NOTIFY statChanged) ///< Computed Speed stat.
  Q_PROPERTY(int spStat READ spStat NOTIFY statChanged)   ///< Computed Special stat.

  Q_PROPERTY(int dexNum READ dexNum NOTIFY speciesChanged)            ///< Pokedex number for the species.
  Q_PROPERTY(QString speciesName READ speciesName NOTIFY speciesChanged) ///< Species display name.

public:
  /// @param startOffset record start; @param nicknameStartOffset / @param otNameStartOffset
  /// locate the split name tables; @param index slot within the box; @param recordSize
  /// per-record byte size (box default 0x21).
  PokemonBox(SaveFile* saveFile = nullptr,
             var16 startOffset = 0,
             var16 nicknameStartOffset = 0,
             var16 otNameStartOffset = 0,
             var8 index = 0,
             var8 recordSize = 0x21);

  virtual ~PokemonBox();

  /// Expand one Pokemon from the save. @return the iterator left past this record.
  virtual SaveFileIterator* load(SaveFile* saveFile = nullptr,
                                 var16 startOffset = 0,
                                 var16 nicknameStartOffset = 0,
                                 var16 otNameStartOffset = 0,
                                 var8 index = 0,

                                 // Unless overridden, the record size for box data is 0x21
                                 var8 recordSize = 0x21);

  /// Flatten one Pokemon back to the save. @return the iterator left past this record.
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
  PokemonDBEntry* isValid(); ///< The species' DB entry, or null if not a real Pokedex species.
  bool isValidBool();        ///< Convenience bool form of isValid().

  Q_INVOKABLE unsigned int levelToExp(int level = -1); ///< EXP needed for @p level (current level if -1).
  unsigned int expLevelRangeStart();   ///< EXP at the start of the current level.
  unsigned int expLevelRangeEnd();     ///< EXP at the next level.
  float expLevelRangePercent();        ///< Fractional progress through the level.

  int hpDV(); // Get HP DV
  int hpStat(); // Get HP Stat
  Q_INVOKABLE int nonHpStat(PokemonStats::PokemonStats_ stat); // Get Non-HP Stat

  // Performs Pokecenter Heal
  bool isAfflicted(); ///< Has any status condition.
  bool isHealed();    ///< Fully healed (HP + status). (heal() performs a Pokecenter heal.)
  bool isMaxHp();     ///< HP equals computed max.
  bool hasNickname(); ///< Carries a real nickname.
  Q_INVOKABLE bool hasTradeStatus(PlayerBasics* basics = nullptr); ///< Counts as traded relative to @p basics.

  bool hasEvolution();   ///< Species can evolve.
  bool hasDeEvolution(); ///< Species has a pre-evolution.
  bool isMaxLevel();     ///< Level 100.
  bool isMaxPP();        ///< All moves at max PP.
  bool isMaxPpUps();     ///< All moves at max PP-Ups.
  bool isMaxEVs();       ///< All stat-exp maxed.
  bool isMinEvs();       ///< All stat-exp zero.
  bool isMaxDVs();       ///< All DVs maxed.
  bool isMinDVs();       ///< All DVs zero.
  bool isPokemonReset(); ///< Matches the reset baseline.

  bool isMaxedOut();  ///< Level/EV/DV/PP all maxed.
  bool isCorrected(); ///< Values internally consistent (see correct* slots).

  int dexNum();         ///< Pokedex number.
  QString speciesName(); ///< Species display name.

  // Gen 1 does not have shinies or natures
  // However Pokemon has released a formula for determining them in gen 1
  // This mainly applies to the bank for the virtual consoles

  // It's important to note that this program is not designed or intended
  // to be used to modify vc versions especially for bank. If you choose to
  // use it for that then I take no responsibility for any reprocussions
  // Any issues that may come up from using it for that I'm not going to fix
  // because that's not the purpose of this program
  bool isShiny();    ///< Shiny per the VC-era DV formula (see disclaimer above).
  int getNature(); // Use nature enum

  virtual void copyFrom(PokemonBox* pkmn); ///< Deep-copy another mon's values into this one.
  PokemonDBEntry* toData();                ///< The species' DB entry for this mon.

  int atkStat(); ///< Computed Attack stat.
  int defStat(); ///< Computed Defense stat.
  int spdStat(); ///< Computed Speed stat.
  int spStat();  ///< Computed Special stat.

  int movesCount();                       ///< Number of non-empty move slots.
  int movesMax();                         ///< Move-slot capacity (maxMoves).
  Q_INVOKABLE PokemonMove* movesAt(int ind); ///< Move slot @p ind (GC-protected return).

  int dvCount();                       ///< Number of stored DVs (maxDV).
  Q_INVOKABLE int dvAt(int ind);       ///< DV value at @p ind.
  Q_INVOKABLE void dvSet(int ind, int val); ///< Set DV @p ind.

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
  virtual void reset();                              ///< Blank this Pokemon.
  virtual void randomize(PlayerBasics* basics = nullptr); ///< Randomize this Pokemon (constrained).
  void clearMoves(); ///< Empty all move slots.
  void resetExp();   ///< Reset EXP to the current level's baseline.

  // Re-calculate stats and resetting them to updated values
  // HP and Exp are optional because their values will be lost if updated
  // Type needs to be updated in certain cases but not always
  /// Recompute derived stats. Each flag opts into clobbering a value that would
  /// otherwise be preserved (HP/EXP) or only sometimes needs it (type/catch rate/moves).
  virtual void update(bool resetHp = false,
                      bool resetExp = false,
                      bool resetType = false,
                      bool resetCatchRate = false,
                      bool correctMoves = false);

  /// Reset type1/type2 to this species' DB-default type(s) (e.g. Fire/Flying for
  /// Charizard). Standalone so QML can call it cleanly -- update() with mixed bool
  /// args is unreliable from QML (the arg-passing glitch), so don't route through it.
  Q_INVOKABLE void correctTypes();

  void heal(); ///< Pokecenter heal: full HP, clear status.

  // Remove or Randomize nickname/ OT Data
  // Removing requires saveFile
  void changeName(bool removeNickname = false); ///< Randomize or (if true) remove the nickname.
  void changeOtData(bool removeOtData = false, PlayerBasics* basics = nullptr);   ///< Randomize or remove OT data.
  void changeTrade(bool removeTradeStatus = false, PlayerBasics* basics = nullptr); ///< Toggle traded status.

  void evolve();         ///< Evolve to the next species.
  void deEvolve();       ///< Revert to the prior species.
  void maxLevel();       ///< Set to level 100.
  void maxPpUps();       ///< Max every move's PP-Ups.
  void maxEVs();         ///< Max all stat-exp.
  void resetEVs();       ///< Zero all stat-exp.
  void reRollEVs();      ///< Randomize stat-exp.
  void maxDVs();         ///< Max all DVs.
  void reRollDVs();      ///< Randomize DVs.
  void resetDVs();       ///< Zero all DVs.
  void maxOut();         ///< Max level/EV/DV/PP at once.
  void randomizeMoves(); ///< Randomize the move set.
  void resetPokemon();   ///< Reset to the baseline state.
  void rollShiny();      ///< Randomize DVs until shiny.
  void rollNonShiny();   ///< Randomize DVs until not shiny.
  void makeShiny();      ///< Force DVs to a shiny combination.
  void unmakeShiny();    ///< Force DVs to a non-shiny combination.
  void setNature(int nature); // Use nature enum
  void cleanupMoves();   ///< Remove invalid/duplicate moves.
  void correctMoves();   ///< Repair move/PP inconsistencies.

  // It's critical that party mon are not added into box mon and vice-versa
  // This directly says if the class SAVEFILE_AUTOPORT is actually has the extra party mon data
  // and methods or if it's a pure box mon
  // Box mon have to be in box data and party mon have to be in party data
  virtual bool isBoxMon(); ///< True for a pure box mon; PokemonParty overrides to false.

  void changeMove(int ind, int moveID = 0, int pp = 0, int ppUp = 0); ///< Set move slot @p ind.

  /// Delete the move in slot @p ind, then compact so there is no gap in the move
  /// list (the slots after it slide up; empties stay parked at the bottom).
  Q_INVOKABLE void deleteMoveAt(int ind);

  /// Remove every move except the first one (slots 1..3 cleared). The list is
  /// compacted first, so "first" is whatever move currently leads the list.
  Q_INVOKABLE void clearMovesButFirst();

  /// Reorder the filled move slots: take the move at @p from and re-insert it
  /// before slot @p to (drop-slot convention: 0..movesCount; == movesCount
  /// appends after the last move). Only the filled prefix participates (empty
  /// slots stay parked at the bottom, per game move-list compaction). The fixed
  /// PokemonMove slot objects keep their identity -- the (id/pp/ppUp) VALUES move
  /// between them -- so QML's movesAt() pointers stay valid. Writes only the move
  /// bytes the reorder touches; nothing else in the save changes.
  Q_INVOKABLE void reorderMove(int from, int to);

  void manualSpeciesChanged(); ///< UI hook: species edited directly.
  void manualLevelChanged();   ///< UI hook: level edited directly.

public:
  int species;       ///< @see species property.
  int hp;            ///< @see hp property.
  int level;         ///< @see level property.
  int status;        ///< @see status property.
  int type1;         ///< @see type1 property.
  int type2;         ///< @see type2 property.
  int catchRate;     ///< @see catchRate property.
  int otID;          ///< @see otID property.
  unsigned int exp;  ///< @see exp property.
  int hpExp;         ///< @see hpExp property.
  int atkExp;        ///< @see atkExp property.
  int defExp;        ///< @see defExp property.
  int spdExp;        ///< @see spdExp property.
  int spExp;         ///< @see spExp property.
  var8 dv[maxDV];    ///< Stored DVs (Atk/Def/Spd/Spc); HP DV is derived.
  QString otName;    ///< @see otName property.
  QString nickname;  ///< @see nickname property.

  PokemonMove* moves[4]; ///< The four move slots.

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
