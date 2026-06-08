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
#include <pse-db/moves.h>
#include <pse-db/itemsdb.h>
#include <pse-db/types.h>
#include <pse-db/mapsdb.h>
#include <pse-db/util/mapsearch.h>

class TestDbIntegrity : public QObject
{
  Q_OBJECT

private slots:
  void boots();
  void pokemonStoreIsComplete();
  void everyNonGlitchSpeciesDeepLinksType();
  void movesLoadedAndDeepLinkType();
  void typesLoadedWithNames();
  void itemsLoaded();
  void mapsSearchChainWorks();
  void allSubDbsLoadAndCount();
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

void TestDbIntegrity::movesLoadedAndDeepLinkType()
{
  (void)DB::inst();
  const QVector<MoveDBEntry*> store = MovesDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(MoveDBEntry* e : store) {
    QVERIFY(e != nullptr);
    if(e->glitch)
      continue;
    QVERIFY2(!e->name.isEmpty(), "a non-glitch move has an empty name");
    QVERIFY2(e->toType != nullptr,
             qPrintable(QStringLiteral("move '%1' did not deep-link its type").arg(e->name)));
  }
}

void TestDbIntegrity::typesLoadedWithNames()
{
  (void)DB::inst();
  const QVector<TypeDBEntry*> store = TypesDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(TypeDBEntry* e : store) {
    QVERIFY(e != nullptr);
    QVERIFY2(!e->name.isEmpty(), "a type entry has an empty name");
  }
}

void TestDbIntegrity::itemsLoaded()
{
  (void)DB::inst();
  QVERIFY(ItemsDB::inst()->getStoreSize() > 0);
  QVERIFY(ItemsDB::inst()->getStoreAt(0) != nullptr);
}

void TestDbIntegrity::mapsSearchChainWorks()
{
  (void)DB::inst();
  QVERIFY(MapsDB::inst()->getStoreSize() > 0);

  // The chainable finder returns results (exercises the whole isGood() chain).
  const int good = MapsDB::inst()->search()->isGood()->getMapCount();
  QVERIFY(good > 0);

  // isType(...) must run WITHOUT crashing and return a sane subset -- this is the
  // live regression guard for the MapSearch::isType() null-deref fixed 2026-06-07
  // (the crash in the map-randomizer path). We don't assert a specific type exists:
  // note isType("Cave") currently matches 0 maps (the cave tileset type string is
  // apparently not literally "Cave"), which the randomizer relies on -- tracked for
  // phase 7 (randomizer), since pickRandom() on an empty result returns nullptr.
  const int caves = MapsDB::inst()->search()->isGood()->isType("Cave")->getMapCount();
  QVERIFY2(caves >= 0 && caves <= good, "isType() produced an out-of-range count");
}

void TestDbIntegrity::allSubDbsLoadAndCount()
{
  DB* db = DB::inst();
  QVERIFY(db != nullptr);

  // Call getStoreSize() on every store-style sub-DB: proves each constructed +
  // loaded without crashing, and exercises their count paths.
  struct Sub { const char* name; int size; };
  const QVector<Sub> subs = {
    {"pokemon",      db->pokemon()->getStoreSize()},
    {"moves",        db->moves()->getStoreSize()},
    {"items",        db->items()->getStoreSize()},
    {"types",        db->types()->getStoreSize()},
    {"maps",         db->maps()->getStoreSize()},
    {"trainers",     db->trainers()->getStoreSize()},
    {"tilesets",     db->tilesets()->getStoreSize()},
    {"sprites",      db->sprites()->getStoreSize()},
    {"spriteSets",   db->spriteSets()->getStoreSize()},
    {"music",        db->music()->getStoreSize()},
    {"fly",          db->fly()->getStoreSize()},
    {"tmHms",        db->tmHms()->getStoreSize()},
    {"trades",       db->trades()->getStoreSize()},
    {"scripts",      db->scripts()->getStoreSize()},
    {"missables",    db->missables()->getStoreSize()},
    {"hiddenItems",  db->hiddenItems()->getStoreSize()},
    {"hiddenCoins",  db->hiddenCoins()->getStoreSize()},
    {"gameCorner",   db->gameCorner()->getStoreSize()},
    {"eventPokemon", db->eventPokemon()->getStoreSize()},
    {"starters",     db->starters()->getStoreSize()},
    {"fonts",        db->fonts()->getStoreSize()},
    {"credits",      db->credits()->getStoreSize()},
  };
  for(const Sub& s : subs)
    QVERIFY2(s.size >= 0, qPrintable(QStringLiteral("%1 returned a negative count").arg(s.name)));

  // The DBs that must carry real Gen 1 data.
  QVERIFY(db->pokemon()->getStoreSize()  >= 151);
  QVERIFY(db->moves()->getStoreSize()    > 0);
  QVERIFY(db->items()->getStoreSize()    > 0);
  QVERIFY(db->types()->getStoreSize()    > 0);
  QVERIFY(db->maps()->getStoreSize()     > 0);
  QVERIFY(db->trainers()->getStoreSize() > 0);
  QVERIFY(db->tilesets()->getStoreSize() > 0);
  QVERIFY(db->sprites()->getStoreSize()  > 0);
  QVERIFY(db->tmHms()->getStoreSize()    > 0);
  QVERIFY(db->fonts()->getStoreSize()    > 0);
  QVERIFY(db->starters()->getStoreSize() > 0);
}

QTEST_GUILESS_MAIN(TestDbIntegrity)
#include "tst_db_integrity.moc"
