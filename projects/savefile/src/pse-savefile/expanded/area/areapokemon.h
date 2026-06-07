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
class SaveFileIterator;
class MapDBEntry;

constexpr var8 wildMonsCount = 10; ///< Wild-encounter slots per list (grass or water).

/**
 * @brief One wild-encounter slot: a species index and a level.
 *
 * The atomic entry of a grass/water encounter table. Comparable (operator</>) so
 * encounter lists can be ordered. Loaded/saved through an iterator since it's part
 * of a packed list.
 *
 * @see AreaPokemon (the encounter tables).
 */
class SAVEFILE_AUTOPORT AreaPokemonWild : public QObject {

  Q_OBJECT

  Q_PROPERTY(int index MEMBER index NOTIFY indexChanged) ///< Species index.
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged) ///< Encounter level.

public:
  AreaPokemonWild(int index = 0, int level = 0); ///< From an explicit index + level.
  AreaPokemonWild(bool random);                  ///< Blank or random.

  void load(SaveFileIterator* it); ///< Read one entry at the cursor.
  void save(SaveFileIterator* it); ///< Write one entry at the cursor.

  bool operator<(const AreaPokemonWild& a); ///< Order by encounter value.
  bool operator>(const AreaPokemonWild& a); ///< Order by encounter value.

signals:
  void indexChanged();
  void levelChanged();

public slots:
  /// Generates a random Pokemon from any dex entry and level.
  void randomize();
  void reset();                    ///< Blank this slot.
  void load(int index, int level); ///< Set from an explicit index + level.

public:
  // Pokemon index number and level
  int index; ///< Species index (backs property).
  int level; ///< Encounter level (backs property).
};

/**
 * Rate is how likely to encounter Pokemon
 * higher number = higher chance
 * A rate of 0 means no wild pokemon on map
 *
 * The Pokemon list is in order from most common to most rare
 * Pokemon 0: 19.9% chance
 * Pokemon 1: 19.9% chance
 * Pokemon 2: 15.2% chance
 * Pokemon 3: 9.8% chance
 * Pokemon 4: 9.8% chance
 * Pokemon 5: 9.8% chance
 * Pokemon 6: 5.1% chance
 * Pokemon 7: 5.1% chance
 * Pokemon 8: 4.3% chance
 * Pokemon 9: 1.2% chance
 *
 * @brief The map's wild-encounter tables: a grass list and a water list.
 *
 * Each list is exactly @ref wildMonsCount slots (most-common to most-rare by the
 * fixed odds above), with a @ref grassRate / @ref waterRate gating whether wild
 * mons appear at all (0 = none). Standard expanded-node convention (see
 * SaveFileExpanded); setTo() pulls a chosen map's encounter data.
 *
 * @see AreaPokemonWild (a slot), Area, MapDBEntry.
 */
class SAVEFILE_AUTOPORT AreaPokemon : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int grassRate MEMBER grassRate NOTIFY grassRateChanged)             ///< Grass encounter rate (0 = none).
  Q_PROPERTY(int waterRate MEMBER waterRate NOTIFY waterRateChanged)             ///< Water encounter rate (0 = none).
  Q_PROPERTY(bool pauseMons3Steps MEMBER pauseMons3Steps NOTIFY pauseMons3StepsChanged) ///< Suppress encounters for 3 steps.

  // C++ Arrays can't be Q_PROPERTY and don't need a signal because they are
  // pre-created with only their contents changing and have no properties of
  // their own

public:
  AreaPokemon(SaveFile* saveFile = nullptr);
  virtual ~AreaPokemon();

  void load(SaveFile* saveFile = nullptr); ///< Expand both encounter tables from the save.
  void save(SaveFile* saveFile);           ///< Flatten both encounter tables to the save.

  Q_INVOKABLE int grassMonsCount();              ///< Grass-slot count (wildMonsCount).
  Q_INVOKABLE AreaPokemonWild* grassMonsAt(int ind); ///< Grass slot @p ind (GC-protected return).
  Q_INVOKABLE void grassMonsSwap(int from, int to);  ///< Reorder grass slots.

  Q_INVOKABLE int waterMonsCount();              ///< Water-slot count (wildMonsCount).
  Q_INVOKABLE AreaPokemonWild* waterMonsAt(int ind); ///< Water slot @p ind (GC-protected return).
  Q_INVOKABLE void waterMonsSwap(int from, int to);  ///< Reorder water slots.

signals:
  void grassRateChanged();
  void waterRateChanged();
  void pauseMons3StepsChanged();
  void grassMonsChanged();
  void waterMonsChanged();

public slots:
  void reset();              ///< Blank both encounter tables.
  void randomize();          ///< Randomize the encounter tables.
  void setTo(MapDBEntry* map); ///< Set encounters from @p map.

public:
  // There are exactly 10 wild Pokemon in areas that have wild Pokemon
  // Create 10 entries each, no more or less
  int grassRate;                          ///< @see grassRate property.
  AreaPokemonWild* grassMons[wildMonsCount]; ///< The 10 grass-encounter slots.

  int waterRate;                          ///< @see waterRate property.
  AreaPokemonWild* waterMons[wildMonsCount]; ///< The 10 water-encounter slots.

  bool pauseMons3Steps;                   ///< @see pauseMons3Steps property.
};
