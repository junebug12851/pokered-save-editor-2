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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

// PlayerBasics is used as a slot parameter below; include the full type so its
// QMetaType resolves now that it is no longer Q_DECLARE_OPAQUE_POINTER'd.
#include "../player/playerbasics.h"

class SaveFile;
class PokemonStorageBox;
class PlayerBasics;

// Maximum boxes that can fit into a set
constexpr var8 setMaxBoxes = 6; ///< Boxes per storage set (a save has two sets = 12 boxes).

/// Holds contents of a single box set, basically a row or array of boxes
/// each holding Pokemon.
/**
 * @brief One bank of six PC boxes.
 *
 * The PC's storage is two of these sets (12 boxes total). The set knows how to
 * load/save all six boxes from one base address, optionally @e skipping a box --
 * because the "current box" is stored separately in the save, so when expanding a
 * set we skip the slot that the live current-box occupies. loadSpecific()/
 * saveSpecific() move an individual box to/from a specific address.
 *
 * @see PokemonStorageBox (a single box), Storage (owns the two sets + current box).
 */
class SAVEFILE_AUTOPORT PokemonStorageSet : public QObject
{
  Q_OBJECT

public:
  /// @param boxesOffset base address of the six boxes; @param skipInd box to skip (-1 = none).
  PokemonStorageSet(SaveFile* saveFile = nullptr, var16 boxesOffset = 0, svar8 skipInd = -1);
  virtual ~PokemonStorageSet();

  /// Auto load or save boxes 1-6 from a single address and skip a box if it's
  /// the current box.
  void load(SaveFile* saveFile = nullptr, var16 boxesOffset = 0, svar8 skipInd = -1);
  void save(SaveFile* saveFile, var16 boxesOffset, svar8 skipInd = -1); ///< @copydoc PokemonStorageSet::load

  /// Load a specific box at a specific address into box @p toBox, overwriting it.
  void loadSpecific(SaveFile* saveFile = nullptr, var16 offset = 0, var8 toBox = 0);
  /// Save box @p fromBox out to a specific address.
  void saveSpecific(SaveFile* saveFile = nullptr, var16 offset = 0, var8 fromBox = 0);

  PokemonStorageBox* boxAt(int ind); ///< Box at @p ind within this set.

signals:
  void boxesChanged(); ///< Any box in the set changed.

public slots:
  void reset();                          ///< Empty all six boxes.
  void randomize(PlayerBasics* basics);  ///< Fill the set with constrained random mons.

public:
  // There are never any more or less than exactly a set amount of boxes in a
  // set, there's no need for this to be a Vector
  PokemonStorageBox* boxes[setMaxBoxes]; ///< The six boxes (fixed-size; never grows/shrinks).
};
