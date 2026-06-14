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
 * @file tst_misc_regions.cpp
 * @brief The remaining top-level expanded regions not covered elsewhere: the
 *        Day Care (empty/reset + identity round-trip -- the empty path that used
 *        to crash the destructor), the Rival (name + starter round-trip), and the
 *        Hall of Fame (structural sweep of records/mons + an identity round-trip).
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/daycare.h>
#include <pse-savefile/expanded/rival.h>
#include <pse-savefile/expanded/halloffame.h>
#include <pse-savefile/expanded/fragments/hofrecord.h>
#include <pse-savefile/expanded/fragments/hofpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

class TestMiscRegions : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();
  void daycare_identityRoundTrip();
  void daycare_emptyAfterResetRoundTrips();
  void rival_roundTrip();
  void hof_structureIsSane();
  void hof_identityRoundTrip();
};

void TestMiscRegions::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

// Whatever state BaseSAV's Day Care is in (empty or occupied), it must survive a
// flatten -> re-expand unchanged, exercising load()'s both branches + save().
void TestMiscRegions::daycare_identityRoundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dc = sf.dataExpanded->daycare;

  const bool inUse = (dc->pokemon != nullptr);
  int species = -1, level = -1;
  if(inUse) { species = dc->pokemon->species; level = dc->pokemon->level; }

  sf.flattenData(); sf.expandData();

  auto* dc2 = sf.dataExpanded->daycare;
  QCOMPARE(dc2->pokemon != nullptr, inUse);
  if(inUse) {
    QCOMPARE(dc2->pokemon->species, species);
    QCOMPARE(dc2->pokemon->level, level);
  }
}

// reset() empties the Day Care (pokemon -> null). The empty state must flatten and
// re-expand to a still-empty Day Care -- this is the empty-destructor path that
// crashed before the session-12 fix, so it's a permanent regression guard.
void TestMiscRegions::daycare_emptyAfterResetRoundTrips()
{
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->daycare->reset();
  QVERIFY(sf.dataExpanded->daycare->pokemon == nullptr);

  sf.flattenData(); sf.expandData();

  QVERIFY2(sf.dataExpanded->daycare->pokemon == nullptr,
           "empty Day Care did not stay empty across a round-trip");
}

void TestMiscRegions::rival_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* r = sf.dataExpanded->rival;
  r->name = QStringLiteral("BLUE");
  r->starter = 4; // a small raw value; we assert it round-trips, not its meaning

  sf.flattenData(); sf.expandData();

  auto* r2 = sf.dataExpanded->rival;
  QCOMPARE(r2->name, QStringLiteral("BLUE"));
  QCOMPARE(r2->starter, 4);
}

void TestMiscRegions::hof_structureIsSane()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* hof = sf.dataExpanded->hof;

  const int count = hof->recordCount();
  const int max = hof->recordMax();
  QVERIFY(count >= 0);
  QVERIFY(max > 0);
  QVERIFY(count <= max);

  for(int i = 0; i < count; i++) {
    HoFRecord* rec = hof->recordAt(i);
    QVERIFY2(rec != nullptr, qPrintable(QStringLiteral("HoF record %1 null").arg(i)));

    const int mons = rec->pokemonCount();
    QVERIFY(mons >= 0);
    QVERIFY(mons <= rec->pokemonMax());
    for(int j = 0; j < mons; j++)
      QVERIFY2(rec->pokemonAt(j) != nullptr,
               qPrintable(QStringLiteral("HoF record %1 mon %2 null").arg(i).arg(j)));
  }
}

// The Hall of Fame's first recorded mon (if any) survives a round-trip.
void TestMiscRegions::hof_identityRoundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* hof = sf.dataExpanded->hof;
  if(hof->recordCount() == 0)
    QSKIP("BaseSAV has no Hall of Fame records to round-trip");

  HoFRecord* rec = hof->recordAt(0);
  if(rec->pokemonCount() == 0)
    QSKIP("first HoF record has no mons");

  HoFPokemon* mon = rec->pokemonAt(0);
  const int species = mon->species;
  const int level = mon->level;
  const QString name = mon->name;

  sf.flattenData(); sf.expandData();

  HoFPokemon* mon2 = sf.dataExpanded->hof->recordAt(0)->pokemonAt(0);
  QCOMPARE(mon2->species, species);
  QCOMPARE(mon2->level, level);
  QCOMPARE(mon2->name, name);
}

QTEST_GUILESS_MAIN(TestMiscRegions)
#include "tst_misc_regions.moc"
