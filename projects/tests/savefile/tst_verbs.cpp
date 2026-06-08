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
 * @file tst_verbs.cpp
 * @brief Phase-2 coverage of SaveFile's verbs, focused on byte fidelity:
 *  - resetData(): zeros the raw 32 KB buffer and blanks the expansion.
 *  - eraseExpansion(): blanks the expansion but must leave EVERY raw byte untouched.
 *  - randomizeExpansion(): fills the expansion but must leave EVERY raw byte
 *    untouched (it only changes the model; bytes change later, on flatten).
 *
 * The two "untouched" tests directly defend the project's prime value: an operation
 * that isn't a save must not write a single byte of the save.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

using namespace pse_test;

class TestVerbs : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();

  void resetData_zerosBufferAndBlanksExpansion();
  void eraseExpansion_leavesBytesUntouched();
  void randomizeExpansion_leavesBytesUntouched();
};

void TestVerbs::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestVerbs::resetData_zerosBufferAndBlanksExpansion()
{
  SaveFile sf; loadInto(sf, m_orig);
  sf.resetData();

  // The raw buffer is fully zeroed.
  const QByteArray d = snapshot(sf);
  int nonZero = 0;
  for(int i = 0; i < kSaveSize; i++) if(d[i] != 0) nonZero++;
  QCOMPARE(nonZero, 0);

  // The expansion reflects a blank save.
  QCOMPARE(sf.dataExpanded->player->basics->money, 0u);
  QCOMPARE(sf.dataExpanded->player->basics->getPlayerName(), QString());
}

void TestVerbs::eraseExpansion_leavesBytesUntouched()
{
  SaveFile sf; loadInto(sf, m_orig);
  const QByteArray before = snapshot(sf);

  sf.eraseExpansion();

  // Fidelity: erasing the editable model must not write any save byte.
  const QByteArray after = snapshot(sf);
  QCOMPARE(diffOffsets(before, after).size(), 0);

  // But the model itself is blanked.
  QCOMPARE(sf.dataExpanded->player->basics->money, 0u);
}

void TestVerbs::randomizeExpansion_leavesBytesUntouched()
{
  SaveFile sf; loadInto(sf, m_orig);
  const QByteArray before = snapshot(sf);

  // Map/area randomization is disabled (maps WIP), so the full tree randomizes
  // without crashing. Fidelity: randomizing the model changes the model only; the
  // raw save is not written until an explicit flatten/save. Not a single byte moves.
  sf.randomizeExpansion();

  const QByteArray after = snapshot(sf);
  const QVector<int> diffs = diffOffsets(before, after);
  QVERIFY2(diffs.isEmpty(),
           qPrintable(QStringLiteral("randomizeExpansion() touched %1 save byte(s); first 0x%2")
                        .arg(diffs.size())
                        .arg(diffs.isEmpty() ? 0 : diffs.first(), 0, 16)));
}

QTEST_GUILESS_MAIN(TestVerbs)
#include "tst_verbs.moc"
