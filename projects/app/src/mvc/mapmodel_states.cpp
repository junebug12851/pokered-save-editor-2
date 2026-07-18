/*
  * Copyright 2026 Twilight
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

/**
 * @file mapmodel_states.cpp
 * @brief MapModel's MAP-STATES surface — the per-map progression blueprints.
 *
 * The data: MapStatesDB (98 blueprints generated from pret/pokered by
 * `scripts/extract_map_states.py`; 34 story maps hand-curated). The model: a STAGE is
 * script byte + ABSOLUTE event flags + the map's own missables + badges — see
 * notes/reference/map-states.md. Ground rules (leadership, 2026-07-17): cross-map
 * context flags are written naturally ("the game is setup to work on global variables
 * that can be shared"); map change is seamless ("as though the map has always been
 * loaded"); transients are shown ("if it's a valid option it needs to be shown").
 * Every write below is exactly the blueprint's named facts — nothing else moves.
 */

#include <QVariantList>
#include <QVariantMap>

#include <pse-db/mapsdb.h>
#include <pse-db/mapstatesdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrywarpin.h>
#include <pse-db/entries/mapdbentrywarpout.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldevents.h>
#include <pse-savefile/expanded/world/worldmissables.h>
#include <pse-savefile/expanded/world/worldscripts.h>

#include "./mapmodel.h"

namespace {

const QStringList& badgeNames()
{
  static const QStringList names = {
    QStringLiteral("BOULDERBADGE"), QStringLiteral("CASCADEBADGE"),
    QStringLiteral("THUNDERBADGE"), QStringLiteral("RAINBOWBADGE"),
    QStringLiteral("SOULBADGE"),    QStringLiteral("MARSHBADGE"),
    QStringLiteral("VOLCANOBADGE"), QStringLiteral("EARTHBADGE")};
  return names;
}

/// The stage's script byte as the live save holds it: the WorldScripts slot when the map
/// has one, else (for the current map) the live wCurMapScript byte.
int liveScriptByteFor(const MapStateBlueprint* bp, World* worldAll, AreaMap* map,
                      int curMapInd)
{
  if (bp->getScriptSlot() >= 0 && worldAll != nullptr && worldAll->scripts != nullptr)
    return worldAll->scripts->scriptsAt(bp->getScriptSlot());
  if (bp->getMapInd() == curMapInd && map != nullptr)
    return map->curMapScript;
  return 0;
}

/// Does the live save hold exactly this resting stage's save block?
bool stageMatches(const MapStateStage& st, const MapStateBlueprint* bp, World* worldAll,
                  AreaMap* map, int curMapInd)
{
  if (!st.hasSave || worldAll == nullptr || worldAll->events == nullptr
      || worldAll->missables == nullptr)
    return false;
  if (liveScriptByteFor(bp, worldAll, map, curMapInd) != st.script)
    return false;
  for (const auto& ev : st.set)
    if (ev.owned && !worldAll->events->eventsAt(ev.ind))
      return false;
  for (const auto& ev : st.cleared)
    if (ev.owned && worldAll->events->eventsAt(ev.ind))
      return false;
  for (const auto& mis : st.missables)
    if (worldAll->missables->missablesAt(mis.ind) != mis.hide)
      return false;
  return true;
}

/// Is @p id a branch CHILD, and of which fork? Returns the parent id or "".
QString branchParentOf(const QVariantMap& branches, const QString& id)
{
  for (auto it = branches.constBegin(); it != branches.constEnd(); ++it)
    for (const QVariant& c : it.value().toList())
      if (c.toString() == id)
        return it.key();
  return QString();
}

}  // namespace

bool MapModel::hasStateBlueprint(int mapIndArg) const
{
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  return MapStatesDB::inst()->at(ind) != nullptr;
}

QVariantList MapModel::stateList(int mapIndArg) const
{
  QVariantList out;
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  if (bp == nullptr)
    return out;
  const QString cur = currentStateId(ind);
  for (const auto& st : bp->getStages()) {
    QVariantMap m;
    m[QStringLiteral("id")] = st.id;
    m[QStringLiteral("kind")] = st.kind;
    m[QStringLiteral("name")] = st.name;
    m[QStringLiteral("desc")] = st.desc;
    m[QStringLiteral("timeline")] = st.timeline;
    m[QStringLiteral("trigger")] = st.triggerText;
    m[QStringLiteral("script")] = st.script;
    m[QStringLiteral("scriptName")] = st.scriptName;
    m[QStringLiteral("derived")] = st.derived;
    m[QStringLiteral("isCurrent")] = (st.id == cur);
    out.append(m);
  }
  return out;
}

QString MapModel::currentStateId(int mapIndArg) const
{
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  if (bp == nullptr || worldAll == nullptr)
    return QString();

  // Resting stages, LATEST first — where two stages could both read as a match, the
  // furthest story position wins (in practice they differ by their owned flags).
  const auto& stages = bp->getStages();
  for (int i = stages.size() - 1; i >= 0; --i)
    if (stages[i].kind == QLatin1String("resting")
        && stageMatches(stages[i], bp, worldAll, map, mapInd()))
      return stages[i].id;

  // Then the transients — the byte alone names them (they carry no save block).
  const int byte = liveScriptByteFor(bp, worldAll, map, mapInd());
  for (const auto& st : stages)
    if (st.kind == QLatin1String("transient") && st.script == byte)
      return st.id;

  return QString();  // custom — said honestly, never guessed
}

QVariantMap MapModel::stateAt(const QString& id, int mapIndArg) const
{
  QVariantMap m;
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  const auto* st = bp == nullptr ? nullptr : bp->stage(id);
  if (st == nullptr)
    return m;
  m[QStringLiteral("id")] = st->id;
  m[QStringLiteral("kind")] = st->kind;
  m[QStringLiteral("name")] = st->name;
  m[QStringLiteral("desc")] = st->desc;
  m[QStringLiteral("timeline")] = st->timeline;
  m[QStringLiteral("trigger")] = st->triggerText;
  m[QStringLiteral("script")] = st->script;
  m[QStringLiteral("scriptName")] = st->scriptName;
  m[QStringLiteral("derived")] = st->derived;
  m[QStringLiteral("notes")] = st->notes;
  m[QStringLiteral("eventsSet")] = int(st->set.size());
  m[QStringLiteral("eventsCleared")] = int(st->cleared.size());
  m[QStringLiteral("badges")] = st->badges;
  return m;
}

void MapModel::applyState(const QString& id, int mapIndArg)
{
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  const auto* st = bp == nullptr ? nullptr : bp->stage(id);
  if (st == nullptr)
    return;

  // The script byte — the map's own WorldScripts slot, AND the live working byte when
  // this is the loaded map (one meaning, two homes; reference/map-scripts-missables.md).
  if (bp->getScriptSlot() >= 0 && worldAll != nullptr && worldAll->scripts != nullptr)
    worldAll->scripts->scriptsSet(bp->getScriptSlot(), st->script);
  if (bp->getMapInd() == mapInd() && map != nullptr
      && map->curMapScript != var8(st->script)) {
    map->curMapScript = st->script;
    map->curMapScriptChanged();
  }

  // Transients carry only the byte — applying one is legal and writes nothing else.
  if (st->hasSave && worldAll != nullptr) {
    if (worldAll->events != nullptr) {
      for (const auto& ev : st->set)
        worldAll->events->eventsSet(ev.ind, true);
      for (const auto& ev : st->cleared)
        worldAll->events->eventsSet(ev.ind, false);
    }
    if (worldAll->missables != nullptr)
      for (const auto& mis : st->missables)
        worldAll->missables->missablesSet(mis.ind, mis.hide);
    if (basics != nullptr && bp->getBadgeUniverse() != 0) {
      for (int bit = 0; bit < 8; ++bit) {
        if (!(bp->getBadgeUniverse() & (1u << bit)))
          continue;  // a badge no stage of this map touches is not ours to move
        basics->badgeSet(bit, st->badges.contains(badgeNames().at(bit)));
      }
    }
  }

  emit changed();
}

bool MapModel::rollForward(int mapIndArg)
{
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  if (bp == nullptr)
    return false;
  QString cur = currentStateId(ind);
  const auto& order = bp->getOrder();
  const auto& branches = bp->getBranches();
  if (cur.isEmpty()) {
    // A custom state rolls INTO the line at its start.
    if (order.isEmpty())
      return false;
    applyState(order.first(), ind);
    return true;
  }
  // A transient "N.k" belongs to stage N's exit — roll from N.
  if (cur.contains(QLatin1Char('.')))
    cur = cur.section(QLatin1Char('.'), 0, 0);

  // A fork advances into its FIRST branch; otherwise the next id in the order that is
  // not some other fork's child (a branch is entered only from its own fork).
  QString next;
  if (branches.contains(cur)) {
    const auto kids = branches.value(cur).toList();
    if (!kids.isEmpty())
      next = kids.first().toString();
  } else {
    const int at = order.indexOf(cur);
    if (at < 0)
      return false;
    for (int j = at + 1; j < order.size(); ++j) {
      const QString parent = branchParentOf(branches, order.at(j));
      if (!parent.isEmpty() && parent != cur)
        continue;  // a sibling branch, or someone else's child — not on this path
      next = order.at(j);
      break;
    }
  }
  if (next.isEmpty())
    return false;
  applyState(next, ind);
  return true;
}

bool MapModel::rollBack(int mapIndArg)
{
  const int ind = mapIndArg < 0 ? mapInd() : mapIndArg;
  const auto* bp = MapStatesDB::inst()->at(ind);
  if (bp == nullptr)
    return false;
  QString cur = currentStateId(ind);
  if (cur.isEmpty())
    return false;  // a custom state has no "previous"; rolling forward enters the line
  if (cur.contains(QLatin1Char('.'))) {
    // Mid-cutscene: back means the stage the cutscene leaves.
    applyState(cur.section(QLatin1Char('.'), 0, 0), ind);
    return true;
  }
  const auto& order = bp->getOrder();
  const auto& branches = bp->getBranches();

  // A branch child returns to its fork.
  const QString parent = branchParentOf(branches, cur);
  if (!parent.isEmpty()) {
    applyState(parent, ind);
    return true;
  }
  const int at = order.indexOf(cur);
  if (at <= 0)
    return false;
  // Walking back over a branch group lands on its FIRST child (the default path).
  QString prev = order.at(at - 1);
  const QString prevParent = branchParentOf(branches, prev);
  if (!prevParent.isEmpty()) {
    const auto kids = branches.value(prevParent).toList();
    if (!kids.isEmpty())
      prev = kids.first().toString();
  }
  applyState(prev, ind);
  return true;
}

void MapModel::changeMapConstructed(int newMapInd)
{
  if (map == nullptr)
    return;
  auto* entry = MapsDB::inst()->getIndAt(QString::number(newMapInd));
  if (entry == nullptr || area == nullptr) {
    // A glitch/half-baked id has no ROM data to construct from — fall back to the
    // honest one-byte write, exactly as before.
    setMapInd(newMapInd);
    return;
  }

  // The landing spot: the blueprint's entry (the first warp), else the map's first
  // arrival point, else the origin.
  const auto* bp = MapStatesDB::inst()->at(newMapInd);
  int x = 0, y = 0;
  if (bp != nullptr) {
    x = bp->getEntryX();
    y = bp->getEntryY();
  } else if (!entry->getWarpIn().isEmpty()) {
    x = entry->getWarpIn().first()->getX();
    y = entry->getWarpIn().first()->getY();
  }

  // The whole Area block, rebuilt from the destination's own ROM data — header, tileset
  // + pointers, cast, warps, signs, wild tables, music, sprite set, the player on (x,y).
  area->setTo(entry, x, y);

  // The live working script byte resumes the map's OWN stored progression — the same
  // copy the console makes on entry. Seamless: as though the map had always been loaded.
  int v = 0;
  if (bp != nullptr && bp->getScriptSlot() >= 0 && worldAll != nullptr
      && worldAll->scripts != nullptr)
    v = worldAll->scripts->scriptsAt(bp->getScriptSlot());
  if (map->curMapScript != var8(v)) {
    map->curMapScript = v;
    map->curMapScriptChanged();
  }

  // `wLastMap` — what every `$FF` "back outside" door means. Outdoors: the map itself.
  // Indoors: the outdoor map whose door leads here (the first one the ROM knows of).
  int last = -1;
  if (newMapInd < 0x25) {  // FIRST_INDOOR_MAP — id $25; below it is the overworld
    last = newMapInd;
  } else {
    for (auto* m : MapsDB::inst()->getStore()) {
      if (m == nullptr || m->getInd() >= 0x25)
        continue;
      for (auto* wo : m->getWarpOut()) {
        if (wo != nullptr && wo->getToMap() == entry) {
          last = m->getInd();
          break;
        }
      }
      if (last >= 0)
        break;
    }
  }
  if (last >= 0)
    setLastMap(last);

  // Constructed = the game's own defaults for this map — there is nothing here the game
  // would "restore" differently on re-entry, so the edited-this-session warnings reset.
  castEdited = false;
  warpsWereEdited = false;
  signsWereEdited = false;
  connectionsWereEdited = false;

  emit changed();
  emit castChanged();
  emit warpsChanged();
  emit signsChanged();
}
