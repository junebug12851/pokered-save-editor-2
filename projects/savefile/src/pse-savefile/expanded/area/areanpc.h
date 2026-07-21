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
class MapDBEntry;

/**
 * @brief The current map's map-global character-state flags (v1's "NPC" page).
 *
 * Nine flags that govern how *all* the map's characters behave -- NOT per-NPC. Each is a
 * single bit (or, for the trainer pointer, a 2-byte word) inside the save's status-flag
 * bytes. They are all transient script / battle / link scratch; every one is editable, but
 * the panel that consumes them must say whether an edit survives a Continue.
 *
 * Real WRAM homes (wram = sav + 0xAD54) and per-flag persistence are **console-verified**
 * (scripts/emu/probe_npc_character_state.py); the full write-up, including the three v1
 * mislabels this rename corrects, is notes/reference/npc-character-state.md. In short, on a
 * Continue:
 *   - wStatusFlags3 (0x29D9) is zeroed whole -> initTradeCenterFacing, npcsDoNotFacePlayer
 *     are REWRITTEN (an edit is momentary);
 *   - wStatusFlags5's two control bits (disableJoypad, scriptedMovementActive) are CLEARED;
 *   - the other five (initScriptedMovement, scriptedNpcMoving, testBattle, trainerBattle,
 *     trainerHeaderPtr) KEEP the raw value written (an edit is durable).
 *
 * Byte offsets/bits are unchanged from v1 -- this is a truth-in-labelling fix, save output
 * is byte-identical. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see Area.
 */
class SAVEFILE_AUTOPORT AreaNPC : public QObject
{
  Q_OBJECT

  // Sprites (wStatusFlags3, 0x29D9) -- REWRITTEN on load (whole byte zeroed)
  Q_PROPERTY(bool initTradeCenterFacing MEMBER initTradeCenterFacing NOTIFY initTradeCenterFacingChanged)   ///< BIT_INIT_TRADE_CENTER_FACING: link Trade Center sprites have faced. Zeroed on load.
  Q_PROPERTY(bool npcsDoNotFacePlayer MEMBER npcsDoNotFacePlayer NOTIFY npcsDoNotFacePlayerChanged)         ///< BIT_NO_NPC_FACE_PLAYER: NPCs don't turn to face you when talked to. Zeroed on load.

  // Controls (wStatusFlags4 b7, wStatusFlags5 b0/b5/b7)
  Q_PROPERTY(bool initScriptedMovement MEMBER initScriptedMovement NOTIFY initScriptedMovementChanged)      ///< BIT_INIT_SCRIPTED_MOVEMENT: a scripted movement is starting. Kept on load.
  Q_PROPERTY(bool scriptedNpcMoving MEMBER scriptedNpcMoving NOTIFY scriptedNpcMovingChanged)               ///< BIT_SCRIPTED_NPC_MOVEMENT: a sprite is being walked by a script. Kept on load.
  Q_PROPERTY(bool disableJoypad MEMBER disableJoypad NOTIFY disableJoypadChanged)                           ///< BIT_DISABLE_JOYPAD: player input ignored (cutscene). Cleared on load.
  Q_PROPERTY(bool scriptedMovementActive MEMBER scriptedMovementActive NOTIFY scriptedMovementActiveChanged) ///< BIT_SCRIPTED_MOVEMENT_STATE: executing a scripted movement. Cleared on load.

  // Battle (wStatusFlags7 b0/b3, wTrainerHeaderPtr)
  Q_PROPERTY(bool testBattle MEMBER testBattle NOTIFY testBattleChanged)                                    ///< BIT_TEST_BATTLE: DEBUG test-battle flag. Kept on load.
  Q_PROPERTY(bool trainerBattle MEMBER trainerBattle NOTIFY trainerBattleChanged)                           ///< BIT_TRAINER_BATTLE: the pending battle is a trainer's. Kept on load.
  Q_PROPERTY(int trainerHeaderPtr MEMBER trainerHeaderPtr NOTIFY trainerHeaderPtrChanged)                   ///< wTrainerHeaderPtr: pointer into the map's trainer-header data. Kept on load.

public:
  AreaNPC(SaveFile* saveFile = nullptr);
  virtual ~AreaNPC();

  void load(SaveFile* saveFile = nullptr); ///< Expand these flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten these flags to the save.

signals:
  void initTradeCenterFacingChanged();
  void npcsDoNotFacePlayerChanged();
  void initScriptedMovementChanged();
  void scriptedNpcMovingChanged();
  void disableJoypadChanged();
  void scriptedMovementActiveChanged();
  void testBattleChanged();
  void trainerBattleChanged();
  void trainerHeaderPtrChanged();

public slots:
  void reset();              ///< Blank the flags.
  void randomize();          ///< Randomize the flags (no-op -- see areanpc.cpp).
  void setTo(MapDBEntry* map); ///< Set from a chosen map's defaults.

public:
  // Sprites -- wStatusFlags3 (0x29D9); both zeroed on load.
  bool initTradeCenterFacing;  ///< @see initTradeCenterFacing property.
  bool npcsDoNotFacePlayer;     ///< @see npcsDoNotFacePlayer property.

  // Controls -- wStatusFlags4 b7 + wStatusFlags5 b0/b5/b7.
  bool initScriptedMovement;    ///< @see initScriptedMovement property. Kept on load.
  bool scriptedNpcMoving;       ///< @see scriptedNpcMoving property. Kept on load.
  bool disableJoypad;           ///< @see disableJoypad property. Cleared on load.
  bool scriptedMovementActive;  ///< @see scriptedMovementActive property. Cleared on load.

  // Battle -- wStatusFlags7 b0/b3 + wTrainerHeaderPtr.
  bool testBattle;              ///< @see testBattle property. DEBUG. Kept on load.
  bool trainerBattle;           ///< @see trainerBattle property. Kept on load.
  int  trainerHeaderPtr;        ///< @see trainerHeaderPtr property. Kept on load.
};
