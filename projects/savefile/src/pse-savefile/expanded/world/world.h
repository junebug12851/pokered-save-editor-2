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
#include "../../savefile_autoport.h"

#include "./worldother.h"
// Fully included (not just forward-declared) so QML can traverse `world.local.*` -- the same
// de-opaque fix area.h uses for its children (see CLAUDE.md -> the "undefined chain" bug). The Map
// Storage panel binds brg.file.data.dataExpanded.world.local directly.
#include "./worldlocal.h"
// Same fix for the panel's per-map script progress (world.scripts.scriptsAt/Set), missable
// visibility bits (world.missables.missablesAt/Set) and the event flags its conflict badges
// compare against (world.events.eventsAt) -- all traversed by MapStoragePanel since 2026-07-16.
#include "./worldscripts.h"
#include "./worldmissables.h"
#include "./worldevents.h"
// ...and the hidden-pickup flags (world.hidden.hItemsAt/Set, hCoinsAt/Set), traversed by the Map
// Storage panel's "Hidden Items & Coins" section since 2026-07-17. A forward declaration is NOT
// enough here: an opaque QObject pointer makes QML read the WHOLE chain as `undefined` -- silently,
// with no warning, which is the single most expensive bug this project has had.
// @see notes/reference/qt-patterns.md
#include "./worldhidden.h"
// ...and the Phase 17 trio the Map Storage panel binds directly (2026-07-17): the town "visited"
// bits (world.towns.townsAt/Set), the in-game trade "done" bits (world.trades.tradesAt/Set), and
// the "completed" one-shots (world.completed[key], keyed Q_PROPERTY bools). Forward declarations
// left these as opaque QObject pointers, so the WHOLE chain read `undefined` in QML -- exactly the
// bug the note above describes. `worldother.h` (the fossil bytes) was already included below.
#include "./worldcompleted.h"
#include "./worldtowns.h"
#include "./worldtrades.h"

class SaveFile;

class WorldGeneral;

/**
 * @brief Global, map-independent game state -- "the state of the world".
 *
 * Where Area is about the current map, World is everything that persists across
 * maps: story @ref events, @ref completed milestones, @ref missables visibility,
 * @ref scripts, @ref towns visited, @ref trades done, @ref hidden items, plus
 * @ref general settings, @ref local map-object state, and @ref other (playtime,
 * fossils, debug). Follows the standard expanded-node convention
 * (load/save/reset/randomize -- see SaveFileExpanded).
 *
 * @note Most children are kept opaque to QML in `savefile_autoport.h` for build
 *       speed (only `other` is traversed, via the trainer-card playtime).
 * @see SaveFileExpanded, Area, and [the system map](../../../../../notes/systems/savefile.md).
 */
class SAVEFILE_AUTOPORT World : public QObject
{
  Q_OBJECT

  Q_PROPERTY(WorldCompleted* completed MEMBER completed NOTIFY completedChanged) ///< Completion milestones.
  Q_PROPERTY(WorldEvents* events MEMBER events NOTIFY eventsChanged)             ///< The 508-flag story event bitfield.
  Q_PROPERTY(WorldGeneral* general MEMBER general NOTIFY generalChanged)         ///< Options, letter delay, last maps.
  Q_PROPERTY(WorldHidden* hidden MEMBER hidden NOTIFY hiddenChanged)             ///< Hidden-item collection flags.
  Q_PROPERTY(WorldMissables* missables MEMBER missables NOTIFY missablesChanged) ///< Missable-sprite visibility flags.
  Q_PROPERTY(WorldOther* other MEMBER other NOTIFY otherChanged)                 ///< Playtime, fossils, debug (QML-traversed).
  Q_PROPERTY(WorldScripts* scripts MEMBER scripts NOTIFY scriptsChanged)         ///< Per-map script progress.
  Q_PROPERTY(WorldTowns* towns MEMBER towns NOTIFY townsChanged)                 ///< Towns visited (fly destinations).
  Q_PROPERTY(WorldTrades* trades MEMBER trades NOTIFY tradesChanged)             ///< In-game trades completed.
  Q_PROPERTY(WorldLocal* local MEMBER local NOTIFY localChanged)                 ///< Current-map sprite/object local state.

public:
  World(SaveFile* saveFile = nullptr);
  virtual ~World();

  void load(SaveFile* saveFile = nullptr); ///< Expand all world regions from the save.
  void save(SaveFile* saveFile);           ///< Flatten all world regions to the save.

signals:
  void completedChanged();
  void eventsChanged();
  void generalChanged();
  void hiddenChanged();
  void missablesChanged();
  void otherChanged();
  void scriptsChanged();
  void townsChanged();
  void tradesChanged();
  void localChanged();

public slots:
  void reset();     ///< Blank all world regions.
  void randomize(); ///< Randomize all world regions (constrained).

public:
  WorldCompleted* completed = nullptr; ///< @see completed property.
  WorldEvents* events = nullptr;       ///< @see events property.
  WorldGeneral* general = nullptr;     ///< @see general property.
  WorldHidden* hidden = nullptr;       ///< @see hidden property.
  WorldMissables* missables = nullptr; ///< @see missables property.
  WorldOther* other = nullptr;         ///< @see other property.
  WorldScripts* scripts = nullptr;     ///< @see scripts property.
  WorldTowns* towns = nullptr;         ///< @see towns property.
  WorldTrades* trades = nullptr;       ///< @see trades property.
  WorldLocal* local = nullptr;         ///< @see local property.
};
