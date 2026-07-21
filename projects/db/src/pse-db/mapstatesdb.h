/*
  * Copyright 2026 Fairy Fox
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
#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>
#include <QVariantList>
#include <QVariantMap>

#include "./db_autoport.h"

/**
 * @brief One event flag of a map-state stage's ABSOLUTE save block.
 *
 * `owned` is false for a CONTEXT flag -- one another map's story writes (Pallet's
 * post-lab stage needs Oak's Lab's `EVENT_FOLLOWED_OAK_INTO_LAB`). Per leadership
 * (2026-07-17) context flags are written naturally when a state is applied -- "the
 * game is setup to work on global variables that can be shared".
 */
struct DB_AUTOPORT MapStateEv
{
  int ind = -1;        ///< The flag's index in WorldEvents / events.json.
  QString name;        ///< Its pret name (EVENT_...).
  bool owned = true;   ///< False = a cross-map context flag.
};

/// One missable of a stage's absolute save block (the map's OWN missables, complete).
struct DB_AUTOPORT MapStateMis
{
  int ind = -1;        ///< The bit in WorldMissables / missables.json.
  QString name;        ///< Display name ("Prof. Oak").
  bool hide = false;   ///< Desired state: true = HIDDEN (the bit's own polarity).
};

/**
 * @brief One state of a map's progression blueprint.
 *
 * RESTING stages (`kind == "resting"`) are the progression a person rolls through --
 * ids "1", "2", "2a"... (letters = genuine branches). TRANSIENT states ("N.k") are
 * mid-cutscene script values: real, storable, shown (leadership: "if it's a valid
 * option it needs to be shown"), but they carry no absolute save block -- applying
 * one writes only the script byte.
 */
struct DB_AUTOPORT MapStateStage
{
  QString id;          ///< Progression id ("1", "2a", "1.2"...).
  QString kind;        ///< "resting" | "transient".
  QString name;        ///< Human name ("First ambush armed").
  QString desc;        ///< What the map looks/behaves like here.
  QString timeline;    ///< Where this sits in the run of the game.
  QString triggerText; ///< What moved the game INTO this state.
  int script = 0;      ///< The value the map's script byte holds here.
  QString scriptName;  ///< Its SCRIPT_* suffix name.
  bool derived = false;///< Auto-derived skeleton stage (not hand-curated).
  bool hasSave = false;///< Does it carry an absolute save block? (resting only)
  QVector<MapStateEv> set;      ///< Events SET in this stage (absolute).
  QVector<MapStateEv> cleared;  ///< Events CLEARED (owned universe minus set).
  QVector<MapStateMis> missables; ///< The map's own missables, complete, absolute.
  QStringList badges;  ///< Badges held in this stage ("BOULDERBADGE"...).
  QStringList notes;   ///< saveNotes -- facts bits can't carry (cross-map toggles...).
  QVariantList advance;///< Outgoing edges, pass-through for the UI.
};

/**
 * @brief One map's whole progression blueprint (a `map-states/<Map>.json` file).
 *
 * The researched default states of the map -- what the "map state" concept IS. See
 * notes/reference/map-states.md for the model and notes/plans/map-states.md for the
 * feature. Fields are plain data resolved at generation time; every ind was validated
 * against events.json / missables.json / scripts.json when the file was generated.
 */
class DB_AUTOPORT MapStateBlueprint
{
public:
  int getMapInd() const { return mapInd; }
  int getScriptSlot() const { return scriptSlot; }   ///< WorldScripts index; -1 = none.
  bool getCurated() const { return curated; }
  const QString& getMapName() const { return mapName; }
  int getEntryX() const { return entryX; }           ///< Default landing spot (first warp).
  int getEntryY() const { return entryY; }
  const QString& getStart() const { return start; }
  const QString& getEnd() const { return end; }
  const QStringList& getOrder() const { return order; } ///< Resting stages, story order.
  const QVariantMap& getBranches() const { return branches; }
  bool getMessy() const { return messy; }
  const QString& getNotes() const { return notes; }
  const QVector<MapStateStage>& getStages() const { return stages; }
  const QVariantList& getScriptValues() const { return scriptValues; }
  quint8 getBadgeUniverse() const { return badgeUniverse; } ///< Bits of badges any stage touches.

  const MapStateStage* stage(const QString& id) const; ///< Stage by id, or nullptr.

protected:
  friend class MapStatesDB;
  int mapInd = -1;
  int scriptSlot = -1;
  bool curated = false;
  QString mapName;
  int entryX = 0, entryY = 0;
  QString start, end;
  QStringList order;
  QVariantMap branches;
  bool messy = false;
  QString notes;
  QVector<MapStateStage> stages;
  QVariantList scriptValues;
  quint8 badgeUniverse = 0;
};

/**
 * @brief The map-state blueprints database -- one progression blueprint per scripted map.
 *
 * Loads the 98 `assets/data/map-states/<Map>.json` files (generated by
 * `scripts/extract_map_states.py` from pret/pokered; 34 story maps hand-curated).
 * Keyed by map index. NOT exposed to QML -- MapModel is the surface; this stays plain
 * data, which is also what keeps it out of the GC-ownership trap entirely.
 *
 * Standard DB-singleton shape (inst() + load() called from DB::load(); the
 * constructor deliberately does NOT load -- the Qt 6 static-init rule).
 *
 * @see MapStateBlueprint, notes/reference/map-states.md, notes/plans/map-states.md.
 */
class DB_AUTOPORT MapStatesDB : public QObject
{
  Q_OBJECT

public:
  static MapStatesDB* inst(); ///< The process-wide singleton.

  const QHash<int, MapStateBlueprint*>& getStore() const; ///< All blueprints, by map ind.
  int getStoreSize() const;                               ///< Blueprint count.

  /// The blueprint for map @p mapInd, or nullptr when that map has none.
  const MapStateBlueprint* at(int mapInd) const;

public slots:
  void load(); ///< Load every blueprint from the qrc (idempotent).

public:
  MapStatesDB() = default; ///< (Obtain the singleton via inst(); ctor loads nothing.)

  QHash<int, MapStateBlueprint*> store; ///< mapInd -> blueprint.
};
