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
 * @file tst_world.cpp
 * @brief Phase-2 coverage of world-state fields: the playtime clock and the
 *        WorldOther odds-and-ends (debug flag, fossil bookkeeping). Value
 *        round-trips with boundary values.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldother.h>

using namespace pse_test;

class TestWorld : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();

  void playtime_roundTrip_data();
  void playtime_roundTrip();
  void worldOther_roundTrip();
};

void TestWorld::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestWorld::playtime_roundTrip_data()
{
  QTest::addColumn<int>("hours");   // max 255
  QTest::addColumn<int>("minutes"); // 0-59
  QTest::addColumn<int>("seconds"); // 0-59
  QTest::addColumn<int>("frames");  // 0-59
  QTest::addColumn<bool>("maxed");

  QTest::newRow("zero")    << 0   << 0  << 0  << 0  << false;
  QTest::newRow("typical") << 123 << 45 << 30 << 15 << false;
  QTest::newRow("ceiling") << 255 << 59 << 59 << 59 << false;
  QTest::newRow("maxed")   << 10  << 20 << 30 << 40 << true;
}

void TestWorld::playtime_roundTrip()
{
  QFETCH(int, hours); QFETCH(int, minutes); QFETCH(int, seconds);
  QFETCH(int, frames); QFETCH(bool, maxed);

  SaveFile sf; loadInto(sf, m_orig);
  auto* pt = sf.dataExpanded->world->other->playtime;
  pt->hours = hours; pt->minutes = minutes; pt->seconds = seconds;
  pt->frames = frames; pt->clockMaxed = maxed;

  sf.flattenData(); sf.expandData();

  auto* pt2 = sf.dataExpanded->world->other->playtime;
  QCOMPARE(pt2->hours, hours);
  QCOMPARE(pt2->minutes, minutes);
  QCOMPARE(pt2->seconds, seconds);
  QCOMPARE(pt2->frames, frames);
  QCOMPARE(pt2->clockMaxed, maxed);
}

void TestWorld::worldOther_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* wo = sf.dataExpanded->world->other;
  wo->debugMode = true;
  wo->fossilItemGiven = 0x29;
  wo->fossilPkmnResult = 100;

  sf.flattenData(); sf.expandData();

  auto* wo2 = sf.dataExpanded->world->other;
  QCOMPARE(wo2->debugMode, true);
  QCOMPARE(wo2->fossilItemGiven, 0x29);
  QCOMPARE(wo2->fossilPkmnResult, 100);
}

QTEST_GUILESS_MAIN(TestWorld)
#include "tst_world.moc"
