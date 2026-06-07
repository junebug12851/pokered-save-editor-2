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
#include "../savefile_autoport.h"

#include "./fragments/itemstoragebox.h"
#include "./player/playerbasics.h"
#include "./fragments/pokemonstoragebox.h"

class SaveFile;

class ItemStorageBox;
class PokemonStorageSet;
class PokemonStorageBox;
class PlayerBasics;
class PokemonBox;

// 2 Sets of 6 Pokemon Boxes
constexpr var8 maxPokemonStorageSets = 2;                       ///< PC Pokemon-box banks (2 sets).
constexpr var8 maxPokemonBoxes = maxPokemonStorageSets * 6;     ///< Total PC boxes (12).

/**
 * @brief The PC: the item storage box and all 12 Pokemon boxes.
 *
 * A top-level region of the save. Holds the PC @ref items box and two
 * PokemonStorageSets (@ref pokemon, 6 boxes each = 12). It flattens the two sets
 * into one consecutive 0..11 box space for QML (boxAt() ignores set boundaries),
 * and offers deposit/free-space helpers. @ref curBox is the active box index.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see SaveFileExpanded, PokemonStorageSet, PokemonStorageBox, ItemStorageBox.
 */
class SAVEFILE_AUTOPORT Storage : public QObject
{
  Q_OBJECT

  Q_PROPERTY(ItemStorageBox* items MEMBER items NOTIFY itemsChanged)                    ///< The PC item box.
  Q_PROPERTY(int curBox MEMBER curBox NOTIFY curBoxChanged)                             ///< Active PC box index (0..11).
  Q_PROPERTY(bool boxesFormatted MEMBER boxesFormatted NOTIFY boxesFormattedChanged)   ///< Whether the boxes are formatted.
  Q_PROPERTY(int boxCount READ boxCount CONSTANT)                                       ///< Total box count (12).

public:
  Storage(SaveFile* saveFile = nullptr);
  virtual ~Storage();

  void load(SaveFile* saveFile = nullptr); ///< Expand the PC (items + both box sets) from the save.
  void save(SaveFile* saveFile);           ///< Flatten the PC back to the save.

  // Ignores the sets and returns the box with given box number
  // Also allow swapping ignoring set boundraries (Appearing as one consecutive
  // array of boxes)
  int boxCount();                              ///< Total boxes across both sets (12).
  Q_INVOKABLE PokemonStorageBox* boxAt(int ind); ///< Box @p ind in the flattened 0..11 space (GC-protected return).

  // Returns a box pointer that has free space
  PokemonStorageBox* freeSpace();              ///< First box with room, or null if all full.

  // Deposits a pokemon into a box
  bool depositPokemon(PokemonBox* pokemon);    ///< Put @p pokemon in the first box with space. @return success.

signals:
  void itemsChanged();
  void curBoxChanged();
  void boxesFormattedChanged();
  void pokemonChanged();

public slots:
  void reset();                              ///< Blank the whole PC.
  void randomize(PlayerBasics* basics);      ///< Randomize items and Pokemon.
  void randomizePokemon(PlayerBasics* basics); ///< Randomize just the boxed Pokemon.
  void randomizeItems();                     ///< Randomize just the PC items.

public:
  ItemStorageBox* items = nullptr; ///< @see items property.
  int curBox;                      ///< @see curBox property.
  bool boxesFormatted = false;     ///< @see boxesFormatted property.

  // Because this is a C++ array, it can't be a Q_PROPERTY and since the array
  // contents never change and have no properties of themselves there's no need
  // for a signal either
  PokemonStorageSet* pokemon[maxPokemonStorageSets]; ///< The two box sets (6 boxes each).
};
