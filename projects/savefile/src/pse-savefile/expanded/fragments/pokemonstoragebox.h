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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

// PokemonBox is returned by the Q_INVOKABLE pokemonAt() below — full include so
// QML receives a real (traversable) PokemonBox, not an opaque value.
#include "./pokemonbox.h"

class SaveFile;
class PokemonBox;
class PlayerBasics;

// Maximum pokemon that can fit into a box
constexpr var8 boxMaxPokemon = 20; ///< Capacity of a single PC box.

/// Holds contents of a single Pokemon storage box.
/**
 * @brief A container of PokemonBox records -- one PC box (or, when subclassed by
 *        PlayerPokemon, the party).
 *
 * Owns a vector of @ref pokemon up to @ref maxSize and provides QML-facing
 * count/full state plus slot access, reordering, removal, insertion, and
 * box-to-box relocation. The @ref isParty flag distinguishes a party from a box,
 * which matters because party and box records are NOT interchangeable on disk
 * (PokemonBox::isBoxMon()).
 *
 * @see PokemonBox (the slot type), PlayerPokemon (the party subclass),
 *      PokemonStorageSet (the collection of all boxes).
 */
class SAVEFILE_AUTOPORT PokemonStorageBox : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int pokemonCount READ pokemonCount NOTIFY pokemonChanged) ///< How many mons are in the box.
  Q_PROPERTY(int isFull READ isFull NOTIFY pokemonChanged)             ///< Is the box at capacity?
  Q_PROPERTY(int pokemonMax READ pokemonMax CONSTANT)                  ///< Box capacity.

public:
  /// @param maxSize Capacity. @param saveFile Source save. @param boxOffset Box location in the save.
  PokemonStorageBox(int maxSize = boxMaxPokemon, SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  virtual ~PokemonStorageBox();

  virtual void load(SaveFile* saveFile = nullptr, var16 boxOffset = 0); ///< Expand the box from the save.
  virtual void save(SaveFile* saveFile, var16 boxOffset = 0);           ///< Flatten the box to the save.

  int pokemonCount(); ///< Number of mons present.
  int pokemonMax();   ///< Capacity (maxSize).
  bool isFull();      ///< At capacity.

  Q_INVOKABLE PokemonBox* pokemonAt(int ind); ///< Mon at @p ind (GC-protected return).

signals:
  // When moving an item away from the box to another box, allow the model to
  // perform cleanup actions
  void beforePokemonRelocate(PokemonBox* item); ///< Emitted before a relocate so models can clean up.

  void pokemonChanged();          ///< Box contents changed.
  void pokemonMoveChange(int from, int to); ///< A mon moved slot.
  void pokemonRemoveChange(int ind);        ///< A mon was removed.
  void pokemonInsertChange();               ///< A mon was inserted.
  void pokemonResetChange();                ///< The box was reset.

public slots:
  void reset();                          ///< Empty the box.
  virtual void randomize(PlayerBasics* basics); ///< Fill with constrained random mons.

  bool pokemonMove(int from, int to); ///< Reorder a mon within the box.
  void pokemonRemove(int ind);        ///< Remove the mon at @p ind.
  virtual void pokemonNew();          ///< Add a fresh mon.

  bool relocateAll(PokemonStorageBox* dst);              ///< Move every mon into @p dst.
  virtual bool relocateOne(PokemonStorageBox* dst, int ind); ///< Move one mon into @p dst.

public:
  bool isParty = false;          ///< True if this box is actually the party (affects record format).
  QVector<PokemonBox*> pokemon;  ///< The stored mons.
  int maxSize = 0;               ///< Capacity.
  SaveFile* file;                ///< Owning save.
};
