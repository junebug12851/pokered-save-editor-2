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
 * @file tst_common.cpp
 * @brief Phase-3 unit tests for the common layer: fixed-width type aliases,
 *        the Random wrapper (bounds + degenerate-range + edge behavior), and
 *        Utility's URL hex encode/decode round-trip.
 */

#include <QtTest>
#include <type_traits>

#include <pse-common/types.h>
#include <pse-common/random.h>
#include <pse-common/utility.h>

class TestCommon : public QObject
{
  Q_OBJECT

private slots:
  void types_widthsAndSign();
  void random_rangeInclusive_isInBounds();
  void random_rangeExclusive_isInBounds();
  void random_rangeFloat_isInBounds();
  void random_degenerateRanges_returnStart();
  void random_coinFlipsVary();
  void utility_urlEncodeRoundTrip_data();
  void utility_urlEncodeRoundTrip();
};

void TestCommon::types_widthsAndSign()
{
  // A save editor depends on exact byte widths.
  QCOMPARE(int(sizeof(var8)), 1);
  QCOMPARE(int(sizeof(var16)), 2);
  QCOMPARE(int(sizeof(var32)), 4);
  QCOMPARE(int(sizeof(var64)), 8);
  QCOMPARE(int(sizeof(svar8)), 1);
  QCOMPARE(int(sizeof(svar16)), 2);
  QCOMPARE(int(sizeof(svar32)), 4);

  QVERIFY(std::is_unsigned<var8>::value);
  QVERIFY(std::is_unsigned<var16>::value);
  QVERIFY(std::is_unsigned<var32>::value);
  QVERIFY(std::is_signed<svar8>::value);
  QVERIFY(std::is_signed<svar16>::value);
  QVERIFY(std::is_signed<svar32>::value);
}

void TestCommon::random_rangeInclusive_isInBounds()
{
  Random* r = Random::inst();
  for(int i = 0; i < 2000; i++) {
    int v = r->rangeInclusive(10, 20);
    QVERIFY2(v >= 10 && v <= 20, qPrintable(QStringLiteral("got %1").arg(v)));
  }
}

void TestCommon::random_rangeExclusive_isInBounds()
{
  Random* r = Random::inst();
  for(int i = 0; i < 2000; i++) {
    int v = r->rangeExclusive(10, 20);
    QVERIFY2(v >= 10 && v < 20, qPrintable(QStringLiteral("got %1").arg(v)));
  }
}

void TestCommon::random_rangeFloat_isInBounds()
{
  Random* r = Random::inst();
  for(int i = 0; i < 2000; i++) {
    float f = r->range(5.0f);
    QVERIFY2(f >= 0.0f && f < 5.0f, qPrintable(QStringLiteral("got %1").arg(double(f))));
  }
}

void TestCommon::random_degenerateRanges_returnStart()
{
  Random* r = Random::inst();
  // Documented: start >= end returns start unchanged.
  QCOMPARE(r->rangeInclusive(7, 7), 7);
  QCOMPARE(r->rangeExclusive(7, 7), 7);
  QCOMPARE(r->rangeInclusive(20, 10), 20);
  QCOMPARE(r->rangeExclusive(20, 10), 20);
}

void TestCommon::random_coinFlipsVary()
{
  Random* r = Random::inst();
  bool sawTrue = false, sawFalse = false;
  for(int i = 0; i < 2000 && !(sawTrue && sawFalse); i++)
    (r->flipCoin() ? sawTrue : sawFalse) = true;
  QVERIFY2(sawTrue && sawFalse, "flipCoin() never varied over 2000 flips");
}

void TestCommon::utility_urlEncodeRoundTrip_data()
{
  QTest::addColumn<QString>("text");
  QTest::newRow("empty")  << QString("");
  QTest::newRow("letter") << QString("A");
  QTest::newRow("word")   << QString("Pikachu");
  QTest::newRow("spaces") << QString("Hello World");
  QTest::newRow("name")   << QString("RED");
}

void TestCommon::utility_urlEncodeRoundTrip()
{
  QFETCH(QString, text);
  Utility* u = Utility::inst();
  // encode -> decode must reproduce the original (the two are documented inverses).
  const QString encoded = u->encodeBeforeUrl(text);
  const QString decoded = u->decodeAfterUrl(encoded);
  QCOMPARE(decoded, text);
}

QTEST_GUILESS_MAIN(TestCommon)
#include "tst_common.moc"
