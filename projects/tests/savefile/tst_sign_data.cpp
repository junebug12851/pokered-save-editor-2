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
 * @file tst_sign_data.cpp
 * @brief Unit coverage for SignData's DB-population and randomization paths --
 *        setTo / setToAll (copy a map's signs into save-side signs), randomize /
 *        randomizeAll (shuffle sign positions), and the load(nullptr) reset path.
 *        These are the ~60% of signdata.cpp that the area-fragment list-ops test
 *        does not reach. MapDBEntrySign has a protected ctor, so the DB-defined
 *        signs are sourced from a real map in MapsDB.
 */

#include <QtTest>
#include <QVector>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrysign.h>
#include <pse-savefile/expanded/fragments/signdata.h>

class TestSignData : public QObject
{
  Q_OBJECT

private:
  // The first map carrying at least two signs -- used by every DB-backed case.
  MapDBEntry* m_map = nullptr;
  QVector<MapDBEntrySign*> m_signs;

private slots:
  void initTestCase();
  void load_null_resets();
  void setTo_copiesEntry_andNullResets();
  void setToAll_buildsMatchingList();
  void randomize_nullIsNoOp();
  void randomizeAll_isAPermutationOfPositions();
};

void TestSignData::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);

  // Find the first map with >= 2 signs so the permutation case is meaningful.
  for(auto* map : MapsDB::inst()->getStore()) {
    if(map != nullptr && map->getSignsSize() >= 2) {
      m_map = map;
      m_signs = map->getSigns();
      break;
    }
  }
  QVERIFY2(m_map != nullptr, "no map in MapsDB has >= 2 signs");
  QVERIFY(m_signs.size() >= 2);
}

// load() begins with reset() and bails on a null save -> all fields zeroed,
// no dereference of the null SaveFile. This is the graceful-degradation path.
void TestSignData::load_null_resets()
{
  SignData s;
  s.x = 7; s.y = 8; s.txtId = 9;
  s.load(nullptr, 0);
  QCOMPARE(s.x, 0);
  QCOMPARE(s.y, 0);
  QCOMPARE(s.txtId, 0);
}

// setTo(entry) copies x/y/txtId from a map-defined sign; setTo(nullptr) resets
// (the null guard after reset()).
void TestSignData::setTo_copiesEntry_andNullResets()
{
  MapDBEntrySign* e = m_signs.at(0);
  SignData s;
  s.setTo(e);
  QCOMPARE(s.x, e->getX());
  QCOMPARE(s.y, e->getY());
  QCOMPARE(s.txtId, e->getTextID());

  s.setTo(nullptr); // must reset, not crash
  QCOMPARE(s.x, 0);
  QCOMPARE(s.y, 0);
  QCOMPARE(s.txtId, 0);
}

// setToAll builds one SignData per map sign, each matching its source entry.
void TestSignData::setToAll_buildsMatchingList()
{
  QVector<SignData*> built = SignData::setToAll(m_signs);
  QCOMPARE(built.size(), m_signs.size());
  for(int i = 0; i < built.size(); i++) {
    QCOMPARE(built.at(i)->x, m_signs.at(i)->getX());
    QCOMPARE(built.at(i)->y, m_signs.at(i)->getY());
    QCOMPARE(built.at(i)->txtId, m_signs.at(i)->getTextID());
  }
  qDeleteAll(built);
}

// randomize(nullptr): with no coordinate pool there is nothing to do, so the
// sign is left exactly as-is.
void TestSignData::randomize_nullIsNoOp()
{
  SignData s;
  s.x = 3; s.y = 4; s.txtId = 5;
  s.randomize(nullptr);
  QCOMPARE(s.x, 3);
  QCOMPARE(s.y, 4);
  QCOMPARE(s.txtId, 5);
}

// randomizeAll builds the (cpp-private) TmpSignPos pool from the map's signs and
// hands each new sign a distinct position drawn from it -- so the resulting set
// of (x,y) positions is exactly a permutation of the map's sign positions,
// regardless of the RNG sequence. This is the deterministic invariant that
// covers randomizeAll + the non-null branch of randomize().
void TestSignData::randomizeAll_isAPermutationOfPositions()
{
  QVector<SignData*> result = SignData::randomizeAll(m_signs);
  QCOMPARE(result.size(), m_signs.size());

  // Multiset of source positions encoded as "x,y" strings.
  QMap<QString, int> expected;
  for(auto* e : m_signs)
    expected[QStringLiteral("%1,%2").arg(e->getX()).arg(e->getY())]++;

  QMap<QString, int> got;
  for(auto* s : result)
    got[QStringLiteral("%1,%2").arg(s->x).arg(s->y)]++;

  QCOMPARE(got, expected);
  qDeleteAll(result);
}

QTEST_GUILESS_MAIN(TestSignData)
#include "tst_sign_data.moc"
