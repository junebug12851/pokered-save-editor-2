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
 * @file tst_toolset.cpp
 * @brief Phase-3 unit tests for SaveFileToolset's raw-byte primitives -- the
 *        foundation every edit rides on: BCD<->int, 16-bit words (incl. byte
 *        order), single bytes, individual bits (incl. non-disturbance of
 *        neighbours), and the Gen 1 checksum against a hand-computed value.
 *
 * These run on a blank SaveFile's buffer; no fixture needed.
 */

#include <QtTest>

#include <pse-common/types.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>

class TestToolset : public QObject
{
  Q_OBJECT

private slots:
  void bcd_intRoundTrip_data();
  void bcd_intRoundTrip();
  void bcd_bufferRoundTrip();
  void word_roundTripAndByteOrder();
  void byte_roundTrip();
  void bit_roundTripAndIsolation();
  void checksum_matchesHandComputed();
};

void TestToolset::bcd_intRoundTrip_data()
{
  QTest::addColumn<uint>("v");
  for(uint v : {0u, 1u, 9u, 10u, 99u, 100u, 999u, 1000u, 123456u, 999999u})
    QTest::newRow(qPrintable(QString::number(v))) << v;
}
void TestToolset::bcd_intRoundTrip()
{
  QFETCH(uint, v);
  SaveFile sf;
  // 3 BCD bytes hold up to 999999.
  QCOMPARE(sf.toolset->bcd2int(sf.toolset->int2bcd(v, 3)), var32(v));
}

void TestToolset::bcd_bufferRoundTrip()
{
  SaveFile sf;
  sf.toolset->setBCD(0x100, 3, 654321);
  QCOMPARE(sf.toolset->getBCD(0x100, 3), var32(654321));
  sf.toolset->setBCD(0x110, 2, 4242);
  QCOMPARE(sf.toolset->getBCD(0x110, 2), var32(4242));
}

void TestToolset::word_roundTripAndByteOrder()
{
  SaveFile sf;
  auto* ts = sf.toolset;

  ts->setWord(0x200, 0x1234);
  QCOMPARE(ts->getWord(0x200), var16(0x1234));
  // "0x12,0x34 <=> 0x1234": high byte first at the lower address.
  QCOMPARE(ts->getByte(0x200), var8(0x12));
  QCOMPARE(ts->getByte(0x201), var8(0x34));

  // Reversed read/write round-trips too.
  ts->setWord(0x210, 0x1234, true);
  QCOMPARE(ts->getWord(0x210, true), var16(0x1234));
}

void TestToolset::byte_roundTrip()
{
  SaveFile sf;
  sf.toolset->setByte(0x300, 0xAB);
  QCOMPARE(sf.toolset->getByte(0x300), var8(0xAB));
}

void TestToolset::bit_roundTripAndIsolation()
{
  SaveFile sf;
  auto* ts = sf.toolset;
  ts->setByte(0x400, 0x00);

  // Each bit sets and clears independently.
  for(int bit = 0; bit < 8; bit++) {
    ts->setBit(0x400, 1, bit, true);
    QVERIFY2(ts->getBit(0x400, 1, bit), qPrintable(QStringLiteral("bit %1 not set").arg(bit)));
    ts->setBit(0x400, 1, bit, false);
    QVERIFY2(!ts->getBit(0x400, 1, bit), qPrintable(QStringLiteral("bit %1 not cleared").arg(bit)));
  }

  // Setting one bit leaves the rest at zero.
  ts->setByte(0x400, 0x00);
  ts->setBit(0x400, 1, 3, true);
  QCOMPARE(ts->getByte(0x400), var8(0x08));
}

void TestToolset::checksum_matchesHandComputed()
{
  SaveFile sf;
  auto* ts = sf.toolset;
  ts->setByte(0x500, 0x10);
  ts->setByte(0x501, 0x20);
  // Gen 1 checksum: start at 0xFF and subtract each byte. 0xFF-0x10-0x20 = 0xCF.
  QCOMPARE(ts->getChecksum(0x500, 2), var8(0xCF));
}

QTEST_GUILESS_MAIN(TestToolset)
#include "tst_toolset.moc"
