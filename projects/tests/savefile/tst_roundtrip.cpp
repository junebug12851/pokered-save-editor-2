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
 * @file tst_roundtrip.cpp
 * @brief Phase-1 guardrail for the project's prime value: byte-exact save fidelity.
 *
 * Two tests:
 *  - loadFlatten_isIdentity: open a real save, flatten + recalc checksums (the same
 *    pipeline FileManagement::saveFile/writeSaveData uses), and assert the resulting
 *    32 KB is byte-for-byte identical to the file we opened. This is "open then save
 *    with no edits must not change a single byte."
 *  - moneyEdit_touchesOnlyMoneyAndChecksum: change exactly one field (money) and
 *    assert that ONLY the money bytes and the main-data checksum byte changed, and
 *    that the new value round-trips back through raw->expand.
 *
 * Field offsets come from the independent oracle assets/savefile-structure.bt
 * (cross-checked against notes/reference/gen1-knowledge.md), NOT from the code under
 * test -- so a wrong offset in the engine can't quietly agree with a wrong offset
 * in the test.
 */

#include <QtTest>
#include <QByteArray>
#include <QSet>
#include <QVector>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

using namespace pse_test;

namespace {
// From assets/savefile-structure.bt (bank1 base 0x2000; main data begins after the
// 0x598 lead-in + the 0xB player name). Cross-checked with gen1-knowledge.md.
constexpr int kMoneyOffset   = 0x25F3; ///< Money: 24-bit BCD, 3 bytes.
constexpr int kMoneySize     = 3;
constexpr int kMainChecksum  = 0x3523; ///< Bank-1 main-data checksum byte.
} // namespace

class TestRoundTrip : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void loadFlatten_isIdentity_data();
  void loadFlatten_isIdentity();

  void moneyEdit_touchesOnlyMoneyAndChecksum();
};

void TestRoundTrip::initTestCase()
{
  // Boot the game databases once. Expansion deep-links species/moves/etc., so the
  // DB must be up before any SaveFile is expanded. DB::inst() is idempotent.
  QVERIFY(DB::inst() != nullptr);
}

void TestRoundTrip::loadFlatten_isIdentity_data()
{
  QTest::addColumn<QString>("fixture");
  QTest::newRow("progressed (default)") << QStringLiteral("BaseSAV.sav");
  QTest::newRow("new game")             << QStringLiteral("BaseSAV.new.sav");
}

void TestRoundTrip::loadFlatten_isIdentity()
{
  QFETCH(QString, fixture);

  const QByteArray orig = readSaveBytes(fixture);
  QCOMPARE(orig.size(), kSaveSize); // fixture present and full-size

  SaveFile sf;
  loadInto(sf, orig);                       // setData -> expandData
  sf.flattenData();                         // expanded -> raw (only managed bytes)
  const QByteArray afterFlatten = snapshot(sf);
  sf.toolset->recalcChecksums();            // the rest of the real save pipeline
  const QByteArray afterRecalc = snapshot(sf);

  // Split the two stages so a divergence is attributable: is it flatten writing a
  // managed byte differently, or recalcChecksums rewriting a checksum?
  const QVector<int> flattenDiffs = diffOffsets(orig, afterFlatten);
  const QVector<int> recalcDiffs  = diffOffsets(afterFlatten, afterRecalc);
  const QVector<int> totalDiffs   = diffOffsets(orig, afterRecalc);

  auto fmt = [](const QVector<int>& d, const QByteArray& a, const QByteArray& b) {
    QString s;
    for(int off : d.mid(0, 24))
      s += QStringLiteral("0x%1:%2->%3 ")
             .arg(off, 4, 16, QChar('0'))
             .arg(quint8(a[off]), 2, 16, QChar('0'))
             .arg(quint8(b[off]), 2, 16, QChar('0'));
    return s;
  };
  if(!totalDiffs.isEmpty()) {
    qWarning().noquote()
      << "\n  fixture:" << fixture
      << "\n  flatten-only changed" << flattenDiffs.size() << "byte(s):" << fmt(flattenDiffs, orig, afterFlatten)
      << "\n  recalcChecksums changed" << recalcDiffs.size() << "byte(s):" << fmt(recalcDiffs, afterFlatten, afterRecalc);
  }
  QVERIFY2(totalDiffs.isEmpty(),
           qPrintable(QStringLiteral("open->save changed %1 byte(s) for %2 (flatten=%3, recalc=%4)")
                        .arg(totalDiffs.size()).arg(fixture)
                        .arg(flattenDiffs.size()).arg(recalcDiffs.size())));
}

void TestRoundTrip::moneyEdit_touchesOnlyMoneyAndChecksum()
{
  const QByteArray orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(orig.size(), kSaveSize);

  SaveFile sf;
  loadInto(sf, orig);

  // Baseline = flatten + recalc with NO logical change. Comparing two flattens
  // isolates the money edit even if the full identity round-trip has a separate,
  // independently-tested imperfection.
  sf.flattenData();
  sf.toolset->recalcChecksums();
  const QByteArray base = snapshot(sf);

  // Edit exactly one field: money. Pick a value guaranteed to differ and valid as
  // 24-bit BCD (decimal digits only, <= 999999).
  PlayerBasics* basics = sf.dataExpanded->player->basics;
  const unsigned int oldMoney = basics->money;
  const unsigned int newMoney = (oldMoney == 123456u) ? 654321u : 123456u;
  basics->money = newMoney;

  sf.flattenData();
  sf.toolset->recalcChecksums();
  const QByteArray edited = snapshot(sf);

  // (1) Byte isolation: only money bytes and the main checksum may differ.
  QSet<int> allowed;
  for(int i = 0; i < kMoneySize; ++i)
    allowed.insert(kMoneyOffset + i);
  allowed.insert(kMainChecksum);

  const QVector<int> diffs = diffOffsets(base, edited);
  for(int off : diffs)
    QVERIFY2(allowed.contains(off),
             qPrintable(QStringLiteral("money edit touched unexpected offset 0x%1")
                          .arg(off, 0, 16)));

  // (2) The edit actually took effect (at least one money byte changed).
  bool moneyChanged = false;
  for(int i = 0; i < kMoneySize; ++i)
    if(base[kMoneyOffset + i] != edited[kMoneyOffset + i])
      moneyChanged = true;
  QVERIFY2(moneyChanged, "money edit produced no byte change");

  // (3) Value round-trips through raw -> expand.
  sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->money, newMoney);
}

QTEST_GUILESS_MAIN(TestRoundTrip)
#include "tst_roundtrip.moc"
