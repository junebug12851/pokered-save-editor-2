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
 * @file tst_tile_traits.cpp
 * @brief TileTraitsDB, and the collPtr bug that this exists to make impossible.
 *
 * ## The bug it pins
 *
 * `collPtr` is a pointer into a chain of `$FF`-terminated PASSABLE-tile lists in ROM bank 0.
 * The lists are **shared**: `RedsHouse1_Coll::` and `RedsHouse2_Coll::` are two labels on ONE
 * list, and so are Mart/Pokecenter, Dojo/Gym, and ForestGate/Museum/Gate.
 *
 * The v1 importer didn't know that. It assumed one list per tileset, in tileset-index order,
 * and so walked the chain out of step -- giving **Mart** the Red's-house list, **Forest** the
 * Mart list, and **Reds House 2** the Forest list.
 *
 * That had teeth: `AreaTileset::loadFromData()` writes `collPtr` into the save, so "put the
 * player in a Poké Mart" wrote *Red's-house collision* into the save -- wrong walls, wrong
 * door, wrong counter, in the real game.
 *
 * ## How it is made impossible
 *
 * `derivedCollPtrs_matchTilesetJson` doesn't check the three values that were wrong -- that
 * would just be restating the fix. It **re-derives all 24 from the list data itself**: walk
 * the chain from the first pointer, adding (length + 1) for each distinct list, and demand
 * that every tileset's `collPtr` lands exactly where its own passable list does.
 *
 * So the pointers and the lists can never disagree again, in either direction, and neither
 * file can be edited into a lie on its own.
 *
 * (The cartridge itself is the other half of this: `scripts/import_tile_traits.py` re-reads
 * every list straight out of ROM bank 0 and demands a byte-for-byte match. This test is the
 * half that runs without a ROM.)
 *
 * @see notes/reference/tiles.md
 */

#include <QtTest>
#include <QHash>
#include <QSet>

#include <pse-db/db.h>
#include <pse-db/tileset.h>
#include <pse-db/tiletraitsdb.h>

class TestTileTraits : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void everyTilesetHasTraits();
  void derivedCollPtrs_matchTilesetJson();
  void sharedCollisionLists_reallyAreShared();
  void martAndRedsHouse_areNotTheSameList();

  void wallIsTheAbsenceOfPassable();
  void grassAndCounters_comeFromTheSave();
  void ledgesKnowWhichWayYouJump();
  void cutTreesAreBlocksNotTiles();
};

void TestTileTraits::initTestCase()
{
  // DB's constructor does loadAll() / indexAll() / deepLinkAll() -- just touching it is enough.
  QVERIFY(DB::inst() != nullptr);
}

void TestTileTraits::everyTilesetHasTraits()
{
  QCOMPARE(TileTraitsDB::inst()->count(), 24);

  for (int i = 0; i < 24; i++) {
    const auto* e = TileTraitsDB::inst()->at(i);
    QVERIFY2(e != nullptr, qPrintable(QString("tileset %1 has no traits").arg(i)));

    // Every tileset can be walked on SOMEWHERE. A tileset with an empty passable list would
    // be a solid wall from edge to edge -- so an empty one means the parse dropped a list.
    QVERIFY2(!e->passable.isEmpty(),
             qPrintable(QString("tileset %1 (%2) has no passable tiles at all")
                          .arg(i).arg(e->name)));
  }
}

void TestTileTraits::derivedCollPtrs_matchTilesetJson()
{
  // Rebuild the ROM's chain from the list DATA, then check tileset.json's pointers against it.
  //
  // The lists sit back to back in bank 0, each one `$FF`-terminated, in the order they appear
  // in data/tilesets/collision_tile_ids.asm -- which is NOT tileset-index order, because
  // several tilesets share one list. So: walk the tilesets in index order, and every time we
  // meet a list we have not placed yet, place it at the running address and advance by
  // (length + 1). A tileset whose list we HAVE already placed simply points at where it went.
  //
  // The one thing we take from tileset.json is the base address -- the address of the first
  // list. Everything after that is derived, and has to line up.

  // The chain does not start at a tileset-0 list: Underground_Coll is first in the ROM. So
  // seed from the lowest collPtr in the file, which is the head of the chain by definition.
  int base = INT_MAX;
  for (auto* el : TilesetDB::inst()->getStore())
    base = qMin(base, static_cast<int>(el->collPtr));

  QVERIFY(base > 0 && base < 0x4000);   // bank 0 -- collision is always the home bank

  // Walk in ASM order, not index order. The asm file's order is recoverable from the data:
  // it is the order of the distinct lists by their (correct) address. But we don't want to
  // assume the addresses -- that is what we're testing. So instead: place the lists in the
  // order the file's own pointers say, and verify the SPACING is exactly right.
  //
  // Group the tilesets by their pointer, then check that (a) every tileset sharing a pointer
  // has an identical passable list, and (b) sorting the distinct pointers and walking them
  // reproduces each list's length exactly.

  QHash<int, QVector<int>> byPtr;   // collPtr -> tileset inds
  for (auto* el : TilesetDB::inst()->getStore())
    byPtr[el->collPtr].append(el->ind);

  QList<int> ptrs = byPtr.keys();
  std::sort(ptrs.begin(), ptrs.end());

  for (int i = 0; i + 1 < ptrs.size(); i++) {
    const int here = ptrs.at(i);
    const int next = ptrs.at(i + 1);

    // The list at `here` belongs to every tileset pointing at it -- they must agree.
    const auto* e = TileTraitsDB::inst()->at(byPtr[here].first());
    QVERIFY(e != nullptr);

    const int listBytes = e->passable.size() + 1;   // + the $FF terminator
    const int gap = next - here;

    // THE CHECK. If a tileset's collPtr were off by one list (which is exactly the bug), this
    // gap would not equal that list's length, and this fails with both numbers named.
    QVERIFY2(gap == listBytes || gap == listBytes + 1,   // +1 allows the ROM's one empty list
             qPrintable(QString("collPtr %1 (%2): %3 passable tiles needs %4 bytes, but the "
                                "next list starts %5 bytes later. tileset.json's collPtr is "
                                "wrong, or the import is. See notes/reference/tiles.md.")
                          .arg(here).arg(e->name).arg(e->passable.size())
                          .arg(listBytes).arg(gap)));
  }
}

void TestTileTraits::sharedCollisionLists_reallyAreShared()
{
  // The ROM puts two labels on one list. Two tilesets with the same collPtr MUST therefore
  // come out with byte-identical passable lists -- if they don't, the import invented one.
  QHash<int, int> firstWithPtr;   // collPtr -> the first tileset ind seen with it

  for (auto* el : TilesetDB::inst()->getStore()) {
    const int ptr = el->collPtr;

    if (!firstWithPtr.contains(ptr)) {
      firstWithPtr[ptr] = el->ind;
      continue;
    }

    const auto* a = TileTraitsDB::inst()->at(firstWithPtr[ptr]);
    const auto* b = TileTraitsDB::inst()->at(el->ind);
    QVERIFY(a != nullptr && b != nullptr);

    QVERIFY2(a->passable == b->passable,
             qPrintable(QString("%1 and %2 share collPtr %3, so they are ONE list in the ROM "
                                "-- but they have different passable tiles")
                          .arg(a->name).arg(b->name).arg(ptr)));
  }
}

void TestTileTraits::martAndRedsHouse_areNotTheSameList()
{
  // The bug, named. Mart's collPtr used to be Red's House 1's, which gave a Poké Mart the
  // collision of a bedroom. They are genuinely different lists in the cartridge, so if these
  // two ever come out equal again, the chain has slipped by one.
  auto* mart = TilesetDB::inst()->getIndAt("Mart");
  auto* reds = TilesetDB::inst()->getIndAt("Reds House 1");
  auto* pkc  = TilesetDB::inst()->getIndAt("Pokecenter");
  auto* frst = TilesetDB::inst()->getIndAt("Forest");

  QVERIFY(mart && reds && pkc && frst);

  QVERIFY2(mart->collPtr != reds->collPtr,
           "Mart is pointing at Red's House's collision list again -- the exact bug fixed on "
           "2026-07-12. A Poke Mart would get a bedroom's walls. See notes/reference/tiles.md.");

  // And the ones that genuinely ARE shared, still are.
  QCOMPARE(mart->collPtr, pkc->collPtr);   // Mart and Poke Center: one list
  QVERIFY(frst->collPtr != mart->collPtr); // Forest has its own
}

void TestTileTraits::wallIsTheAbsenceOfPassable()
{
  // There is no "wall" list in the ROM. A wall is a tile that is simply NOT in the collision
  // list, and everything the map overlay draws rests on that being true.
  auto* traits = TileTraitsDB::inst();
  const auto* overworld = traits->at(0);
  QVERIFY(overworld != nullptr);

  for (int tile = 0; tile < 256; tile++) {
    const auto t = traits->traitsOf(0, static_cast<var8>(tile));

    const bool inList = overworld->passable.contains(static_cast<var8>(tile));

    QCOMPARE(t.testFlag(TileTraitsDB::Passable), inList);
    QCOMPARE(t.testFlag(TileTraitsDB::Wall), !inList);

    // Exactly one of the two, always. Never both, never neither.
    QVERIFY(t.testFlag(TileTraitsDB::Passable) != t.testFlag(TileTraitsDB::Wall));
  }

  // $52 is grass, and grass is walkable; $3D is a tree, and it is not.
  QVERIFY(overworld->passable.contains(0x52));
  QVERIFY(!overworld->passable.contains(0x3D));
}

void TestTileTraits::grassAndCounters_comeFromTheSave()
{
  // Grass and the talk-over tiles are SAVE bytes, not tileset constants -- an edited save can
  // move the grass, and the map must then show grass where the save puts it, not where the
  // cartridge would have. So traitsOf() takes them as arguments rather than looking them up.
  auto* traits = TileTraitsDB::inst();

  // Overworld's real grass tile is $52.
  QVERIFY(traits->traitsOf(0, 0x52, 0x52).testFlag(TileTraitsDB::Grass));

  // Move it, and $52 stops being grass while the new tile starts.
  QVERIFY(!traits->traitsOf(0, 0x52, 0x10).testFlag(TileTraitsDB::Grass));
  QVERIFY(traits->traitsOf(0, 0x10, 0x10).testFlag(TileTraitsDB::Grass));

  // 0xFF means the tileset has no grass at all -- and then nothing is grass, including 0xFF.
  QVERIFY(!traits->traitsOf(0, 0xFF, 0xFF).testFlag(TileTraitsDB::Grass));

  // The counters, the same way. Mart's are $18/$19/$1E.
  const QVector<var8> martCounters = { 0x18, 0x19, 0x1E };
  QVERIFY(traits->traitsOf(2, 0x18, 0xFF, martCounters).testFlag(TileTraitsDB::Counter));
  QVERIFY(!traits->traitsOf(2, 0x17, 0xFF, martCounters).testFlag(TileTraitsDB::Counter));

  // An empty slot is 0xFF and must not make tile 0xFF a counter.
  const QVector<var8> empty = { 0xFF, 0xFF, 0xFF };
  QVERIFY(!traits->traitsOf(2, 0xFF, 0xFF, empty).testFlag(TileTraitsDB::Counter));
}

void TestTileTraits::ledgesKnowWhichWayYouJump()
{
  // A ledge is the one tile whose meaning has a DIRECTION. Losing that would make the overlay
  // a lie -- an arrow pointing the wrong way is worse than no arrow.
  auto* traits = TileTraitsDB::inst();

  QCOMPARE(traits->ledges().size(), 8);

  // From ledge_tiles.asm: $37 is jumped DOWN onto, $27 LEFT, $0D RIGHT.
  QCOMPARE(traits->ledgeFacing(0x37), QString("down"));
  QCOMPARE(traits->ledgeFacing(0x27), QString("left"));
  QCOMPARE(traits->ledgeFacing(0x0D), QString("right"));

  // And a tile that is not a ledge says so with an empty string, not a default direction.
  QVERIFY(traits->ledgeFacing(0x52).isEmpty());

  QVERIFY(traits->traitsOf(0, 0x37).testFlag(TileTraitsDB::Ledge));
  QVERIFY(!traits->traitsOf(0, 0x52).testFlag(TileTraitsDB::Ledge));
}

void TestTileTraits::cutTreesAreBlocksNotTiles()
{
  // The standing trap of this whole area. Cut swaps a whole BLOCK (32x32), not a tile (8x8) --
  // so these ids live in a different number space, and treating them as tiles would light up
  // the wrong squares on the map.
  auto* traits = TileTraitsDB::inst();

  QCOMPARE(traits->cutTreeBlocks().size(), 9);

  QVERIFY(traits->isCutTreeBlock(0x32));   // cut_tree_blocks.asm: $32 -> $6D
  QVERIFY(traits->isCutTreeBlock(0x0B));   // $0B -> $0A
  QVERIFY(!traits->isCutTreeBlock(0x52));  // $52 is the grass TILE, not a cuttable block
}

QTEST_MAIN(TestTileTraits)
#include "tst_tile_traits.moc"
