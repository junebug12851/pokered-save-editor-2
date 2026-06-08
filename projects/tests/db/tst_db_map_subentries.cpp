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
 * @file tst_db_map_subentries.cpp
 * @brief Sweeps every map's child sub-entries (warps-out/in, signs, sprites, wild
 *        encounters, edge connections) and calls each one's getter API. These are
 *        the lowest-covered db files; iterating all maps guarantees we hit at least
 *        one instance of every kind. Also checks each child's getParent() links
 *        back to its owning map.
 */

#include <QtTest>
#include <QVector>
#include <QHash>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrywarpout.h>
#include <pse-db/entries/mapdbentrywarpin.h>
#include <pse-db/entries/mapdbentrysign.h>
#include <pse-db/entries/mapdbentrysprite.h>
#include <pse-db/entries/mapdbentrywildmon.h>
#include <pse-db/entries/mapdbentryconnect.h>

class TestDbMapSubentries : public QObject
{
  Q_OBJECT

  int m_warps = 0, m_warpIns = 0, m_signs = 0, m_sprites = 0, m_mons = 0, m_conns = 0;

  void sweepWildMons(const QVector<MapDBEntryWildMon*>& mons, MapDBEntry* parent)
  {
    for(MapDBEntryWildMon* w : mons) {
      QVERIFY(w != nullptr);
      (void)w->getName();
      QVERIFY(w->getLevel() >= 0);
      (void)w->getToPokemon();           // resolved species (may be null for glitch)
      QCOMPARE(w->getParent(), parent);
      m_mons++;
    }
  }

private slots:
  void initTestCase();
  void everyMap_childGettersAndParentLinks();
};

void TestDbMapSubentries::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(MapsDB::inst()->getStoreSize() > 0);
}

void TestDbMapSubentries::everyMap_childGettersAndParentLinks()
{
  MapsDB* db = MapsDB::inst();
  for(MapDBEntry* e : db->getStore()) {
    QVERIFY(e != nullptr);

    for(int i = 0; i < e->getWarpOutSize(); i++) {
      const MapDBEntryWarpOut* w = e->getWarpOutAt(i);
      QVERIFY(w != nullptr);
      (void)w->getX(); (void)w->getY(); (void)w->getWarp();
      (void)w->getMap(); (void)w->getGlitch(); (void)w->getToMap(); (void)w->getToWarp();
      QCOMPARE(w->getParent(), e);
      m_warps++;
    }

    for(int i = 0; i < e->getWarpInSize(); i++) {
      const MapDBEntryWarpIn* w = e->getWarpInAt(i);
      QVERIFY(w != nullptr);
      (void)w->getX(); (void)w->getY();
      QVERIFY(w->getToConnectingWarpsSize() >= 0);
      QCOMPARE(w->getParent(), e);
      m_warpIns++;
    }

    for(int i = 0; i < e->getSignsSize(); i++) {
      const MapDBEntrySign* s = e->getSignsAt(i);
      QVERIFY(s != nullptr);
      (void)s->getX(); (void)s->getY(); (void)s->getTextID();
      QCOMPARE(s->getParent(), e);
      m_signs++;
    }

    for(int i = 0; i < e->getSpritesSize(); i++) {
      const MapDBEntrySprite* s = e->getSpritesAt(i);
      QVERIFY(s != nullptr);
      (void)s->getSprite(); (void)s->getX(); (void)s->getY();
      (void)s->getMove(); (void)s->getText(); (void)s->getRange(); (void)s->getFace();
      m_sprites++;
    }

    sweepWildMons(e->getMonsRed(), e);
    sweepWildMons(e->getMonsBlue(), e);
    sweepWildMons(e->getMonsWater(), e);

    const QHash<int, MapDBEntryConnect*> conns = e->getConnect();
    for(auto it = conns.constBegin(); it != conns.constEnd(); ++it) {
      MapDBEntryConnect* c = it.value();
      QVERIFY(c != nullptr);
      (void)c->getDir(); (void)c->getMap(); (void)c->getStripMove();
      (void)c->getStripOffset(); (void)c->getFlag(); (void)c->getToMap(); (void)c->getFromMap();
      QCOMPARE(c->getParent(), e);
      m_conns++;
    }
  }

  // The fixture must actually contain at least one of each kind, else the sweep
  // proved nothing.
  QVERIFY2(m_warps   > 0, "no warp-outs across any map");
  QVERIFY2(m_signs   > 0, "no signs across any map");
  QVERIFY2(m_sprites > 0, "no sprites across any map");
  QVERIFY2(m_mons    > 0, "no wild encounters across any map");
  QVERIFY2(m_conns   > 0, "no connections across any map");
}

QTEST_GUILESS_MAIN(TestDbMapSubentries)
#include "tst_db_map_subentries.moc"
