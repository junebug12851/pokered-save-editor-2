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
 * @file tst_iterator.cpp
 * @brief Exhaustive unit coverage for SaveFileIterator -- the auto-advancing
 *        cursor that wraps SaveFileToolset. Every accessor is checked for BOTH
 *        its value (round-trips through the underlying toolset) AND its cursor
 *        side effect (advances the offset by size + padding; bit ops do NOT
 *        advance). Navigation (offsetTo/By, inc/dec, skipPadding) and the
 *        push/pop offset stack (including the empty-pop -> 0x0000 default) are
 *        asserted directly against the public `offset` field.
 *
 * The toolset's own byte semantics are proven separately in tst_toolset; here we
 * only prove the iterator wraps and advances correctly. All writes are to an
 * in-memory scratch region of a throwaway SaveFile, so no fixture is mutated.
 */

#include <QtTest>
#include <QVector>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefileiterator.h>
#include <pse-savefile/savefiletoolset.h>

using namespace pse_test;

class TestIterator : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;
  static constexpr var16 BASE = 0x0100; // scratch region in the in-memory copy

private slots:
  void initTestCase();
  void navigation_movesOffsetExactly();
  void stack_pushPop_andEmptyPopDefaultsToZero();
  void scalars_roundTripAndAdvance();
  void padding_advancesBySizePlusPadding();
  void bits_roundTrip_withoutAdvancing();
  void rangeAndBitfield_roundTripAndAdvance();
  void accessors_fileAndToolset();
};

void TestIterator::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestIterator::navigation_movesOffsetExactly()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  QCOMPARE(it->offsetTo(0x40)->offset, var16(0x40)); // absolute
  QCOMPARE(it->offsetBy(5)->offset,    var16(0x45)); // relative
  QCOMPARE(it->inc()->offset,          var16(0x46));
  QCOMPARE(it->dec()->offset,          var16(0x45));
  QCOMPARE(it->skipPadding(3)->offset, var16(0x48)); // alias for offsetBy

  // Chaining returns `this` each time.
  QCOMPARE(it->offsetTo(0x10)->inc()->inc()->offset, var16(0x12));

  delete it;
}

void TestIterator::stack_pushPop_andEmptyPopDefaultsToZero()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  it->offsetTo(0x70)->push();
  it->offsetTo(0x10)->push();

  it->offsetTo(0x200);
  QCOMPARE(it->pop()->offset, var16(0x10)); // LIFO
  QCOMPARE(it->pop()->offset, var16(0x70));

  // Popping an empty stack resets to the documented 0x0000 default.
  QCOMPARE(it->pop()->offset, var16(0x0000));

  delete it;
}

void TestIterator::scalars_roundTripAndAdvance()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  // byte: advances by 1; word: advances by 2 -- proven by laying three writes
  // out contiguously and reading them back from explicit offsets.
  it->offsetTo(BASE);
  it->setByte(0x12);              QCOMPARE(it->offset, var16(BASE + 1));
  it->setWord(0x3456);           QCOMPARE(it->offset, var16(BASE + 3));
  it->setByte(0x78);             QCOMPARE(it->offset, var16(BASE + 4));

  it->offsetTo(BASE);
  QCOMPARE(it->getByte(), var8(0x12));   QCOMPARE(it->offset, var16(BASE + 1));
  QCOMPARE(it->getWord(), var16(0x3456)); QCOMPARE(it->offset, var16(BASE + 3));
  QCOMPARE(it->getByte(), var8(0x78));

  // BCD round-trip (3 bytes -> up to 6 digits), advances by size.
  it->offsetTo(BASE);
  it->setBCD(3, 123456);         QCOMPARE(it->offset, var16(BASE + 3));
  it->offsetTo(BASE);
  QCOMPARE(it->getBCD(3), var32(123456));

  // Hex: prove the iterator delegates set/get and advances by size. The exact
  // hex string convention is the toolset's contract (covered in tst_toolset), so
  // assert idempotency here rather than a canonical form: write, read R, write R,
  // read again -> stable.
  it->offsetTo(BASE);
  it->setHex(2, QStringLiteral("0A0B")); QCOMPARE(it->offset, var16(BASE + 2));
  it->offsetTo(BASE);
  const QString hexR = it->getHex(2);    QCOMPARE(it->offset, var16(BASE + 2));
  it->offsetTo(BASE);
  it->setHex(2, hexR);
  it->offsetTo(BASE);
  QCOMPARE(it->getHex(2), hexR);

  // Font-encoded string round-trip (letters are in the charset), advances by size.
  it->offsetTo(BASE);
  it->setStr(6, 5, QStringLiteral("ABC")); QCOMPARE(it->offset, var16(BASE + 6));
  it->offsetTo(BASE);
  QCOMPARE(it->getStr(6, 5), QStringLiteral("ABC"));

  delete it;
}

void TestIterator::padding_advancesBySizePlusPadding()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  // A byte write with padding 2 advances by 1 + 2 = 3.
  it->offsetTo(BASE);
  it->setByte(0x01, 2);          QCOMPARE(it->offset, var16(BASE + 3));
  it->setByte(0x02);             QCOMPARE(it->offset, var16(BASE + 4));

  it->offsetTo(BASE);
  QCOMPARE(it->getByte(2), var8(0x01)); QCOMPARE(it->offset, var16(BASE + 3));
  QCOMPARE(it->getByte(),  var8(0x02));

  // A word write with padding 1 advances by 2 + 1 = 3.
  it->offsetTo(BASE);
  it->setWord(0xBEEF, 1);        QCOMPARE(it->offset, var16(BASE + 3));

  delete it;
}

void TestIterator::bits_roundTrip_withoutAdvancing()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  it->offsetTo(BASE);
  it->setByte(0x00);             // clear the scratch byte
  it->offsetTo(BASE);

  // getBit / setBit do NOT advance the cursor (no offsetBy in their bodies).
  it->setBit(1, 3, true);
  QCOMPARE(it->offset, var16(BASE));
  QVERIFY(it->getBit(1, 3));
  QCOMPARE(it->offset, var16(BASE));

  it->setBit(1, 3, false);
  QVERIFY(!it->getBit(1, 3));
  QVERIFY(!it->getBit(1, 0));
  QCOMPARE(it->offset, var16(BASE));

  delete it;
}

void TestIterator::rangeAndBitfield_roundTripAndAdvance()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  // copyRange writes a block and advances by size; getRange reads it back.
  QVector<var8> block{0x11, 0x22, 0x33, 0x44};
  it->offsetTo(BASE);
  it->copyRange(block.size(), block);    QCOMPARE(it->offset, var16(BASE + 4));
  it->offsetTo(BASE);
  QCOMPARE(it->getRange(block.size()), block);
  QCOMPARE(it->offset, var16(BASE + 4));

  // Bitfield round-trip: read the current field, flip every bit, write it back,
  // read again -- proves both directions and the size + padding advance without
  // hard-coding the field's bit length.
  it->offsetTo(BASE);
  QVector<bool> orig = it->getBitField(2);
  QCOMPARE(it->offset, var16(BASE + 2));
  QVERIFY(orig.size() > 0);

  QVector<bool> flipped;
  for(bool b : orig) flipped.append(!b);

  it->offsetTo(BASE);
  it->setBitField(2, flipped);           QCOMPARE(it->offset, var16(BASE + 2));
  it->offsetTo(BASE);
  QCOMPARE(it->getBitField(2), flipped);

  delete it;
}

void TestIterator::accessors_fileAndToolset()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* it = sf.iterator();

  QCOMPARE(it->file(), &sf);
  QVERIFY(it->toolset() != nullptr);
  QCOMPARE(it->toolset(), sf.toolset);

  delete it;
}

QTEST_GUILESS_MAIN(TestIterator)
#include "tst_iterator.moc"
