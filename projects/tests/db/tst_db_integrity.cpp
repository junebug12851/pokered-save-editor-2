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
#include <pse-db/sprites.h>
#include <pse-db/hiddenItemsdb.h>
#include <pse-db/hiddencoinsdb.h>
#include <pse-db/flydb.h>
#include <pse-db/entries/flydbentry.h>
#include <pse-db/trades.h>
#include <pse-db/entries/hiddenitemdbentry.h>
#include <pse-db/entries/mapdbentry.h>
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
  void everySpriteHasAKnownGroup();
  void bothHiddenDbsLoadTheirOwnData();
  void everyHiddenPickupResolvesItsMapAndItem();
  void everyFlyDestinationSitsAtItsMapId();
  void everyTradeResolvesAndSitsAtItsBit();
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

/// Every sprite carries one of the five known Characters-bar groups, and every group has
/// somebody in it. The grouping is a curation (it groups the ARTWORK, not the role an
/// object_event gives a sprite) -- so a typo in the JSON would otherwise sit there silently
/// and quietly empty a shelf of the rail.
void TestDbIntegrity::everySpriteHasAKnownGroup()
{
  const QStringList known = { "Story", "Trainers", "Townsfolk", "Pokemon", "Objects" };
  QHash<QString, int> tally;

  const int n = SpritesDB::inst()->getStoreSize();
  QCOMPARE(n, 72);

  for(int i = 0; i < n; i++) {
    SpriteDBEntry* s = SpritesDB::inst()->getStoreAt(i);
    QVERIFY(s != nullptr);
    QVERIFY2(known.contains(s->group),
             qPrintable(QString("sprite '%1' has unknown group '%2'").arg(s->name, s->group)));
    tally[s->group]++;
  }

  for(const QString& g : known)
    QVERIFY2(tally.value(g) > 0, qPrintable(QString("group '%1' is empty").arg(g)));
}

/// Both hidden DBs must carry their OWN data -- the counts the cartridge fixes: 54 items, 12
/// coins (54 `hidden_item` + 12 `hidden_coin` rows in pret; `MAX_HIDDEN_ITEMS` caps items at
/// 112, of which only 54 are used).
///
/// This is a REGRESSION PIN, not a formality. HiddenItemsDB and HiddenCoinsDB share their
/// load() implementation in AbstractHiddenItemDB, and a `static bool once` inside a base-class
/// method is ONE static for the whole hierarchy -- so whichever DB was constructed second used
/// to hit an already-tripped guard and load NOTHING. `allSubDbsLoadAndCount` could not catch it:
/// it only asserts `>= 0`, and an empty store passes that happily. The counts are asserted
/// exactly, because these two numbers are fixed by the ROM and any drift is a real fault.
///
/// @see WorldHidden (hiddenItemCount / hiddenCoinCount are sized to exactly these).
void TestDbIntegrity::bothHiddenDbsLoadTheirOwnData()
{
  (void)DB::inst();

  // Literals, not the WorldHidden constants: this test links `db` only, and `hiddenItemCount` /
  // `hiddenCoinCount` live in pse-savefile. They MUST stay equal to those two -- the save model's
  // arrays are sized to exactly these, so a drift here is a load() overrun there.
  QCOMPARE(HiddenItemsDB::inst()->getStoreSize(), 54);
  QCOMPARE(HiddenCoinsDB::inst()->getStoreSize(), 12);

  // The two stores are genuinely different data, not the same file loaded twice -- the coins
  // all live in the Game Corner, the items do not.
  QVERIFY(HiddenItemsDB::inst()->getStoreAt(0) != HiddenCoinsDB::inst()->getStoreAt(0));

  // Each entry knows its own bit, and the bit IS the store position. This is the property the
  // whole feature stands on: save bit i == store index i == a real (map, x, y).
  for(int i = 0; i < HiddenItemsDB::inst()->getStoreSize(); i++) {
    QCOMPARE(HiddenItemsDB::inst()->getStoreAt(i)->getInd(), i);
    QCOMPARE(HiddenItemsDB::inst()->getStoreAt(i)->getIsCoin(), false);
  }
  for(int i = 0; i < HiddenCoinsDB::inst()->getStoreSize(); i++) {
    QCOMPARE(HiddenCoinsDB::inst()->getStoreAt(i)->getInd(), i);
    QCOMPARE(HiddenCoinsDB::inst()->getStoreAt(i)->getIsCoin(), true);
  }

  // Items and coins deep-link to SEPARATE per-map lists. If they shared one, an entry's `ind`
  // would be ambiguous -- two different save arrays, both numbered from 0.
  MapDBEntry* gameCorner = HiddenCoinsDB::inst()->getStoreAt(0)->getToMap();
  QVERIFY(gameCorner != nullptr);
  QCOMPARE(gameCorner->getToHiddenCoins().size(), 12); // every coin is in the Game Corner
  QCOMPARE(gameCorner->getToHiddenItems().size(), 0);  // and no hidden ITEM is
}

/// Every shipped hidden pickup deep-links to a real map and names a real item. Same spirit as
/// `everyShippedSignResolvesInItsMapsText`: the save's bit `i` IS row `i` of the ROM's coord
/// table, so a row that cannot find its map is a box we would draw in the wrong place -- or not
/// at all. The item name comes from `import_hidden_items.py` and must resolve against items.json.
void TestDbIntegrity::everyHiddenPickupResolvesItsMapAndItem()
{
  (void)DB::inst();

  // ⚠️ Guard against passing VACUOUSLY. Both loops below iterate the store, so an EMPTY store
  // satisfies them without checking anything -- and that is not hypothetical: while the
  // AbstractHiddenItemDB once-guard bug was live, this test passed green on zero entries. A
  // loop-over-everything test must first insist there is something to loop over.
  QVERIFY(HiddenItemsDB::inst()->getStoreSize() > 0);
  QVERIFY(HiddenCoinsDB::inst()->getStoreSize() > 0);

  for(int i = 0; i < HiddenItemsDB::inst()->getStoreSize(); i++) {
    HiddenItemDBEntry* e = HiddenItemsDB::inst()->getStoreAt(i);
    QVERIFY(e != nullptr);
    QVERIFY2(e->getToMap() != nullptr,
             qPrintable(QString("hidden item bit %1 ('%2') did not deep-link its map")
                          .arg(i).arg(e->getMap())));
    QVERIFY2(!e->getItem().isEmpty(),
             qPrintable(QString("hidden item bit %1 on '%2' has no item name")
                          .arg(i).arg(e->getMap())));
    QVERIFY2(ItemsDB::inst()->getIndAt(e->getItem()) != nullptr,
             qPrintable(QString("hidden item bit %1: '%2' is not a real item")
                          .arg(i).arg(e->getItem())));
  }

  for(int i = 0; i < HiddenCoinsDB::inst()->getStoreSize(); i++) {
    HiddenItemDBEntry* e = HiddenCoinsDB::inst()->getStoreAt(i);
    QVERIFY(e != nullptr);
    QVERIFY2(e->getToMap() != nullptr,
             qPrintable(QString("hidden coin bit %1 ('%2') did not deep-link its map")
                          .arg(i).arg(e->getMap())));
    // A coin pile with no coins in it would be a parse failure, not a real ROM value.
    QVERIFY2(e->getCoins() > 0,
             qPrintable(QString("hidden coin bit %1 on '%2' has %3 coins")
                          .arg(i).arg(e->getMap()).arg(e->getCoins())));
  }
}

/// Every fly destination's `ind` IS its map id -- because the cartridge says so twice.
///
/// `MarkTownVisitedAndLoadToggleableObjects` marks a town visited with
/// `ld a, [wCurMap]` / `ld c, a` / `FLAG_SET` on `wTownVisitedFlag`: **the flag index is the map
/// id, with no translation**. `ExternalMapEntries` (the town map's own name table) lists the same
/// eleven in the same order. So `fly.json`'s `ind` is not a list position anybody chose -- it is a
/// fact about the save, and it is checkable.
///
/// ⚠️ **This test exists because it was WRONG, for years, and shipped.** `fly.json` had Lavender
/// and Vermilion swapped and Saffron/Fuchsia/Cinnabar/Indigo rotated -- 6 of 11 -- and v1's towns
/// screen walked the list positionally, so ticking "Vermilion City" set Lavender Town's bit.
/// Nothing caught it because nothing ever asserted the list against the game: every name was real,
/// every name was a town, and the count was exact. **A plausible list is not a correct one.**
///
/// @see notes/reference/town-visited.md
void TestDbIntegrity::everyFlyDestinationSitsAtItsMapId()
{
  (void)DB::inst();

  // Must be able to fail: a loop over an empty store proves nothing (the AbstractHiddenItemDB
  // lesson, two tests up).
  QCOMPARE(FlyDB::inst()->getStoreSize(), 11); // NUM_CITY_MAPS -- fixed by ROM

  for(int i = 0; i < FlyDB::inst()->getStoreSize(); i++) {
    FlyDBEntry* e = FlyDB::inst()->getStoreAt(i);
    QVERIFY(e != nullptr);

    MapDBEntry* map = e->getToMap();
    QVERIFY2(map != nullptr,
             qPrintable(QString("fly destination '%1' did not deep-link its map").arg(e->getName())));

    // The keystone: the entry's ind, its store position, and its map's id are ONE number. If any
    // two disagree, a "visited" checkbox somewhere is about to tick the wrong town.
    QVERIFY2(e->getInd() == map->getInd(),
             qPrintable(QString("fly destination '%1' has ind %2 but its map's id is %3 -- the "
                                "save's bit for it is the MAP ID (MarkTownVisited: ld c, [wCurMap])")
                          .arg(e->getName()).arg(e->getInd()).arg(map->getInd())));
    QVERIFY2(e->getInd() == i,
             qPrintable(QString("fly destination '%1' is at store position %2 but carries ind %3 -- "
                                "a positional reader (v1's towns screen was one) would mislabel it")
                          .arg(e->getName()).arg(i).arg(e->getInd())));
  }
}

/// All ten in-game trades load, resolve their species, and sit at their own save bit -- and the
/// nine located ones resolve a map while the one unused one resolves none.
///
/// `DoInGameTradeDialogue` FLAG_TESTs bit `wWhichTrade` of `wCompletedInGameTradeFlags`, and the
/// store is loaded in bit order, so store index i must BE trade i. The map/coords were appended to
/// trades.json by import_trades.py (additive), and the whole point of the map screen is putting the
/// nine located trades on their trader's tile -- a trade that cannot find its map is a box drawn in
/// the wrong place, or not at all.
///
/// @see notes/reference/in-game-trades.md
void TestDbIntegrity::everyTradeResolvesAndSitsAtItsBit()
{
  (void)DB::inst();

  // Must be able to fail: a loop over an empty store proves nothing (the AbstractHiddenItemDB
  // lesson). NUM_NPC_TRADES is 10, fixed by ROM.
  QCOMPARE(TradesDB::inst()->getStoreSize(), 10);

  int located = 0, unused = 0;
  for(int i = 0; i < TradesDB::inst()->getStoreSize(); i++) {
    TradeDBEntry* t = TradesDB::inst()->getStoreAt(i);
    QVERIFY(t != nullptr);

    // Store index IS the save bit.
    QVERIFY2(t->ind == i,
             qPrintable(QString("trade at store index %1 carries ind %2 -- the save bit must equal "
                                "the store position").arg(i).arg(t->ind)));

    // Both species must resolve -- import_trades.py validated the names against pokemon.json, so a
    // null here means the DB and the data have genuinely drifted.
    QVERIFY2(t->toGive != nullptr,
             qPrintable(QString("trade %1 (%2): give species '%3' did not resolve")
                          .arg(i).arg(t->nickname).arg(t->give)));
    QVERIFY2(t->toGet != nullptr,
             qPrintable(QString("trade %1 (%2): get species '%3' did not resolve")
                          .arg(i).arg(t->nickname).arg(t->get)));

    if(t->unused) {
      unused++;
      // The unused trade has no NPC, so no map -- and must claim none.
      QVERIFY2(t->mapId < 0 && t->toMap == nullptr,
               qPrintable(QString("trade %1 (%2) is unused but resolved a map")
                            .arg(i).arg(t->nickname)));
    } else {
      located++;
      QVERIFY2(t->mapId >= 0,
               qPrintable(QString("trade %1 (%2) is used but has no map id")
                            .arg(i).arg(t->nickname)));
      QVERIFY2(t->toMap != nullptr,
               qPrintable(QString("trade %1 (%2): map id %3 did not deep-link")
                            .arg(i).arg(t->nickname).arg(t->mapId)));
    }
  }

  QCOMPARE(located, 9);
  QCOMPARE(unused, 1);

  // Cinnabar Lab Trade Room (168) is the one map with TWO trades -- the case that earns the
  // per-map list. If the (map id, text id) join ever collapses them, this drops to 1.
  MapDBEntry* cinnabar = MapsDB::inst()->getIndAt(QStringLiteral("168"));
  QVERIFY(cinnabar != nullptr);
  QCOMPARE(cinnabar->getToTradesSize(), 2);
}
QTEST_GUILESS_MAIN(TestDbIntegrity)
#include "tst_db_integrity.moc"
