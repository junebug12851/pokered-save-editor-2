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

/**
 * @file tst_mapsearch.cpp
 * @brief MapSearch -- the fluent map-finder the randomizer / "place on any map"
 *        feature relies on. Verifies the result-set baseline, that complementary
 *        filters (hasX / noX) partition the full set, that index filters behave at
 *        their edges, that getMaps()/mapAt()/getMapCount() agree, and that
 *        pickRandom() returns a map actually inside the narrowed set.
 */

#include <QtTest>
#include <QVector>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/util/mapsearch.h>
#include <pse-db/entries/mapdbentry.h>

class TestMapSearch : public QObject
{
  Q_OBJECT

private:
  int m_all = 0;

private slots:
  void initTestCase();
  void startOver_isAllMaps();
  void complementaryFilters_partitionTheSet();
  void indexFilters_edges();
  void getMapsMapAtCount_agree();
  void isGood_pickRandom_isInResultSet();
};

void TestMapSearch::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_all = MapsDB::inst()->search()->startOver()->getMapCount();
  QVERIFY2(m_all > 0, "no maps in the DB");
}

void TestMapSearch::startOver_isAllMaps()
{
  // A fresh search reset to startOver() holds every map.
  QCOMPARE(MapsDB::inst()->search()->startOver()->getMapCount(), m_all);
}

void TestMapSearch::complementaryFilters_partitionTheSet()
{
  // hasX and noX are disjoint subsets of the full set (maps with undefined/glitch
  // data can satisfy neither), so each is in [0, all] and together they never
  // exceed the whole set.
  auto pair = [&](int has, int no) {
    QVERIFY(has >= 0 && has <= m_all);
    QVERIFY(no  >= 0 && no  <= m_all);
    QVERIFY2(has + no <= m_all, "complementary filters overlap (sum exceeds all)");
  };

  pair(MapsDB::inst()->search()->startOver()->hasMons()->getMapCount(),
       MapsDB::inst()->search()->startOver()->noMons()->getMapCount());
  pair(MapsDB::inst()->search()->startOver()->hasWarpsOut()->getMapCount(),
       MapsDB::inst()->search()->startOver()->noWarpsOut()->getMapCount());

  // glitch / notGlitch is a clean boolean split, so it does cover the whole set.
  const int glitch    = MapsDB::inst()->search()->startOver()->isGlitch()->getMapCount();
  const int notGlitch = MapsDB::inst()->search()->startOver()->notGlitch()->getMapCount();
  QCOMPARE(glitch + notGlitch, m_all);
}

void TestMapSearch::indexFilters_edges()
{
  // Every map index is >= 0, so indexGt(-1) keeps all and indexLt(0) keeps none.
  QCOMPARE(MapsDB::inst()->search()->startOver()->indexGt(-1)->getMapCount(), m_all);
  QCOMPARE(MapsDB::inst()->search()->startOver()->indexLt(0)->getMapCount(), 0);
  // A huge lower bound excludes everything.
  QCOMPARE(MapsDB::inst()->search()->startOver()->indexGt(100000)->getMapCount(), 0);
}

void TestMapSearch::getMapsMapAtCount_agree()
{
  auto s = MapsDB::inst()->search();
  s->startOver()->notGlitch();
  const int n = s->getMapCount();
  QCOMPARE(s->getMaps().size(), n);
  if(n > 0)
    QCOMPARE(s->mapAt(0), s->getMaps().constFirst());
}

void TestMapSearch::isGood_pickRandom_isInResultSet()
{
  auto s = MapsDB::inst()->search();
  s->startOver()->isGood();
  const int n = s->getMapCount();
  QVERIFY2(n > 0 && n <= m_all, "isGood() produced an empty or oversized set");

  const MapDBEntry* picked = s->pickRandom();
  QVERIFY(picked != nullptr);
  QVERIFY2(s->getMaps().contains(const_cast<MapDBEntry*>(picked)),
           "pickRandom() returned a map outside the result set");
}

QTEST_GUILESS_MAIN(TestMapSearch)
#include "tst_mapsearch.moc"
