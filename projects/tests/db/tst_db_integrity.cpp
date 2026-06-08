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
 * @file tst_db_integrity.cpp
 * @brief Phase-1 sanity for the game databases: the JSON assets load and deep-link
 *        correctly. Catches a corrupted/half-edited asset or a broken deep-link pass
 *        immediately, instead of as a weird blank later in the UI.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>

class TestDbIntegrity : public QObject
{
  Q_OBJECT

private slots:
  void boots();
  void pokemonStoreIsComplete();
  void everyNonGlitchSpeciesDeepLinksType();
};

void TestDbIntegrity::boots()
{
  // DB::inst() runs the whole bootstrap (create -> load -> index -> deep-link).
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(DB::inst()->pokemon() != nullptr);
}

void TestDbIntegrity::pokemonStoreIsComplete()
{
  (void)DB::inst();
  // At least the 151 canonical species must be present (glitch entries may add more).
  QVERIFY2(PokemonDB::inst()->getStoreSize() >= int(pokemonDexCount),
           qPrintable(QStringLiteral("only %1 species loaded; expected >= %2")
                        .arg(PokemonDB::inst()->getStoreSize())
                        .arg(int(pokemonDexCount))));
}

void TestDbIntegrity::everyNonGlitchSpeciesDeepLinksType()
{
  (void)DB::inst();
  const QVector<PokemonDBEntry*> store = PokemonDB::inst()->getStore();
  QVERIFY(!store.isEmpty());

  for(PokemonDBEntry* e : store) {
    QVERIFY(e != nullptr);
    if(e->glitch)
      continue; // glitch mons legitimately lack a resolved type
    QVERIFY2(!e->name.isEmpty(), "a non-glitch species has an empty name");
    QVERIFY2(e->toType1 != nullptr,
             qPrintable(QStringLiteral("species '%1' did not deep-link its primary type")
                          .arg(e->name)));
  }
}

QTEST_GUILESS_MAIN(TestDbIntegrity)
#include "tst_db_integrity.moc"
