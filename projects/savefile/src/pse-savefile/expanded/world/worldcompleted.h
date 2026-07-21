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
#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

/**
 * @brief A handful of one-off "have you done X yet" milestone flags.
 *
 * Unlike the bulk WorldEvents bitfield, these are individually-meaningful named
 * one-shots the game tracks specially: the three fishing rods, the Lapras gift,
 * the starter, the Poke Center nurse, the Saffron guards, and the Elite Four run.
 * Grouped (in the fields) into rods / Pokemon / other. Standard expanded-node
 * convention (see SaveFileExpanded).
 *
 * All eight are durable persistent save data in `SECTION "Main Data"` -- none is
 * scratch, and every one is both written and read by the game.
 *
 * ⚠️ **Two of these bytes also host bits that ARE rewritten on load** --
 * `0x29D4` b0 (`strengthOutsideBattle`) and `0x29DA` b5/b6
 * (`battleEndedOrBlackout`/`usingLinkCable`), see AreaPlayer. The rewrites are
 * per-bit, not whole-byte, so these eight survive; that is console-verified, not
 * assumed (the `wMovementFlags` lesson, 2026-07-14).
 *
 * @see World, WorldEvents (the bulk event bitfield),
 *      notes/reference/world-completed.md (the research: who writes each, who
 *      reads it, where, and the traps).
 */
class SAVEFILE_AUTOPORT WorldCompleted : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool obtainedOldRod MEMBER obtainedOldRod NOTIFY obtainedOldRodChanged)       ///< Got the Old Rod.
  Q_PROPERTY(bool obtainedGoodRod MEMBER obtainedGoodRod NOTIFY obtainedGoodRodChanged)    ///< Got the Good Rod.
  Q_PROPERTY(bool obtainedSuperRod MEMBER obtainedSuperRod NOTIFY obtainedSuperRodChanged) ///< Got the Super Rod.
  Q_PROPERTY(bool obtainedLapras MEMBER obtainedLapras NOTIFY obtainedLaprasChanged)       ///< Received the Lapras gift.
  Q_PROPERTY(bool obtainedStarterPokemon MEMBER obtainedStarterPokemon NOTIFY obtainedStarterPokemonChanged) ///< Chose a starter.
  Q_PROPERTY(bool everHealedPokemon MEMBER everHealedPokemon NOTIFY everHealedPokemonChanged) ///< `BIT_USED_POKECENTER` -- really "ever TALKED to a nurse"; see below.
  Q_PROPERTY(bool satisfiedSaffronGuards MEMBER satisfiedSaffronGuards NOTIFY satisfiedSaffronGuardsChanged) ///< Gave any Saffron guard their drink (one bit, all four gates).
  Q_PROPERTY(bool startedElite4 MEMBER startedElite4 NOTIFY startedElite4Changed)    ///< `BIT_STARTED_ELITE_4` -- walked into Lorelei's room. NOT a victory; see below. 🔫

public:
  WorldCompleted(SaveFile* saveFile = nullptr);
  virtual ~WorldCompleted();

  void load(SaveFile* saveFile = nullptr); ///< Expand these flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten these flags to the save.

signals:
  void obtainedOldRodChanged();
  void obtainedGoodRodChanged();
  void obtainedSuperRodChanged();
  void obtainedLaprasChanged();
  void obtainedStarterPokemonChanged();
  void everHealedPokemonChanged();
  void satisfiedSaffronGuardsChanged();
  void startedElite4Changed();

public slots:
  void reset();     ///< Blank these milestones.
  void randomize(); ///< Randomize these milestones.

public:
  // Rods (wStatusFlags1 = 0x29D4).
  //
  // ⚠️ A rod flag means "the Fishing Guru has GIVEN you this rod", NOT "you own it" -- the rod
  // itself is an ordinary bag item. Toss it and the flag stays set, and the Guru will never hand
  // you another; CLEARING the flag is the repair. And the game only `set`s it on GiveItem SUCCESS,
  // so ticking one of these does not put a rod in the bag -- it denies you one, permanently.
  bool obtainedOldRod;          ///< `BIT_GOT_OLD_ROD` (b3). Vermilion Old Rod House, Guru at (2,4).
  bool obtainedGoodRod;         ///< `BIT_GOT_GOOD_ROD` (b4). Fuchsia Good Rod House, Guru at (5,3).
  bool obtainedSuperRod;        ///< `BIT_GOT_SUPER_ROD` (b5). Route 12 Super Rod House, Guru at (2,4).

  // Pokemon (wStatusFlags4 = 0x29DA).
  bool obtainedLapras;          ///< `BIT_GOT_LAPRAS` (b0). Silph Co 7F, the worker at (1,5).
  bool obtainedStarterPokemon;  ///< `BIT_GOT_STARTER` (b3). Set in Oak's Lab; ALSO read by Red's House 1F.

  /// `BIT_USED_POKECENTER` (b2) -- ⚠️ **misnamed, inherited from v1.** It is set by the shared
  /// Poke Center engine routine (`DisplayPokemonCenterDialogue_`), *unconditionally, before the
  /// Yes/No prompt* -- so declining a heal still sets it. It means **"ever talked to a nurse"**,
  /// not "ever healed". And all it does is suppress the nurse's extra "Shall we heal your
  /// POKeMON?" line for returning visitors: **it gates nothing**. Clearing it makes the next
  /// nurse greet you as a first-timer. The name is kept pending leadership's call.
  bool everHealedPokemon;

  // Other
  bool satisfiedSaffronGuards;  ///< `BIT_GAVE_SAFFRON_GUARDS_DRINK` (0x29D4 b6). ONE bit, read+set by all four gates.

  /// `BIT_STARTED_ELITE_4` (`wElite4Flags` 0x29E0 b1) -- 🔫 **a loaded gun, and it used to be
  /// called `defeatedLorelei`, which was a lie.**
  ///
  /// It is set by `LoreleiShowOrHideExitBlock` **on MAP LOAD, just for walking into Lorelei's
  /// room** -- you have not fought her. Its only reader is `IndigoPlateauLobby_Script`, which,
  /// if it is set, does `ResetEventRange INDIGO_PLATEAU_EVENTS_START, EVENT_LANCES_ROOM_LOCK_DOOR`
  /// -- i.e. **wipes the whole Elite Four run**. So setting this does not record progress; it arms
  /// an erasure that fires the next time the player enters the lobby. Any label must say so.
  ///
  /// The real "defeated Lorelei" is `EVENT_BEAT_LORELEIS_ROOM_TRAINER_0` -- an ordinary event flag,
  /// already shipped among WorldEvents' 2,560.
  ///
  /// 💀 Its neighbour `wElite4Flags` b0 IS named `BIT_UNUSED_BEAT_ELITE_4` -- Hall of Fame sets it
  /// and *nothing ever reads it* (pret's own `; unused`). Deliberately not modelled.
  bool startedElite4;
};
