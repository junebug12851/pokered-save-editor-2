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

class SaveFile;
class HoFPokemon;

constexpr var8 maxPokemon = 6; ///< Mons recorded per Hall of Fame entry (a full party).

/**
 * @brief One Hall of Fame record: the team that beat the Elite Four, in order.
 *
 * Holds up to @ref maxPokemon HoFPokemon snapshots. The save keeps a rolling list
 * of these records; this is a single entry, addressed by @c ind. Provides
 * QML-callable add/remove/swap/access over its mons.
 *
 * @see HoFPokemon (a slot), HallOfFame (the list of records).
 */
class SAVEFILE_AUTOPORT HoFRecord : public QObject
{
  Q_OBJECT

public:
  /// @param ind which Hall of Fame record (index into the save's HoF list).
  HoFRecord(SaveFile* saveFile = nullptr, var8 ind = 0);
  virtual ~HoFRecord();

  void load(SaveFile* saveFile = nullptr, var8 ind = 0); ///< Expand record @p ind from the save.
  void save(SaveFile* saveFile, var8 ind);               ///< Flatten record @p ind to the save.

  Q_INVOKABLE int pokemonCount();              ///< Mons in this record.
  Q_INVOKABLE int pokemonMax();                ///< Capacity (maxPokemon).
  Q_INVOKABLE HoFPokemon* pokemonAt(int ind);  ///< Mon at @p ind (GC-protected return).
  Q_INVOKABLE void pokemonSwap(int from, int to); ///< Reorder mons.
  Q_INVOKABLE void pokemonRemove(int ind);     ///< Remove mon @p ind.
  Q_INVOKABLE void pokemonNew();               ///< Add a fresh mon.

signals:
  void pokemonChanged(); ///< The record's mon list changed.

public slots:
  void reset();     ///< Empty this record.
  void randomize(); ///< Fill with random mons.

public:
  QVector<HoFPokemon*> pokemon; ///< The recorded team.
};
