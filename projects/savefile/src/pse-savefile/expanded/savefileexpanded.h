/*
  * Copyright 2019 Fairy Fox
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
#include "../savefile_autoport.h"

#include "./player/player.h"
#include "./area/area.h"
#include "./world/world.h"
#include "./storage.h"

class SaveFile;
class Player;
class Area;
class World;
class Daycare;
class HallOfFame;
class Rival;
class Storage;

/**
 * @brief Root of the editable object tree -- the friendly mirror of a raw save.
 *
 * `SaveFileExpanded` is the top of the `expanded/` tree that the QML UI binds to
 * (reached as `brg.file.data.dataExpanded`). It owns the seven top-level regions
 * of a Gen 1 save: the @ref player, current @ref area, @ref world state, the
 * @ref daycare, @ref hof (Hall of Fame), @ref rival, and PC @ref storage.
 *
 * @par The "expanded node" convention (used by every object in this tree)
 * Each node in the tree shares one shape, so once you've read one you've read
 * them all:
 * - **Q_PROPERTY** members expose child objects and editable leaf values to QML.
 * - **`load(saveFile)`** = *expand*: read this node's bytes out of the raw save
 *   into the object (recursing into children).
 * - **`save(saveFile)`** = *flatten*: write the object back into the raw save,
 *   only the strictly-necessary bytes (the byte-fidelity rule).
 * - **`reset()`** blanks the node; **`randomize()`** fills it with constrained,
 *   still-playable random data.
 * - The `...Changed()` signals exist only because Q_PROPERTY requires a NOTIFY;
 *   the tree isn't actually re-emitted on edit (see the in-code note below).
 *
 * @see SaveFile (owns this), and [the system map](../../../../notes/systems/savefile.md).
 */
class SAVEFILE_AUTOPORT SaveFileExpanded : public QObject
{
  Q_OBJECT

  Q_PROPERTY(Player* player MEMBER player NOTIFY playerChanged)   ///< The trainer: basics, items, pokedex, party.
  Q_PROPERTY(Area* area MEMBER area NOTIFY areaChanged)           ///< The current map/area state.
  Q_PROPERTY(World* world MEMBER world NOTIFY worldChanged)       ///< Global world state (events, scripts, trades, ...).
  Q_PROPERTY(Daycare* daycare MEMBER daycare NOTIFY daycareChanged) ///< The day-care Pokemon, if any.
  Q_PROPERTY(HallOfFame* hof MEMBER hof NOTIFY hofChanged)        ///< Hall of Fame records.
  Q_PROPERTY(Rival* rival MEMBER rival NOTIFY rivalChanged)       ///< Rival name/data.
  Q_PROPERTY(Storage* storage MEMBER storage NOTIFY storageChanged) ///< PC item/Pokemon storage.

public:
  SaveFileExpanded(SaveFile* saveFile = nullptr);
  virtual ~SaveFileExpanded();

  void load(SaveFile* saveFile = nullptr); ///< Expand: build the whole tree from @p saveFile's raw bytes.
  void save(SaveFile* saveFile);           ///< Flatten: write the whole tree back into @p saveFile.

signals:
  // No point in having these change signals but Q_PROPERTY requires them
  void playerChanged();
  void areaChanged();
  void worldChanged();
  void daycareChanged();
  void hofChanged();
  void rivalChanged();
  void storageChanged();

public slots:
  void reset();     ///< Blank every region (acts like a fresh save's expansion).
  void randomize(); ///< Constrained full randomization across the whole tree.

public:
  Player* player = nullptr;     ///< @see player property.
  Area* area = nullptr;         ///< @see area property.
  World* world = nullptr;       ///< @see world property.
  Daycare* daycare = nullptr;   ///< @see daycare property.
  HallOfFame* hof = nullptr;    ///< @see hof property.
  Rival* rival = nullptr;       ///< @see rival property.
  Storage* storage = nullptr;   ///< @see storage property.
};
