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
 * @file tst_map_states.cpp
 * @brief The map-state progression blueprints: MapStatesDB + MapModel's states surface.
 *
 * Domain: notes/reference/map-states.md; plan: notes/plans/map-states.md. What this pins:
 *
 *  - the DB loads every blueprint and every reference resolves into range (a dangling ind
 *    would silently corrupt a different flag -- the fly.json lesson);
 *  - **applying a state writes ONLY the state's own kinds of bytes** -- the script block,
 *    the live step byte, the event bitfield, the missable bitfield, the badge pair -- and
 *    not one byte outside them (byte fidelity is sacred);
 *  - rolling forward/backward walks the researched progression exactly;
 *  - the seamless map change constructs the destination (header true, cast real, player on
 *    the blueprint's entry spot, the live step resuming the map's own stored progression).
 */

#include <QSet>
#include <QtTest>

#include "../helpers/savefilefixture.h"
#include <pse-db/db.h>
#include <pse-db/mapstatesdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-savefile/expanded/world/worldscripts.h>
#include <pse-savefile/expanded/world/worldevents.h>
#include <pse-savefile/expanded/world/worldmissables.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

// The save regions a state APPLY is allowed to touch -- each is a kind of fact a stage's
// absolute save block names. One byte outside these and the keystone goes red.
bool inAllowedApplyRegion(int off)
{
  if (off >= 0x289C && off <= 0x2915) return true;   // WorldScripts (the 97 state bytes)
  if (off == 0x2CE5)                  return true;   // wCurMapScript (the live step)
  if (off >= 0x29F3 && off <= 0x2B32) return true;   // wEventFlags (the 2,560 bits)
  if (off >= 0x2852 && off <= 0x2871) return true;   // wToggleableObjectFlags (missables)
  if (off == 0x2602 || off == 0x29D6) return true;   // the badge byte, and its twin
  return false;
}

} // namespace

class TestMapStates : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void db_loadsEveryBlueprint_andEveryReferenceResolves();
  void stateList_readsTheProgression();
  void bestEffort_neverCustom_andRawStepsResolve();
  void applyState_writesOnlyStateBytes();
  void roll_walksTheProgressionBothWays();
  void gymStage_movesItsBadge_andOnlyItsBadge();
  void changeMapConstructed_buildsTheDestination();

private:
  QByteArray m_orig;

  struct Rig {
    SaveFile sf;
    MapModel* map = nullptr;
  };

  Rig* makeRig()
  {
    auto* r = new Rig;
    loadInto(r->sf, m_orig);

    auto* area = r->sf.dataExpanded->area;
    r->map = new MapModel(area->map, area->player, area->tileset, area->general,
                          area->preloadedSprites, area->sprites, area->warps,
                          r->sf.dataExpanded->world->general, area->signs, area->pokemon,
                          r->sf.dataExpanded->world, area,
                          r->sf.dataExpanded->player->basics);
    return r;
  }
};

void TestMapStates::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// 98 blueprints (every real def_script_pointers file; the 18 unused-map aliases are not
/// maps), and every ind a stage names is in range for the array it indexes.
void TestMapStates::db_loadsEveryBlueprint_andEveryReferenceResolves()
{
  auto* db = MapStatesDB::inst();
  QCOMPARE(db->getStoreSize(), 98);

  int curated = 0, resting = 0;
  for (auto it = db->getStore().constBegin(); it != db->getStore().constEnd(); ++it) {
    const MapStateBlueprint* bp = it.value();
    QVERIFY(bp != nullptr);
    QVERIFY2(bp->getScriptSlot() < 97,
             qPrintable(QStringLiteral("%1 slot %2").arg(bp->getMapName())
                          .arg(bp->getScriptSlot())));
    if (bp->getCurated())
      curated++;
    QVERIFY(!bp->getOrder().isEmpty());
    for (const auto& st : bp->getStages()) {
      if (st.kind == QLatin1String("resting"))
        resting++;
      for (const auto& ev : st.set)
        QVERIFY2(ev.ind >= 0 && ev.ind < 2560,
                 qPrintable(QStringLiteral("%1: event %2").arg(bp->getMapName(), ev.name)));
      for (const auto& ev : st.cleared)
        QVERIFY(ev.ind >= 0 && ev.ind < 2560);
      for (const auto& mis : st.missables)
        QVERIFY2(mis.ind >= 0 && mis.ind < 228,
                 qPrintable(QStringLiteral("%1: missable %2").arg(bp->getMapName(), mis.name)));
    }
    // Every ordered id is a real resting stage.
    for (const auto& id : bp->getOrder()) {
      const MapStateStage* st = bp->stage(id);
      QVERIFY2(st != nullptr && st->kind == QLatin1String("resting"),
               qPrintable(QStringLiteral("%1: order id %2").arg(bp->getMapName(), id)));
    }
  }
  QCOMPARE(curated, 34);
  QVERIFY(resting >= 190);  // 196 as generated; a regenerate may shift a little, a wipe can't
}

/// The fixture is Pallet Town (curated, 3 stages). Its list reads the story in order, the
/// transients ride along flagged, and the fresh save matches stage "1".
void TestMapStates::stateList_readsTheProgression()
{
  Rig* r = makeRig();

  QVERIFY(r->map->hasStateBlueprint(-1));
  const QVariantList list = r->map->stateList(-1);
  QVERIFY(list.size() >= 3);
  QCOMPARE(list.first().toMap().value("id").toString(), QStringLiteral("1"));

  bool sawTransient = false;
  for (const QVariant& v : list)
    if (v.toMap().value("kind").toString() == QLatin1String("transient"))
      sawTransient = true;
  QVERIFY2(sawTransient, "transient cutscene values must be SHOWN (leadership, 2026-07-17)");

  // ⚠️ BaseSAV is a PLAYED save, and its Pallet Town sits genuinely BETWEEN stages 2 and 3
  // (Daisy walks, the Town Map is given — but the Poke Balls were never collected).
  // Leadership (2026-07-19) retired the "" (custom) answer: the app now does its BEST from
  // the dead-giveaway flags + the step byte, so the in-between save must resolve to one of
  // its neighbouring stages — never to nothing. Applying a stage then matches it exactly.
  const QString between = r->map->currentStateId(-1);
  QVERIFY2(between == QStringLiteral("2") || between == QStringLiteral("3"),
           qPrintable(QStringLiteral("in-between Pallet must best-match stage 2 or 3, got '%1'")
                        .arg(between)));
  r->map->applyState(QStringLiteral("1"), -1);
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("1"));

  delete r;
}

/// Leadership, 2026-07-19: NO "custom / not recognized" — every blueprint map must answer
/// with a state on a real save, and every raw step value no stage carries must surface as
/// (and resolve to) a synthesized "s<value>" entry that writes ONLY the step byte.
void TestMapStates::bestEffort_neverCustom_andRawStepsResolve()
{
  Rig* r = makeRig();
  auto* world = r->sf.dataExpanded->world;

  // 1. The never-custom pin, across ALL blueprints on the played fixture.
  const auto& store = MapStatesDB::inst()->getStore();
  for (auto it = store.constBegin(); it != store.constEnd(); ++it)
    QVERIFY2(!r->map->currentStateId(it.key()).isEmpty(),
             qPrintable(QStringLiteral("map %1 answered '' (custom) — retired 2026-07-19")
                          .arg(it.key())));

  // 2. Find a blueprint with a script value NO state carries (they exist: the engine
  //    battle steps and friends). The list must synthesize it as "s<value>".
  const MapStateBlueprint* bp = nullptr;
  int rawVal = -1;
  for (auto it = store.constBegin(); it != store.constEnd() && rawVal < 0; ++it) {
    if (it.value()->getScriptSlot() < 0)
      continue;
    QSet<int> covered;
    for (const auto& st : it.value()->getStages())
      covered.insert(st.script);
    for (const QVariant& v : it.value()->getScriptValues()) {
      const int val = v.toMap().value(QStringLiteral("value")).toInt();
      if (!covered.contains(val)) {
        bp = it.value();
        rawVal = val;
        break;
      }
    }
  }
  QVERIFY2(bp != nullptr, "no blueprint with an uncovered script value found (regen moved?)");

  const int ind = bp->getMapInd();
  const QString rawId = QStringLiteral("s%1").arg(rawVal);
  bool listed = false;
  for (const QVariant& v : r->map->stateList(ind))
    if (v.toMap().value(QStringLiteral("id")).toString() == rawId) {
      listed = true;
      QCOMPARE(v.toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("step"));
    }
  QVERIFY2(listed, qPrintable(QStringLiteral("state list of map %1 misses raw step %2")
                                .arg(ind).arg(rawId)));

  // 3. Raw-step byte vs flag evidence (leadership, 2026-07-19: "if stage 3 … flags are
  //    set … it should pick the one it thinks is it" — and Noop is a parked script, not
  //    a story position). On Pallet Town (uncovered value 6 = the Noop step):
  //    a. the PLAYED fixture carries later-stage giveaways -> the byte parked on 6 must
  //       NOT answer "s6"; the flags carry the verdict (a resting stage);
  //    b. after applying stage "1" (owned giveaways cleared), byte 6 has only itself to
  //       go on -> NOW it answers "s6".
  const auto* pallet = MapStatesDB::inst()->at(0);
  QVERIFY(pallet != nullptr && pallet->getScriptSlot() >= 0);
  world->scripts->scriptsSet(pallet->getScriptSlot(), 6);   // 6 = Pallet's Noop step
  const QString withEvidence = r->map->currentStateId(0);
  QVERIFY2(!withEvidence.startsWith(QLatin1Char('s')),
           qPrintable(QStringLiteral("flag evidence must outrank a parked byte, got '%1'")
                        .arg(withEvidence)));
  r->map->applyState(QStringLiteral("1"), 0);
  world->scripts->scriptsSet(pallet->getScriptSlot(), 6);
  QCOMPARE(r->map->currentStateId(0), QStringLiteral("s6"));
  r->map->applyState(QStringLiteral("1"), 0);   // leave Pallet clean for the next pins

  // 4. …and APPLYING a synthesized step writes ONLY step-byte territory.
  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);
  r->map->applyState(rawId, ind);
  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);
  for (int off : diffOffsets(before, after))
    QVERIFY2((off >= 0x289C && off <= 0x2915) || off == 0x2CE5,
             qPrintable(QStringLiteral("raw step apply moved byte 0x%1 outside the step "
                                       "bytes").arg(off, 4, 16, QChar('0'))));

  delete r;
}

/// THE KEYSTONE. Applying a stage moves bytes ONLY inside the state regions -- the script
/// block, the live step, the event bitfield, the missable bitfield, the badge pair.
void TestMapStates::applyState_writesOnlyStateBytes()
{
  Rig* r = makeRig();

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->map->applyState(QStringLiteral("2"), -1);   // Pallet Town: "Oak has led you to the lab"

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);
  QCOMPARE(before.size(), after.size());

  const QVector<int> moved = diffOffsets(before, after);
  QVERIFY2(!moved.isEmpty(), "stage 2 must change SOMETHING on a fresh save");
  for (int off : moved)
    QVERIFY2(inAllowedApplyRegion(off),
             qPrintable(QStringLiteral("byte 0x%1 moved -- outside every state region")
                          .arg(off, 4, 16, QChar('0'))));

  // And the save now MATCHES the stage it was set to.
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("2"));

  delete r;
}

/// Rolling forward walks 1 -> 2 -> 3 and stops; rolling back returns exactly.
void TestMapStates::roll_walksTheProgressionBothWays()
{
  Rig* r = makeRig();

  // Pin the origin: the played fixture is an in-between (best-matched, not exact); start
  // the walk from a cleanly-applied "1".
  r->map->applyState(QStringLiteral("1"), -1);
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("1"));
  QVERIFY(r->map->rollForward(-1));
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("2"));
  QVERIFY(r->map->rollForward(-1));
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("3"));
  QVERIFY(!r->map->rollForward(-1));   // the end of the line: nothing written

  QVERIFY(r->map->rollBack(-1));
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("2"));
  QVERIFY(r->map->rollBack(-1));
  QCOMPARE(r->map->currentStateId(-1), QStringLiteral("1"));
  QVERIFY(!r->map->rollBack(-1));      // the start of the line

  delete r;
}

/// A gym's victory stage awards its badge (both save bytes -- the pair the game keeps),
/// and rolling it back UN-awards it. No other map's badge bit moves.
void TestMapStates::gymStage_movesItsBadge_andOnlyItsBadge()
{
  Rig* r = makeRig();

  // Find Pewter Gym: the blueprint whose badge universe is exactly the Boulder bit.
  int gymInd = -1;
  for (auto it = MapStatesDB::inst()->getStore().constBegin();
       it != MapStatesDB::inst()->getStore().constEnd(); ++it)
    if (it.value()->getBadgeUniverse() == 0x01) {
      gymInd = it.key();
      break;
    }
  QVERIFY(gymInd >= 0);

  auto* basics = r->sf.dataExpanded->player->basics;
  QVERIFY(basics->badgeAt(0));                        // the played fixture holds them ALL
  bool others[8];
  for (int b = 0; b < 8; b++)
    others[b] = basics->badgeAt(b);

  r->map->applyState(QStringLiteral("1"), gymInd);    // roll the gym BACK
  QVERIFY2(!basics->badgeAt(0), "rolling the gym back un-awards its badge");
  for (int b = 1; b < 8; b++)
    QVERIFY2(basics->badgeAt(b) == others[b], "no other badge may move");
  QCOMPARE(r->map->currentStateId(gymInd), QStringLiteral("1"));

  r->map->applyState(QStringLiteral("2"), gymInd);    // "Boulder Badge won"
  QVERIFY(basics->badgeAt(0));
  for (int b = 1; b < 8; b++)
    QVERIFY2(basics->badgeAt(b) == others[b], "no other badge may move");
  QCOMPARE(r->map->currentStateId(gymInd), QStringLiteral("2"));

  delete r;
}

/// The seamless map change: the destination is CONSTRUCTED -- header true, the ROM's cast
/// in the sprite slots, the player on the blueprint's entry spot (the first warp), the
/// live step resuming the map's own stored progression byte. "As though the map has
/// always been loaded."
void TestMapStates::changeMapConstructed_buildsTheDestination()
{
  Rig* r = makeRig();

  const int dest = 1;  // Viridian City -- scripted, curated, plenty of cast
  auto* entry = MapsDB::inst()->getIndAt(QString::number(dest));
  QVERIFY(entry != nullptr);

  r->map->changeMapConstructed(dest);

  QCOMPARE(r->map->mapInd(), dest);
  QVERIFY2(r->map->headerMatches(), "construction must leave a coherent header");

  // The cast is the ROM's cast for the destination.
  QCOMPARE(r->sf.dataExpanded->area->sprites->spriteCount(),
           int(entry->getSprites().size()));

  // The player stands on the blueprint's entry spot.
  const auto* bp = MapStatesDB::inst()->at(dest);
  QVERIFY(bp != nullptr);
  QCOMPARE(int(r->sf.dataExpanded->area->player->xCoord), bp->getEntryX());
  QCOMPARE(int(r->sf.dataExpanded->area->player->yCoord), bp->getEntryY());

  // The live step byte resumes the map's own stored progression (fresh save: 0).
  QCOMPARE(r->map->mapScript(),
           r->sf.dataExpanded->world->scripts->scriptsAt(bp->getScriptSlot()));

  // Outdoors: `wLastMap` is the map itself -- the $FF doors mean "back to here".
  QCOMPARE(r->map->lastMap(), dest);

  delete r;
}

QTEST_MAIN(TestMapStates)
#include "tst_map_states.moc"
