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
 * @file tst_pokedex.cpp
 * @brief PlayerPokedex coverage: the two 151-entry seen/owned bit-fields round-trip
 *        through flatten/expand independently; the live counts and the tri-state
 *        getState() stay consistent with the flags; and the bulk verbs (markAll,
 *        reset) set every entry as expected.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokedex.h>

using namespace pse_test;

class TestPokedex : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  static bool seenPattern(int i) { return (i % 2) == 0; }
  static bool ownedPattern(int i) { return (i % 3) == 0; }

private slots:
  void initTestCase();
  void seenOwned_roundTripIndependently();
  void counts_matchSetBits();
  void getState_isConsistentWithFlags();
  void markAll_and_reset();
};

void TestPokedex::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestPokedex::seenOwned_roundTripIndependently()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;
  const int n = dex->seenMax();
  QCOMPARE(n, dex->ownedMax());

  // Set two different patterns -- includes owned-without-seen cases, which the
  // two independent on-disk bit-fields must preserve.
  for(int i = 0; i < n; i++) {
    dex->seenSet(i, seenPattern(i));
    dex->ownedSet(i, ownedPattern(i));
  }

  sf.flattenData(); sf.expandData();

  auto* dex2 = sf.dataExpanded->player->pokedex;
  for(int i = 0; i < n; i++) {
    QVERIFY2(dex2->seenAt(i) == seenPattern(i),
             qPrintable(QStringLiteral("seen[%1] did not round-trip").arg(i)));
    QVERIFY2(dex2->ownedAt(i) == ownedPattern(i),
             qPrintable(QStringLiteral("owned[%1] did not round-trip").arg(i)));
  }
}

void TestPokedex::counts_matchSetBits()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;
  const int n = dex->seenMax();

  int expSeen = 0, expOwned = 0;
  for(int i = 0; i < n; i++) {
    dex->seenSet(i, seenPattern(i));
    dex->ownedSet(i, ownedPattern(i));
    if(seenPattern(i)) expSeen++;
    if(ownedPattern(i)) expOwned++;
  }

  QCOMPARE(dex->seenCount(), expSeen);
  QCOMPARE(dex->ownedCount(), expOwned);
}

void TestPokedex::getState_isConsistentWithFlags()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;
  const int n = dex->seenMax();

  for(int i = 0; i < n; i++) {
    dex->seenSet(i, seenPattern(i));
    dex->ownedSet(i, ownedPattern(i));
  }

  for(int i = 0; i < n; i++) {
    const int st = dex->getState(i);
    if(dex->ownedAt(i))
      QCOMPARE(st, int(PlayerPokedex::DexOwned));     // owned wins
    else if(dex->seenAt(i))
      QCOMPARE(st, int(PlayerPokedex::DexSeen));
    else
      QCOMPARE(st, int(PlayerPokedex::DexNone));
  }
}

void TestPokedex::markAll_and_reset()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;
  const int n = dex->ownedMax();

  dex->markAll(PlayerPokedex::DexOwned);
  QCOMPARE(dex->ownedCount(), n);
  for(int i = 0; i < n; i++)
    QCOMPARE(dex->getState(i), int(PlayerPokedex::DexOwned));

  dex->markAll(PlayerPokedex::DexSeen);
  QCOMPARE(dex->seenCount(), n);
  QCOMPARE(dex->ownedCount(), 0);
  for(int i = 0; i < n; i++)
    QCOMPARE(dex->getState(i), int(PlayerPokedex::DexSeen));

  dex->reset();
  QCOMPARE(dex->seenCount(), 0);
  QCOMPARE(dex->ownedCount(), 0);
}

QTEST_GUILESS_MAIN(TestPokedex)
#include "tst_pokedex.moc"
