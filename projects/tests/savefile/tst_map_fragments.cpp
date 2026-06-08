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
 * @file tst_map_fragments.cpp
 * @brief Unit coverage for the DB-population / randomize / resolve paths of the
 *        map-edge fragments WarpData and MapConnData -- the methods the area
 *        list-ops + byte round-trip tests never reach:
 *          WarpData::load(MapDBEntryWarpOut*) and its ctor, WarpData::randomize(),
 *          WarpData::toMap(); MapConnData::loadFromData(MapDBEntryConnect*),
 *          MapConnData::toMap(), and the load(nullptr) reset path.
 *        DB sub-entries have protected ctors, so warps/connections are sourced
 *        from real, deep-linked maps in MapsDB.
 */

#include <QtTest>
#include <QHash>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrywarpout.h>
#include <pse-db/entries/mapdbentrywarpin.h>
#include <pse-db/entries/mapdbentryconnect.h>
#include <pse-savefile/expanded/fragments/warpdata.h>
#include <pse-savefile/expanded/fragments/mapconndata.h>

class TestMapFragments : public QObject
{
  Q_OBJECT

private:
  MapDBEntryWarpOut* m_warpOut = nullptr;   // a warp-out with a resolved destination
  MapDBEntryConnect* m_conn = nullptr;      // a connection with a resolved destination

private slots:
  void initTestCase();
  void warp_loadFromDbWarp_andCtor();
  void warp_randomize_landsOnAValidWarpIn();
  void conn_loadFromData_copiesAllFields();
  void conn_loadNull_resets();
};

void TestMapFragments::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);

  // Map warps/sprites/connections are NOT resolved by DB::deepLinkAll() at boot
  // (MapsDB::deepLink() is intentionally omitted there -- it's only needed by the
  // currently-disabled map randomizer). The DB-population paths under test
  // (load(warpOut) / loadFromData) read getToMap(), so resolve the links here.
  // In this Debug build QT_DEBUG guards the warp deeplink against a null toMap.
  MapsDB::inst()->deepLink();

  // First deep-linked warp-out anywhere in the map DB.
  for(auto* map : MapsDB::inst()->getStore()) {
    if(map == nullptr) continue;
    for(auto* w : map->getWarpOut()) {
      if(w != nullptr && w->getToMap() != nullptr) { m_warpOut = w; break; }
    }
    if(m_warpOut != nullptr) break;
  }
  QVERIFY2(m_warpOut != nullptr, "no deep-linked warp-out found in MapsDB");

  // First deep-linked edge connection anywhere in the map DB.
  for(auto* map : MapsDB::inst()->getStore()) {
    if(map == nullptr) continue;
    for(auto* c : map->getConnect().values()) {
      if(c != nullptr && c->getToMap() != nullptr) { m_conn = c; break; }
    }
    if(m_conn != nullptr) break;
  }
  QVERIFY2(m_conn != nullptr, "no deep-linked connection found in MapsDB");
}

// load(MapDBEntryWarpOut*) copies tile + destination from the DB warp; the
// convenience ctor routes through the same load(). toMap() then resolves the
// destination id back to its map entry.
void TestMapFragments::warp_loadFromDbWarp_andCtor()
{
  WarpData w(m_warpOut);
  QCOMPARE(w.x, m_warpOut->getX());
  QCOMPARE(w.y, m_warpOut->getY());
  QCOMPARE(w.destWarp, m_warpOut->getWarp());
  QCOMPARE(w.destMap, m_warpOut->getToMap()->getInd());

  MapDBEntry* resolved = w.toMap();
  QVERIFY(resolved != nullptr);
  QCOMPARE(resolved->getInd(), w.destMap);
}

// randomize() picks a non-outdoor map and one of its incoming warps; the result
// invariant (RNG-independent): toMap() resolves, destWarp indexes a real warp-in,
// and the warp's x/y equal that warp-in's x/y. Run a sweep to exercise the RNG.
void TestMapFragments::warp_randomize_landsOnAValidWarpIn()
{
  WarpData w(m_warpOut); // start from a known-good warp
  for(int iter = 0; iter < 25; iter++) {
    w.randomize();

    MapDBEntry* m = w.toMap();
    QVERIFY2(m != nullptr, "randomized destMap does not resolve to a map");

    auto warpIns = m->getWarpIn();
    QVERIFY2(warpIns.size() > 0, "randomized to a map with no warp-ins");
    QVERIFY(w.destWarp >= 0 && w.destWarp < warpIns.size());
    QCOMPARE(w.x, warpIns[w.destWarp]->getX());
    QCOMPARE(w.y, warpIns[w.destWarp]->getY());
  }
}

// loadFromData copies every field from a map-defined connection (its width comes
// from the resolved destination map). toMap() resolves mapPtr back to that map.
void TestMapFragments::conn_loadFromData_copiesAllFields()
{
  MapConnData c;
  c.loadFromData(m_conn);

  QCOMPARE(c.width,      m_conn->getToMap()->getWidth());
  QCOMPARE(c.mapPtr,     m_conn->getToMap()->getInd());
  QCOMPARE(c.stripSrc,   m_conn->stripLocation());
  QCOMPARE(c.stripDst,   m_conn->mapPos());
  QCOMPARE(c.stripWidth, m_conn->stripSize());
  QCOMPARE(c.yAlign,     m_conn->yAlign());
  QCOMPARE(c.xAlign,     m_conn->xAlign());
  QCOMPARE(c.viewPtr,    m_conn->window());

  MapDBEntry* resolved = c.toMap();
  QVERIFY(resolved != nullptr);
  QCOMPARE(resolved->getInd(), c.mapPtr);
}

// load() resets first then bails on a null save -> every field zeroed, no
// dereference (the graceful-degradation guard).
void TestMapFragments::conn_loadNull_resets()
{
  MapConnData c;
  c.mapPtr = 9; c.stripSrc = 9; c.stripDst = 9; c.stripWidth = 9;
  c.width = 9; c.yAlign = 9; c.xAlign = 9; c.viewPtr = 9;
  c.load(nullptr, 0);
  QCOMPARE(c.mapPtr, 0);
  QCOMPARE(c.stripSrc, 0);
  QCOMPARE(c.stripDst, 0);
  QCOMPARE(c.stripWidth, 0);
  QCOMPARE(c.width, 0);
  QCOMPARE(c.yAlign, 0);
  QCOMPARE(c.xAlign, 0);
  QCOMPARE(c.viewPtr, 0);
}

QTEST_GUILESS_MAIN(TestMapFragments)
#include "tst_map_fragments.moc"
