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
 * @file tst_mapsearch_predicates.cpp
 * @brief Exhaustive sweep of every MapSearch filter predicate (the ones
 *        tst_mapsearch's targeted tests don't reach): size/area/index filters,
 *        tileset + type + sprite-set + warp/sign/sprite/connection presence
 *        filters and their complements, the incomplete/glitch/special flags, the
 *        isCity/notCity helpers, mapAt() bounds, pickRandom() on an empty set, and
 *        qmlProtect. deepLink() is called so the tileset/sprite-set predicates
 *        exercise both their match and non-match branches.
 */

#include <QtTest>
#include <QQmlEngine>
#include <functional>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/util/mapsearch.h>
#include <pse-db/entries/mapdbentry.h>

class TestMapSearchPredicates : public QObject
{
  Q_OBJECT

  int m_all = 0;
  QString m_tilesetName;

  MapSearch* fresh() { return MapsDB::inst()->search()->startOver(); }

private slots:
  void initTestCase();
  void everyPredicate_keepsCountInBounds();
  void mapAt_outOfRange_isNull();
  void pickRandom_emptySet_isNull();
  void qmlProtect_runsWithEngine();
};

void TestMapSearchPredicates::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  MapsDB::inst()->deepLink();          // resolve toTileset / toSpriteSet links
  m_all = fresh()->getMapCount();
  QVERIFY(m_all > 0);

  // Grab a real tileset name so hasTileset/notTileset hit their keep branch.
  for(int i = 0; i < MapsDB::inst()->getStoreSize(); i++) {
    MapDBEntry* m = MapsDB::inst()->getStoreAt(i);
    if(m && m->getToTileset()) { m_tilesetName = m->getToTileset()->name; break; }
  }
  QVERIFY(!m_tilesetName.isEmpty());
}

void TestMapSearchPredicates::everyPredicate_keepsCountInBounds()
{
  // Each predicate, run once on the full set, narrows to a subset in [0, all] and
  // returns the search (the chainable contract). Calling each once exercises both
  // the remove and keep paths of its filter loop since real maps fall on both sides.
  QVector<std::function<MapSearch*()>> preds = {
    [&]{ return fresh()->notNamed("Pallet Town"); },
    [&]{ return fresh()->indexLt(50); },
    [&]{ return fresh()->indexGt(5); },
    [&]{ return fresh()->widthGt(5); },
    [&]{ return fresh()->widthLt(50); },
    [&]{ return fresh()->heightGt(5); },
    [&]{ return fresh()->heightLt(50); },
    [&]{ return fresh()->areaGt(10); },
    [&]{ return fresh()->areaLt(100000); },
    [&]{ return fresh()->hasTileset(m_tilesetName); },
    [&]{ return fresh()->notTileset(m_tilesetName); },
    [&]{ return fresh()->isType("Cave"); },
    [&]{ return fresh()->notType("Cave"); },
    [&]{ return fresh()->hasConnections(); },
    [&]{ return fresh()->noConnections(); },
    [&]{ return fresh()->hasWarpsIn(); },
    [&]{ return fresh()->noWarpsIn(); },
    [&]{ return fresh()->hasSigns(); },
    [&]{ return fresh()->noSigns(); },
    [&]{ return fresh()->hasSprites(); },
    [&]{ return fresh()->noSprites(); },
    [&]{ return fresh()->hasSpriteSet(); },
    [&]{ return fresh()->noSpriteSet(); },
    [&]{ return fresh()->hasDynamicSpriteSet(); },
    [&]{ return fresh()->noDynamicSpriteSet(); },
    [&]{ return fresh()->isIncomplete(); },
    [&]{ return fresh()->notIncomplete(); },
    [&]{ return fresh()->isSpsecial(); },
    [&]{ return fresh()->notSpecial(); },
    [&]{ return fresh()->isCity(); },
    [&]{ return fresh()->notCity(); },
  };

  for(auto& p : preds) {
    MapSearch* s = p();
    QVERIFY(s != nullptr);                       // chainable: returns the search
    const int n = s->getMapCount();
    QVERIFY2(n >= 0 && n <= m_all, "predicate produced an out-of-range result count");
  }

  // isCity / notCity partition the set by the index-11 boundary.
  QCOMPARE(fresh()->isCity()->getMapCount() + fresh()->notCity()->getMapCount(), m_all);

  // Correctness guard for the spriteSet -1-sentinel fix: hasSpriteSet (index >= 0)
  // and noSpriteSet (index < 0) are a clean boolean split of the whole set.
  QCOMPARE(fresh()->hasSpriteSet()->getMapCount() + fresh()->noSpriteSet()->getMapCount(), m_all);
  // And the dynamic variants are subsets of hasSpriteSet (no crash on -1 maps).
  QVERIFY(fresh()->hasDynamicSpriteSet()->getMapCount() <= fresh()->hasSpriteSet()->getMapCount());
}

void TestMapSearchPredicates::mapAt_outOfRange_isNull()
{
  MapSearch* s = fresh();
  QVERIFY(s->mapAt(s->getMapCount()) == nullptr);   // index == size -> null
  QVERIFY(s->mapAt(0) != nullptr);
}

void TestMapSearchPredicates::pickRandom_emptySet_isNull()
{
  // A type that matches nothing empties the set; pickRandom then returns null.
  MapSearch* s = fresh()->isType("NoSuchTilesetType");
  QCOMPARE(s->getMapCount(), 0);
  QVERIFY(s->pickRandom() == nullptr);
}

void TestMapSearchPredicates::qmlProtect_runsWithEngine()
{
  QQmlEngine engine;
  MapSearch* s = MapsDB::inst()->search().data();
  s->qmlProtect(&engine);
  QCOMPARE(QQmlEngine::objectOwnership(s), QQmlEngine::CppOwnership);
}

QTEST_GUILESS_MAIN(TestMapSearchPredicates)
#include "tst_mapsearch_predicates.moc"
