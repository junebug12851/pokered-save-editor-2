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
#include <pse-savefile/expanded/world/worldevents.h>
#include <pse-savefile/expanded/world/worldtowns.h>
#include <pse-savefile/expanded/world/worldtrades.h>
#include <pse-savefile/expanded/world/worldcompleted.h>
#include <pse-savefile/expanded/world/worldhidden.h>
#include <pse-savefile/expanded/world/worldmissables.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-savefile/expanded/world/worldscripts.h>
#include <pse-savefile/expanded/world/worldlocal.h>

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
  void events_roundTrip();
  void towns_roundTrip();
  void trades_roundTrip();
  void completed_roundTrip();
  void hidden_roundTrip();
  void missables_roundTrip();
  void general_roundTrip();
  void scripts_roundTrip();
  void scripts_writeExactlyTheirByte();
  void missables_writeExactlyTheirBit();
  void local_roundTrip();
  void local_writesExactlyItsBytes();

private:
  /// "0xNNNN, 0xNNNN" for an offset list, for readable failure messages.
  static QString describeDiff(const QVector<int>& offs)
  {
    QStringList s;
    for(int i : offs) s << QStringLiteral("0x%1").arg(i, 4, 16, QChar('0'));
    return s.join(QStringLiteral(", "));
  }
};

void TestWorld::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
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

void TestWorld::events_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* e = sf.dataExpanded->world->events;
  const int n = e->eventsCount();
  for(int i = 0; i < n; i++) e->eventsSet(i, (i % 3) == 0);

  sf.flattenData(); sf.expandData();

  auto* e2 = sf.dataExpanded->world->events;
  QCOMPARE(e2->eventsCount(), n);
  for(int i = 0; i < n; i++)
    QVERIFY2(e2->eventsAt(i) == ((i % 3) == 0),
             qPrintable(QStringLiteral("event %1 did not round-trip").arg(i)));
}

void TestWorld::towns_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* t = sf.dataExpanded->world->towns;
  const int n = t->townsCount();
  for(int i = 0; i < n; i++) t->townsSet(i, (i % 2) == 0);

  sf.flattenData(); sf.expandData();

  auto* t2 = sf.dataExpanded->world->towns;
  for(int i = 0; i < n; i++)
    QVERIFY2(t2->townsAt(i) == ((i % 2) == 0),
             qPrintable(QStringLiteral("town %1 did not round-trip").arg(i)));
}

void TestWorld::trades_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* t = sf.dataExpanded->world->trades;
  const int n = t->tradesCount();
  for(int i = 0; i < n; i++) t->tradesSet(i, (i % 2) == 1);

  sf.flattenData(); sf.expandData();

  auto* t2 = sf.dataExpanded->world->trades;
  for(int i = 0; i < n; i++)
    QVERIFY2(t2->tradesAt(i) == ((i % 2) == 1),
             qPrintable(QStringLiteral("trade %1 did not round-trip").arg(i)));
}

void TestWorld::completed_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* c = sf.dataExpanded->world->completed;
  c->obtainedOldRod = true;  c->obtainedGoodRod = false; c->obtainedSuperRod = true;
  c->obtainedLapras = true;  c->obtainedStarterPokemon = false;
  c->everHealedPokemon = true; c->satisfiedSaffronGuards = false; c->defeatedLorelei = true;

  sf.flattenData(); sf.expandData();

  auto* c2 = sf.dataExpanded->world->completed;
  QCOMPARE(c2->obtainedOldRod, true);
  QCOMPARE(c2->obtainedGoodRod, false);
  QCOMPARE(c2->obtainedSuperRod, true);
  QCOMPARE(c2->obtainedLapras, true);
  QCOMPARE(c2->obtainedStarterPokemon, false);
  QCOMPARE(c2->everHealedPokemon, true);
  QCOMPARE(c2->satisfiedSaffronGuards, false);
  QCOMPARE(c2->defeatedLorelei, true);
}

void TestWorld::hidden_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* h = sf.dataExpanded->world->hidden;
  for(int i = 0; i < h->hItemsCount(); i++) h->hItemsSet(i, (i % 3) == 0);
  for(int i = 0; i < h->hCoinsCount(); i++) h->hCoinsSet(i, (i % 2) == 0);

  sf.flattenData(); sf.expandData();

  auto* h2 = sf.dataExpanded->world->hidden;
  for(int i = 0; i < h2->hItemsCount(); i++)
    QVERIFY2(h2->hItemsAt(i) == ((i % 3) == 0), qPrintable(QStringLiteral("hidden item %1").arg(i)));
  for(int i = 0; i < h2->hCoinsCount(); i++)
    QVERIFY2(h2->hCoinsAt(i) == ((i % 2) == 0), qPrintable(QStringLiteral("hidden coin %1").arg(i)));
}

void TestWorld::missables_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* m = sf.dataExpanded->world->missables;
  const int n = m->missablesCount();
  for(int i = 0; i < n; i++) m->missablesSet(i, (i % 2) == 0);

  sf.flattenData(); sf.expandData();

  auto* m2 = sf.dataExpanded->world->missables;
  for(int i = 0; i < n; i++)
    QVERIFY2(m2->missablesAt(i) == ((i % 2) == 0), qPrintable(QStringLiteral("missable %1").arg(i)));
}

void TestWorld::general_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* g = sf.dataExpanded->world->general;
  g->lastMap = 40;
  g->lastBlackoutMap = 1;
  g->options->textSlowness = 5;     // 4-bit field
  g->options->battleStyleSet = true;
  g->options->battleAnimOff = true;
  g->letterDelay->normalDelay = true;
  g->letterDelay->dontDelay = false;

  sf.flattenData(); sf.expandData();

  auto* g2 = sf.dataExpanded->world->general;
  QCOMPARE(g2->lastMap, 40);
  QCOMPARE(g2->lastBlackoutMap, 1);
  QCOMPARE(g2->options->textSlowness, 5);
  QCOMPARE(g2->options->battleStyleSet, true);
  QCOMPARE(g2->options->battleAnimOff, true);
  QCOMPARE(g2->letterDelay->normalDelay, true);
  QCOMPARE(g2->letterDelay->dontDelay, false);
}

void TestWorld::scripts_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* s = sf.dataExpanded->world->scripts;
  const int n = s->scriptsCount();
  for(int i = 0; i < n; i++) s->scriptsSet(i, i % 100); // keep within a byte

  sf.flattenData(); sf.expandData();

  auto* s2 = sf.dataExpanded->world->scripts;
  for(int i = 0; i < n; i++)
    QVERIFY2(s2->scriptsAt(i) == (i % 100), qPrintable(QStringLiteral("script %1").arg(i)));
}

/// Keystone: one per-map script-progress value moves EXACTLY its own byte in the 0x289C block
/// (wOaksLabCurScript..; the layout — sizes + skips — is scripts.json's; offsets hardcoded here
/// INDEPENDENTLY from the json walk so a layout regression cannot hide itself).
void TestWorld::scripts_writeExactlyTheirByte()
{
  SaveFile base; loadInto(base, m_orig);
  base.flattenData();
  const QByteArray baseline = snapshot(base);

  struct Case { int ind; int off; int value; };     // values differ from BaseSAV's
  const QVector<Case> cases = {
    { 0,  0x289C, 3 },   // Oaks Lab       (baseline 18)
    { 1,  0x289D, 2 },   // Pallet Town    (baseline 5; a skip byte follows -- must NOT move)
    { 3,  0x28A0, 1 },   // Viridian City  (baseline 0; skip 2 follows)
    { 96, 0x2915, 1 },   // Route 18 Gate  (the last byte of the block)
  };

  for(const Case& c : cases)
  {
    SaveFile sf; loadInto(sf, m_orig);
    sf.dataExpanded->world->scripts->scriptsSet(c.ind, c.value);
    sf.flattenData();
    const QVector<int> moved = diffOffsets(baseline, snapshot(sf));
    QVERIFY2(moved == QVector<int>{ c.off },
             qPrintable(QStringLiteral("script %1: moved {%2}, expected {0x%3}")
                        .arg(c.ind).arg(describeDiff(moved)).arg(c.off, 4, 16, QChar('0'))));
  }
}

/// Keystone: one missable-visibility bit moves EXACTLY its own byte (and only its bit) in the
/// wToggleableObjectFlags block at 0x2852. set = HIDDEN (the polarity documented in
/// worldmissables.h). Ends at bit 227 -- the last real missable.
void TestWorld::missables_writeExactlyTheirBit()
{
  SaveFile base; loadInto(base, m_orig);
  base.flattenData();
  const QByteArray baseline = snapshot(base);

  struct Case { int ind; int off; int bit; bool baselineSet; };
  const QVector<Case> cases = {
    { 0,   0x2852, 0, true  },   // Prof. Oak (Pallet) -- hidden in BaseSAV
    { 7,   0x2852, 7, false },   // same byte, top bit
    { 227, 0x286E, 3, false },   // the very last missable
  };

  for(const Case& c : cases)
  {
    SaveFile sf; loadInto(sf, m_orig);
    sf.dataExpanded->world->missables->missablesSet(c.ind, !c.baselineSet);
    sf.flattenData();
    const QByteArray after = snapshot(sf);
    const QVector<int> moved = diffOffsets(baseline, after);
    QVERIFY2(moved == QVector<int>{ c.off },
             qPrintable(QStringLiteral("missable %1: moved {%2}, expected {0x%3}")
                        .arg(c.ind).arg(describeDiff(moved)).arg(c.off, 4, 16, QChar('0'))));
    const int delta = quint8(baseline[c.off]) ^ quint8(after[c.off]);
    QVERIFY2(delta == (1 << c.bit),
             qPrintable(QStringLiteral("missable %1 flipped 0x%2, expected bit %3")
                        .arg(c.ind).arg(delta, 2, 16, QChar('0')).arg(c.bit)));
  }
}

// WorldLocal -- the six map-specific minigame bytes (Vermilion locks, Cinnabar quiz opponent, Safari
// steps/balls/game-over). Domain + console verification: notes/reference/gym-safari-state.md.
void TestWorld::local_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* wl = sf.dataExpanded->world->local;
  wl->lock1 = 10;
  wl->lock2 = 12;
  wl->quizOpp = 7;
  wl->safariSteps = 758;      // 0x02F6 -- exercises the BIG-endian word
  wl->safariGameOver = true;
  wl->safariBallCount = 5;

  sf.flattenData(); sf.expandData();

  auto* wl2 = sf.dataExpanded->world->local;
  QCOMPARE(wl2->lock1, 10);
  QCOMPARE(wl2->lock2, 12);
  QCOMPARE(wl2->quizOpp, 7);
  QCOMPARE(wl2->safariSteps, 758);
  QCOMPARE(wl2->safariGameOver, true);
  QCOMPARE(wl2->safariBallCount, 5);

  // wSafariSteps is stored HIGH byte first (pret/pokered `dw` written HIGH then LOW; console-verified):
  // 758 == 0x02F6 -> 0x29B9 = 0x02, 0x29BA = 0xF6.
  QCOMPARE(int(sf.data[0x29B9]), 0x02);
  QCOMPARE(int(sf.data[0x29BA]), 0xF6);
}

/// Keystone: change ONE field, flatten, and diff the whole 32 KB against an untouched resave --
/// exactly the field's own byte(s) move, nothing else. (The safari word moves two; a flag moves its
/// one byte.) Offsets are WRAM - 0xAD54; all six verified in notes/reference/gym-safari-state.md.
void TestWorld::local_writesExactlyItsBytes()
{
  SaveFile base; loadInto(base, m_orig);
  base.flattenData();
  const QByteArray baseline = snapshot(base);

  struct Case { QString key; QVector<int> offs; };
  const QVector<Case> cases = {
    { QStringLiteral("lock1"),           { 0x29EF } },
    { QStringLiteral("lock2"),           { 0x29F0 } },
    { QStringLiteral("quizOpp"),         { 0x2CE4 } },
    { QStringLiteral("safariSteps"),     { 0x29B9, 0x29BA } },
    { QStringLiteral("safariGameOver"),  { 0x2CF2 } },
    { QStringLiteral("safariBallCount"), { 0x2CF3 } },
  };

  for(const Case& c : cases)
  {
    SaveFile sf; loadInto(sf, m_orig);
    auto* wl = sf.dataExpanded->world->local;
    if(c.key == QStringLiteral("lock1"))                wl->lock1 = 10;          // baseline 4
    else if(c.key == QStringLiteral("lock2"))           wl->lock2 = 12;          // baseline 7
    else if(c.key == QStringLiteral("quizOpp"))         wl->quizOpp = 7;         // baseline 0
    else if(c.key == QStringLiteral("safariSteps"))     wl->safariSteps = 758;   // baseline 468; both bytes move
    else if(c.key == QStringLiteral("safariGameOver"))  wl->safariGameOver = true; // baseline false
    else if(c.key == QStringLiteral("safariBallCount")) wl->safariBallCount = 5; // baseline 30

    sf.flattenData();
    const QVector<int> moved = diffOffsets(baseline, snapshot(sf));

    QVERIFY2(moved == c.offs,
             qPrintable(QStringLiteral("%1: moved {%2}, expected {%3}")
                        .arg(c.key, describeDiff(moved), describeDiff(c.offs))));
  }
}

QTEST_GUILESS_MAIN(TestWorld)
#include "tst_world.moc"
