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
 * @file tst_fuzz.cpp
 * @brief Property-based fuzzing of the edit -> flatten -> recalc -> reopen loop.
 *        Over many DETERMINISTICALLY-seeded random field combinations it asserts
 *        the invariants from the principles doc: every edit persists across a real
 *        save+reload, the main checksum stays valid, and flatten is idempotent
 *        (flatten == flatten-after-re-expand). A fixed seed makes any failure
 *        reproducible from the logged iteration.
 */

#include <QtTest>
#include <QRandomGenerator>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldother.h>

using namespace pse_test;

namespace {
constexpr int kMainChecksumOffset = 0x3523;
constexpr int kMainChecksumStart  = 0x2598;
constexpr int kMainChecksumSize   = 0xF8B;
constexpr int kIters = 150;
}

class TestFuzz : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  struct Edit { unsigned int money; int coins; int hours, mins, secs, frames; bool badges[8]; };

  Edit randomEdit(QRandomGenerator& g)
  {
    Edit e;
    e.money = g.bounded(1000000u);   // 24-bit BCD max 999,999
    e.coins = int(g.bounded(10000u)); // 16-bit BCD max 9,999
    e.hours = int(g.bounded(256u)); e.mins = int(g.bounded(60u));
    e.secs  = int(g.bounded(60u));   e.frames = int(g.bounded(60u));
    for(bool& b : e.badges) b = (g.bounded(2u) == 1);
    return e;
  }

  void apply(SaveFile& sf, const Edit& e)
  {
    auto* b = sf.dataExpanded->player->basics;
    b->money = e.money; b->coins = e.coins;
    for(int i = 0; i < 8; i++) b->badgeSet(i, e.badges[i]);
    auto* pt = sf.dataExpanded->world->other->playtime;
    pt->hours = e.hours; pt->minutes = e.mins; pt->seconds = e.secs; pt->frames = e.frames;
  }

private slots:
  void initTestCase();
  void randomEdits_persistAndChecksumStaysValid();
  void flatten_isIdempotentOverRandomStates();
};

void TestFuzz::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestFuzz::randomEdits_persistAndChecksumStaysValid()
{
  QRandomGenerator g(0xC0FFEEu); // fixed seed -> reproducible

  for(int it = 0; it < kIters; it++) {
    SaveFile sf; loadInto(sf, m_orig);
    const Edit e = randomEdit(g);
    apply(sf, e);

    sf.flattenData();
    sf.toolset->recalcChecksums();

    // Reopen the flattened bytes in a fresh SaveFile (a real save->load cycle).
    SaveFile sf2; loadInto(sf2, snapshot(sf));

    const QString ctx = QStringLiteral("iter %1").arg(it);
    auto* b = sf2.dataExpanded->player->basics;
    QVERIFY2(b->money == e.money, qPrintable(ctx + " money"));
    QVERIFY2(b->coins == e.coins, qPrintable(ctx + " coins"));
    for(int i = 0; i < 8; i++)
      QVERIFY2(b->badgeAt(i) == e.badges[i], qPrintable(ctx + QStringLiteral(" badge %1").arg(i)));
    auto* pt = sf2.dataExpanded->world->other->playtime;
    QVERIFY2(pt->hours == e.hours && pt->minutes == e.mins &&
             pt->seconds == e.secs && pt->frames == e.frames, qPrintable(ctx + " playtime"));

    // The main checksum must be valid in the reopened save.
    const quint8 stored = quint8(snapshot(sf2).at(kMainChecksumOffset));
    const quint8 computed = sf2.toolset->getChecksum(kMainChecksumStart, kMainChecksumSize);
    QVERIFY2(stored == computed, qPrintable(ctx + " main checksum invalid after reopen"));
  }
}

void TestFuzz::flatten_isIdempotentOverRandomStates()
{
  QRandomGenerator g(0x1234567u);

  for(int it = 0; it < kIters; it++) {
    SaveFile sf; loadInto(sf, m_orig);
    apply(sf, randomEdit(g));

    sf.flattenData(); sf.toolset->recalcChecksums();
    const QByteArray a = snapshot(sf);

    sf.expandData();                 // re-expand from the flattened bytes...
    sf.flattenData(); sf.toolset->recalcChecksums();
    const QByteArray b = snapshot(sf);

    QVERIFY2(a == b, qPrintable(QStringLiteral("flatten not idempotent at iter %1").arg(it)));
  }
}

QTEST_GUILESS_MAIN(TestFuzz)
#include "tst_fuzz.moc"
