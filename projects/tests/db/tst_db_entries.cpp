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
 * @file tst_db_entries.cpp
 * @brief Deeper-than-integrity checks of the richest db entry, PokemonDBEntry:
 *        a per-species deep-link COMPLETENESS sweep (every resolved-pointer vector
 *        matches its source name vector, every edge resolves), a self-documenting
 *        characterization of a known species (Bulbasaur), and the store lookup
 *        accessors incl. their negative paths.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>

class TestDbEntries : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void everySpecies_deepLinksAreComplete();
  void bulbasaur_characterization();
  void storeAccessors_andNegativePaths();
};

void TestDbEntries::initTestCase()
{
  QVERIFY(DB::inst() != nullptr); // full bootstrap incl. deep-link pass
}

// For every non-glitch species, the resolved (`to*`) vectors must line up 1:1
// with the source name/number vectors, and every evolution/learnset edge must
// resolve. This exercises the whole deep-link pass, not just the type link.
void TestDbEntries::everySpecies_deepLinksAreComplete()
{
  const QVector<PokemonDBEntry*> store = PokemonDB::inst()->getStore();
  QVERIFY(!store.isEmpty());

  for(PokemonDBEntry* e : store) {
    if(e->glitch)
      continue;

    const QString who = e->name;

    // Primary type always resolves; secondary only when a type2 string exists.
    QVERIFY2(e->toType1 != nullptr, qPrintable(who + ": toType1 null"));
    if(e->type2.isEmpty())
      QVERIFY2(e->toType2 == nullptr, qPrintable(who + ": stray toType2"));
    else
      QVERIFY2(e->toType2 != nullptr, qPrintable(who + ": toType2 unresolved"));

    // Capture moves + TM/HM lists resolve element-for-element.
    QCOMPARE(e->toInitial.size(), e->initial.size());
    for(MoveDBEntry* m : e->toInitial)
      QVERIFY2(m != nullptr, qPrintable(who + ": null resolved initial move"));

    QCOMPARE(e->toTmHmMove.size(), e->tmHm.size());
    for(MoveDBEntry* m : e->toTmHmMove)
      QVERIFY2(m != nullptr, qPrintable(who + ": null resolved TM/HM move"));

    // Level-up learnset entries each resolve their move.
    for(PokemonDBEntryMove* lm : e->moves) {
      QVERIFY2(lm != nullptr, qPrintable(who + ": null learnset entry"));
      QVERIFY2(lm->toMove != nullptr, qPrintable(who + ": learnset move unresolved"));
    }

    // Evolution edges resolve their target species.
    for(PokemonDBEntryEvolution* ev : e->evolution) {
      QVERIFY2(ev != nullptr, qPrintable(who + ": null evolution edge"));
      QVERIFY2(ev->toEvolution != nullptr, qPrintable(who + ": evolution target unresolved"));
    }
  }
}

// Anchors the fixture to known canon so the data + deep-link is self-documenting.
void TestDbEntries::bulbasaur_characterization()
{
  PokemonDBEntry* bulba = PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur"));
  QVERIFY(bulba != nullptr);

  QVERIFY(bulba->pokedex.has_value());
  // The DB stores pokedex 0-based (the UI adds 1 for display -- the documented
  // `dexInd+1` convention), so Bulbasaur's national #1 is value 0 here.
  QCOMPARE(int(*bulba->pokedex), 0);

  // Dual-typed; both resolve.
  QVERIFY(!bulba->type1.isEmpty());
  QVERIFY(!bulba->type2.isEmpty());
  QVERIFY(bulba->toType1 != nullptr);
  QVERIFY(bulba->toType2 != nullptr);

  // All Gen-1 base stats present.
  QVERIFY(bulba->baseHp.has_value());
  QVERIFY(bulba->baseAttack.has_value());
  QVERIFY(bulba->baseDefense.has_value());
  QVERIFY(bulba->baseSpeed.has_value());
  QVERIFY(bulba->baseSpecial.has_value());
  QVERIFY(bulba->catchRate.has_value());

  // Evolves; the first edge resolves to Ivysaur (#2).
  QVERIFY2(!bulba->evolution.isEmpty(), "Bulbasaur has no evolution edge");
  PokemonDBEntry* next = bulba->evolution.first()->toEvolution;
  QVERIFY(next != nullptr);
  QVERIFY(next->pokedex.has_value());
  QCOMPARE(int(*next->pokedex), 1); // Ivysaur = national #2, 0-based value 1
}

void TestDbEntries::storeAccessors_andNegativePaths()
{
  PokemonDB* db = PokemonDB::inst();
  const int n = db->getStoreSize();
  QVERIFY(n > 0);

  // Valid index resolves; the name-index round-trips.
  PokemonDBEntry* first = db->getStoreAt(0);
  QVERIFY(first != nullptr);
  QCOMPARE(db->getIndAt(first->name), first);

  // Negative paths: unknown name key and an out-of-range index return null,
  // not a crash.
  QVERIFY(db->getIndAt(QStringLiteral("NotARealSpecies__")) == nullptr);
  QVERIFY(db->getStoreAt(n + 100) == nullptr);
}

QTEST_GUILESS_MAIN(TestDbEntries)
#include "tst_db_entries.moc"
