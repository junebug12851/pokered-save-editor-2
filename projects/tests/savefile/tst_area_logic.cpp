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
 * @file tst_area_logic.cpp
 * @brief The area sub-trees' DB-population + randomize logic that tst_area.cpp's
 *        byte round-trips don't reach: AreaTileset (talk-tile accessors, randomize,
 *        loadFromData), AreaMap (conn accessors, toCurMap, randomize/setTo from a
 *        map, coordsToPtr), and AreaPokemon/AreaPokemonWild (wild randomize +
 *        operators, table randomize, setTo from a map).
 *
 * Map-DB links are resolved with MapsDB::deepLink() in initTestCase (the
 * tst_map_fragments / tst_sprite_data precedent), so the map-load paths are safe.
 *
 * Regression guards for three fixes (2026-06-08, project leadership-approved):
 *  - AreaTileset::loadFromData inverted ternary (crashed on null map, ignored the
 *    tileset on a real map);
 *  - PokemonBox Random_Pokedex off-by-one is covered in tst_pokemonbox, but the
 *    0-based dex range is exercised here too via AreaPokemonWild::randomize;
 *  - AreaPokemon::setTo bounds guard.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/pokemon.h>
#include <pse-db/tileset.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areapokemon.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/fragments/mapconndata.h>
#include <pse-savefile/expanded/fragments/warpdata.h>

class TestAreaLogic : public QObject
{
  Q_OBJECT

  // Find the first map satisfying a predicate (helper for picking real data).
  template<typename Pred>
  MapDBEntry* firstMap(Pred p)
  {
    int n = MapsDB::inst()->getStoreSize();
    for(int i = 0; i < n; i++) {
      MapDBEntry* m = MapsDB::inst()->getStoreAt(i);
      if(m != nullptr && p(m)) return m;
    }
    return nullptr;
  }

private slots:
  void initTestCase();

  // AreaTileset
  void tileset_talkOverAccessorsAndSwap();
  void tileset_randomize();
  void tileset_loadFromData_nullIsSafe();
  void tileset_loadFromData_fromMap();
  void tileset_loadFromData_randomType();

  // AreaMap
  void map_connAccessors();
  void map_toCurMap();
  void map_coordsToPtr();
  void map_randomizeFromMap();
  void map_randomizeNullIsSafe();

  // AreaPokemon
  void wild_randomizeAndOperators();
  void wild_explicitCtor();
  void areaPokemon_randomize();
  void areaPokemon_setToFromMap();
  void areaPokemon_setToNullIsSafe();

  // AreaWarps
  void warps_listAccessors();
  void warps_setToFromMap();
  void warps_randomizeFromMap();
};

void TestAreaLogic::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  MapsDB::inst()->deepLink();
}

// ----- AreaTileset --------------------------------------------------------

void TestAreaLogic::tileset_talkOverAccessorsAndSwap()
{
  AreaTileset t;
  QCOMPARE(t.talkingOverTilesCount(), 3);

  t.talkingOverTiles[0] = 0x11;
  t.talkingOverTiles[1] = 0x22;
  t.talkingOverTiles[2] = 0x33;
  QCOMPARE(t.talkingOverTilesAt(0), 0x11);
  QCOMPARE(t.talkingOverTilesAt(2), 0x33);

  t.talkingOverTilesSwap(0, 2);
  QCOMPARE(t.talkingOverTilesAt(0), 0x33);
  QCOMPARE(t.talkingOverTilesAt(2), 0x11);
}

void TestAreaLogic::tileset_randomize()
{
  AreaTileset t;
  t.current = 9; t.grassTile = 9; t.bank = 9;   // dirty it first
  t.randomize();
  QVERIFY(t.type >= 0 && t.type <= 2);           // type randomized in range
  QCOMPARE(t.current, 0);                         // everything else reset
  QCOMPARE(t.grassTile, 0);
  QCOMPARE(t.bank, 0);
}

void TestAreaLogic::tileset_loadFromData_nullIsSafe()
{
  // Regression: the inverted ternary used to dereference the null map here.
  AreaTileset t;
  t.current = 5;
  t.loadFromData(nullptr, false);
  QCOMPARE(t.current, 0);          // reset, no crash
  QCOMPARE(t.type, 0);
}

void TestAreaLogic::tileset_loadFromData_fromMap()
{
  // Regression: with a real map the function must actually load the map's tileset
  // (the inverted ternary previously discarded it and loaded all zeros).
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getToTileset() != nullptr; });
  QVERIFY2(map != nullptr, "no map resolved a tileset after deepLink");
  TilesetDBEntry* ts = map->getToTileset();

  AreaTileset t;
  t.loadFromData(map, false);
  QCOMPARE(t.current, (int)ts->ind);
  QCOMPARE(t.type, (int)ts->typeAsEnum());
  QCOMPARE(t.grassTile, (int)ts->grass);
  QCOMPARE(t.bank, (int)ts->bank);
  QCOMPARE(t.blockPtr, (int)ts->blockPtr);
  QCOMPARE(t.gfxPtr, (int)ts->gfxPtr);
  QCOMPARE(t.collPtr, (int)ts->collPtr);
}

void TestAreaLogic::tileset_loadFromData_randomType()
{
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getToTileset() != nullptr; });
  QVERIFY(map != nullptr);

  AreaTileset t;
  t.loadFromData(map, true);                 // randomType branch
  QVERIFY(t.type >= 0 && t.type <= 2);
  QCOMPARE(t.current, (int)map->getToTileset()->ind);  // non-type data still from map
}

// ----- AreaMap ------------------------------------------------------------

void TestAreaLogic::map_connAccessors()
{
  AreaMap m;                       // load(nullptr) -> reset -> empty
  QCOMPARE(m.connCount(), 0);

  const int dir = 0;               // some direction key
  m.connNew(dir);
  QCOMPARE(m.connCount(), 1);
  QVERIFY(m.connAt(dir) != nullptr);

  m.connNew(dir);                  // replacing the same dir keeps count at 1
  QCOMPARE(m.connCount(), 1);

  m.connRemove(dir);
  QCOMPARE(m.connCount(), 0);
  m.connRemove(dir);               // removing a missing dir is a safe no-op
  QCOMPARE(m.connCount(), 0);
}

void TestAreaLogic::map_toCurMap()
{
  AreaMap m;
  MapDBEntry* anyMap = MapsDB::inst()->getStoreAt(0);
  QVERIFY(anyMap != nullptr);
  m.curMap = anyMap->getInd();
  QVERIFY(m.toCurMap() != nullptr);
  QCOMPARE(m.toCurMap()->getInd(), anyMap->getInd());
}

void TestAreaLogic::map_coordsToPtr()
{
  AreaMap m;
  // Pure arithmetic helper; assert it is deterministic and monotonic in x.
  const int a = m.coordsToPtr(2, 3, 10);
  const int b = m.coordsToPtr(3, 3, 10);
  QCOMPARE(b - a, 1);              // +1 x -> +1 ptr
  const int c = m.coordsToPtr(2, 4, 10);
  QCOMPARE(c - a, 10 + 6);         // +1 y -> +(width+6)
}

void TestAreaLogic::map_randomizeFromMap()
{
  // Pick a map that actually has edge connections so MapConnData::loadFromData runs.
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getConnect().size() > 0; });
  QVERIFY2(map != nullptr, "no map with connections found");

  AreaMap m;
  m.randomize(map, 5, 5);
  QCOMPARE(m.curMap, (int)map->getInd());
  QCOMPARE(m.connCount(), map->getConnect().size());   // one AreaMap conn per map conn

  // setTo currently delegates to randomize; same result.
  AreaMap m2;
  m2.setTo(map, 5, 5);
  QCOMPARE(m2.curMap, (int)map->getInd());
}

void TestAreaLogic::map_randomizeNullIsSafe()
{
  AreaMap m;
  m.curMap = 40;
  m.connNew(0);
  m.randomize(nullptr, 0, 0);      // resets and bails
  QCOMPARE(m.curMap, 0);
  QCOMPARE(m.connCount(), 0);
}

// ----- AreaPokemon --------------------------------------------------------

void TestAreaLogic::wild_randomizeAndOperators()
{
  // Exercises the 0-based dex range [dex0..dex150] -- every roll must resolve to a
  // real species (a dex0 / dex151 off-by-one would null-deref or miss a species).
  for(int i = 0; i < 60; i++) {
    AreaPokemonWild w;
    w.randomize();
    QVERIFY(PokemonDB::inst()->getIndAt(QString::number(w.index)) != nullptr);
    QVERIFY(w.level >= 5 && w.level <= pokemonLevelMax);
  }

  AreaPokemonWild lo(1, 5), hi(1, 50);
  QVERIFY(lo < hi);
  QVERIFY(hi > lo);
}

void TestAreaLogic::wild_explicitCtor()
{
  AreaPokemonWild w(25, 17);
  QCOMPARE(w.index, 25);
  QCOMPARE(w.level, 17);

  w.load(99, 42);
  QCOMPARE(w.index, 99);
  QCOMPARE(w.level, 42);
}

void TestAreaLogic::areaPokemon_randomize()
{
  AreaPokemon ap;                  // blank (no save)
  ap.randomize();
  QVERIFY(ap.grassRate >= 0 && ap.grassRate <= 35);
  QVERIFY(ap.waterRate >= 0 && ap.waterRate <= 35);

  // If a rate is non-zero the corresponding table got real species.
  if(ap.grassRate > 0)
    QVERIFY(PokemonDB::inst()->getIndAt(QString::number(ap.grassMonsAt(0)->index)) != nullptr);
}

void TestAreaLogic::areaPokemon_setToFromMap()
{
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getMonRate() > 0; });
  QVERIFY2(map != nullptr, "no map with a land encounter rate found");

  AreaPokemon ap;
  ap.setTo(map);
  QCOMPARE(ap.grassRate, (int)map->getMonRate());
  QVERIFY(ap.grassMonsAt(0)->index > 0);            // a real species was copied in
}

void TestAreaLogic::areaPokemon_setToNullIsSafe()
{
  AreaPokemon ap;
  ap.setTo(nullptr);               // rates -> 0, early return, no crash
  QCOMPARE(ap.grassRate, 0);
  QCOMPARE(ap.waterRate, 0);
}

// ----- AreaWarps ----------------------------------------------------------

void TestAreaLogic::warps_listAccessors()
{
  AreaWarps w;                     // load(nullptr) -> reset -> empty list
  QCOMPARE(w.warpCount(), 0);
  QVERIFY(w.warpMax() > 0);

  w.warpNew();
  w.warpNew();
  QCOMPARE(w.warpCount(), 2);
  QVERIFY(w.warpAt(0) != nullptr);

  w.warpAt(0)->destMap = 5;
  w.warpAt(1)->destMap = 9;
  w.warpSwap(0, 1);
  QCOMPARE(w.warpAt(0)->destMap, 9);
  QCOMPARE(w.warpAt(1)->destMap, 5);

  w.warpRemove(0);
  QCOMPARE(w.warpCount(), 1);
  QCOMPARE(w.warpAt(0)->destMap, 5);
}

void TestAreaLogic::warps_setToFromMap()
{
  // setTo rebuilds the warp list from a map's warps-out and picks dungeon/special
  // destinations via isType("Cave")->pickRandom() -- which resolves under deepLink
  // (60 Cave maps), so this does NOT crash (the old "isType Cave == 0" note was the
  // deepLink-not-called landmine, not a wrong type string).
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getWarpOut().size() > 0; });
  QVERIFY2(map != nullptr, "no map with warps-out found");

  AreaWarps w;
  w.setTo(map);
  QCOMPARE(w.warpCount(), map->getWarpOut().size());
  // Dungeon/special destinations are real map ids (a Cave map and any good map).
  QVERIFY(MapsDB::inst()->getIndAt(QString::number(w.dungeonWarpDestMap)) != nullptr);
  QVERIFY(MapsDB::inst()->getIndAt(QString::number(w.specialWarpDestMap)) != nullptr);
}

void TestAreaLogic::warps_randomizeFromMap()
{
  MapDBEntry* map = firstMap([](MapDBEntry* m){ return m->getWarpOut().size() > 0; });
  QVERIFY(map != nullptr);

  AreaWarps w;
  w.randomize(map);                // randomizes non-return warps; runs clean
  QCOMPARE(w.warpCount(), map->getWarpOut().size());
}

QTEST_GUILESS_MAIN(TestAreaLogic)
#include "tst_area_logic.moc"
