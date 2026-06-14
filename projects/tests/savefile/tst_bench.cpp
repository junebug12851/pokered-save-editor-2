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
 * @file tst_bench.cpp
 * @brief Performance pins for the hot paths (QBENCHMARK: expand, flatten, full
 *        randomize) plus hard wall-clock UPPER BOUNDS that fail the test if an op
 *        ever hangs or slows by orders of magnitude. The bounds are the real point:
 *        a runtime hang once shipped (FontsDB::splice infinite loop); a bounded
 *        benchmark turns "silently slow / hung" into a red test.
 */

#include <QtTest>
#include <QElapsedTimer>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>

using namespace pse_test;

class TestBench : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  // Generous ceilings -- they exist to catch hangs / order-of-magnitude regressions,
  // not to police small fluctuations. Each op is milliseconds in practice.
  static constexpr qint64 kMaxExpandMs    = 1500;
  static constexpr qint64 kMaxFlattenMs   = 1500;
  static constexpr qint64 kMaxRandomizeMs = 3000;
  static constexpr qint64 kMaxFontConvMs  = 1000;

private slots:
  void initTestCase();

  void bench_expand();
  void bench_flatten();
  void bench_randomize();
  void fontConvert_completesQuickly();   // the FontsDB::splice hang guard
};

void TestBench::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestBench::bench_expand()
{
  SaveFile sf; loadInto(sf, m_orig);

  QElapsedTimer t; t.start();
  sf.expandData();
  QVERIFY2(t.elapsed() < kMaxExpandMs, "expandData() exceeded its time ceiling (hang?)");

  QBENCHMARK { sf.expandData(); } // record the metric for trend tracking
}

void TestBench::bench_flatten()
{
  SaveFile sf; loadInto(sf, m_orig);

  QElapsedTimer t; t.start();
  sf.flattenData();
  QVERIFY2(t.elapsed() < kMaxFlattenMs, "flattenData() exceeded its time ceiling (hang?)");

  QBENCHMARK { sf.flattenData(); }
}

void TestBench::bench_randomize()
{
  SaveFile sf; loadInto(sf, m_orig);

  QElapsedTimer t; t.start();
  sf.dataExpanded->randomize();
  QVERIFY2(t.elapsed() < kMaxRandomizeMs, "randomize() exceeded its time ceiling (hang?)");

  QBENCHMARK { sf.dataExpanded->randomize(); }
}

void TestBench::fontConvert_completesQuickly()
{
  FontsDB* f = FontsDB::inst();
  // A long string of varied glyphs -- the kind of input that tickled the historic
  // splice() infinite loop. Must complete well under the ceiling.
  const QString s = QStringLiteral("RED BLUE PIKACHU CHARIZARD MEWTWO 12345 ABCDEFGHIJ").repeated(4);

  QElapsedTimer t; t.start();
  const QVector<int> codes = f->convertToCode(s);
  const QString back = f->convertFromCode(codes);
  const qint64 ms = t.elapsed();

  QVERIFY2(ms < kMaxFontConvMs, "convertToCode/convertFromCode exceeded its time ceiling (splice hang?)");
  QVERIFY(!codes.isEmpty());
  Q_UNUSED(back);
}

QTEST_GUILESS_MAIN(TestBench)
#include "tst_bench.moc"
