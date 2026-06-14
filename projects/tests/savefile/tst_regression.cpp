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
 * @file tst_regression.cpp
 * @brief The bug history as a permanent test suite -- one named guard per real
 *        corruption/crash bug so it fails if the bug ever returns. Covers the
 *        original save-corruption commits (hidden items, party data, missables),
 *        the FontsDB::splice runtime hang, and the bugs this test effort itself
 *        found + fixed (Daycare empty-destructor crash, PlayerPokedex::reset
 *        blanking to all-true, CreditsDB::getStoreAt inverted guard, MapSearch
 *        isType null-deref).
 */

#include <QtTest>
#include <QElapsedTimer>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/creditsdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/util/mapsearch.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldhidden.h>
#include <pse-savefile/expanded/world/worldmissables.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokedex.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/daycare.h>

using namespace pse_test;

class TestRegression : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();

  void hiddenItems_roundTrip_e20c167();
  void partyData_roundTrip_cb6fc99();
  void missables_roundTrip_ff76662();
  void fontSplice_doesNotHang();
  void daycareEmpty_destructorAndRoundTrip_s12();
  void pokedexReset_blanksNotFills();
  void creditsGetStoreAt_boundsGuard();
  void mapSearchIsType_noNullDeref();
};

void TestRegression::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

// e20c167 -- hidden items were corrupted on save. Guard: the hidden-item flags
// round-trip through flatten/expand exactly.
void TestRegression::hiddenItems_roundTrip_e20c167()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* h = sf.dataExpanded->world->hidden;
  for(int i = 0; i < h->hItemsCount(); i++) h->hItemsSet(i, (i % 3) == 0);

  sf.flattenData(); sf.expandData();

  auto* h2 = sf.dataExpanded->world->hidden;
  for(int i = 0; i < h2->hItemsCount(); i++)
    QVERIFY2(h2->hItemsAt(i) == ((i % 3) == 0), qPrintable(QStringLiteral("hidden item %1").arg(i)));
}

// cb6fc99 -- party data corruption. Guard: the loaded party survives an identity
// round-trip (species/level of every party member unchanged).
void TestRegression::partyData_roundTrip_cb6fc99()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* party = sf.dataExpanded->player->pokemon;
  const int n = party->pokemon.size();
  QVERIFY2(n > 0, "BaseSAV party is empty");

  QVector<int> species, levels;
  for(int i = 0; i < n; i++) { species.append(party->pokemon.at(i)->species); levels.append(party->pokemon.at(i)->level); }

  sf.flattenData(); sf.expandData();

  auto* party2 = sf.dataExpanded->player->pokemon;
  QCOMPARE(party2->pokemon.size(), n);
  for(int i = 0; i < n; i++) {
    QCOMPARE(party2->pokemon.at(i)->species, species[i]);
    QCOMPARE(party2->pokemon.at(i)->level, levels[i]);
  }
}

// ff76662 -- missable sprite flags corrupted. Guard: they round-trip exactly.
void TestRegression::missables_roundTrip_ff76662()
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

// FontsDB::splice once spun forever on certain strings. Guard: a long conversion
// completes promptly.
void TestRegression::fontSplice_doesNotHang()
{
  const QString s = QStringLiteral("ABCDEFGHIJ KLMNOP 0123456789 RED BLUE").repeated(5);
  QElapsedTimer t; t.start();
  const QVector<int> codes = FontsDB::inst()->convertToCode(s);
  QVERIFY2(t.elapsed() < 1000, "convertToCode hung (splice regression)");
  QVERIFY(!codes.isEmpty());
}

// s12 -- an empty Day Care crashed its destructor (unconditional pokemon->deleteLater
// on a null). Guard: reset -> empty -> round-trip stays empty, no crash.
void TestRegression::daycareEmpty_destructorAndRoundTrip_s12()
{
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->daycare->reset();
  QVERIFY(sf.dataExpanded->daycare->pokemon == nullptr);
  sf.flattenData(); sf.expandData();
  QVERIFY(sf.dataExpanded->daycare->pokemon == nullptr);
}

// Found 2026-06-08 -- PlayerPokedex::reset() filled the dex with 151 (true) instead
// of blanking it. Guard: reset zeroes both counts.
void TestRegression::pokedexReset_blanksNotFills()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;
  dex->markAll(PlayerPokedex::DexOwned);   // make it non-empty first
  dex->reset();
  QCOMPARE(dex->ownedCount(), 0);
  QCOMPARE(dex->seenCount(), 0);
}

// Found 2026-06-08 -- CreditsDB::getStoreAt had an inverted guard (nullptr for valid
// indices, crash for out-of-range). Guard: valid resolves, out-of-range is null.
void TestRegression::creditsGetStoreAt_boundsGuard()
{
  CreditsDB* db = CreditsDB::inst();
  if(db->getStoreSize() > 0)
    QVERIFY(db->getStoreAt(0) != nullptr);
  QVERIFY(db->getStoreAt(-1) == nullptr);
  QVERIFY(db->getStoreAt(db->getStoreSize() + 100) == nullptr);
}

// Fixed earlier in the effort -- MapSearch::isType() dereferenced a null tileset.
// Guard: the search chain that uses it runs without crashing.
void TestRegression::mapSearchIsType_noNullDeref()
{
  const int caves = MapsDB::inst()->search()->startOver()->isType("Cave")->getMapCount();
  QVERIFY(caves >= 0); // value may be 0; the point is it doesn't crash
}

QTEST_GUILESS_MAIN(TestRegression)
#include "tst_regression.moc"
