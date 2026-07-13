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
 * @file tst_sprite_set.cpp
 * @brief The SPRITE SET -- the eleven sprite pictures the game loads for an outdoor map, and the id
 *        of the set they came from (`wSpriteSet` / `wSpriteSetID`, save 0x2649-0x2654).
 *
 * The keystone here is `splitSets_storeTheRESOLVEDId`. Twelve maps are too big for one set, so the
 * game's table gives them a **split id** ($F1-$FC) naming a dividing line and the two real sets
 * either side of it -- and `GetSplitMapSpriteSetID` resolves that to one of the TEN real ids
 * *before* anything is stored. So `wSpriteSetID` can only ever hold 0-10.
 *
 * Ours stored the split id itself, which is a value no console ever writes there. Fixed 2026-07-13.
 * See notes/reference/sprite-sets.md.
 */
#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/spriteSet.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>

#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>

using namespace pse_test;

class TestSpriteSet : public QObject
{
  Q_OBJECT

  QByteArray m_orig;

private slots:
  void initTestCase();

  void everySplitSet_resolvesToARealSet();
  void splitSets_storeTheRESOLVEDId();
  void loadingASet_fillsElevenSlots();
  void aSlotWrite_touchesOneByte();
};

void TestSpriteSet::initTestCase()
{
  DB::inst();
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// Whichever side of the dividing line you are on, a split set resolves to one of the ten real ones.
void TestSpriteSet::everySplitSet_resolvesToARealSet()
{
  int split = 0;

  for (auto* el : SpriteSetDB::inst()->getStore()) {
    if (!el->isDynamic())
      continue;

    ++split;

    // Both sides of the line, and the line itself.
    const QVector<QPair<int, int>> spots = { {0, 0}, {60, 60}, {255, 255} };

    for (const auto& at : spots) {
      const auto* resolved = el->getResolvedSet(static_cast<var8>(at.first),
                                                static_cast<var8>(at.second));

      QVERIFY2(resolved != nullptr, "a split set resolved to nothing");
      QVERIFY2(resolved->ind >= 1 && resolved->ind <= 10,
               qPrintable(QStringLiteral("split set %1 resolved to %2 -- the console only ever "
                                         "stores 1-10").arg(el->ind).arg(resolved->ind)));
      QVERIFY2(!resolved->isDynamic(), "a split set must not resolve to another split set");
    }
  }

  QVERIFY2(split == 12, "there are twelve split sets in the game");
}

/**
 * THE ONE THAT MATTERS: putting the player on a split-set map must store the RESOLVED id.
 *
 * Route 2's table entry is $F1. `GetSplitMapSpriteSetID` turns that into set 1 or set 2 depending on
 * which side of the dividing line the player is standing. `wSpriteSetID` gets THAT -- never $F1.
 */
void TestSpriteSet::splitSets_storeTheRESOLVEDId()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* cache = sf.dataExpanded->area->preloadedSprites;

  int checked = 0;

  for (auto* map : MapsDB::inst()->getStore()) {
    auto* set = map->getToSpriteSet();
    if (set == nullptr || !set->isDynamic())
      continue;

    // Both sides of the split, so both branches of the resolve are exercised.
    for (const QPair<int, int>& at : { qMakePair(0, 0), qMakePair(120, 120) }) {
      cache->setTo(map, at.first, at.second);

      QVERIFY2(cache->loadedSetId >= 1 && cache->loadedSetId <= 10,
               qPrintable(QStringLiteral("%1 stored sprite set id %2 -- a split id ($F1-$FC) is a "
                                         "value the game NEVER writes to wSpriteSetID")
                          .arg(map->getName()).arg(cache->loadedSetId)));
      ++checked;
    }
  }

  QVERIFY2(checked > 0, "no split-set map was found -- the test proved nothing");
}

/// Loading a set fills all eleven picture slots from that set.
void TestSpriteSet::loadingASet_fillsElevenSlots()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* cache = sf.dataExpanded->area->preloadedSprites;

  auto* pallet = MapsDB::inst()->getIndAt(QStringLiteral("0"));
  QVERIFY(pallet != nullptr);
  QVERIFY(pallet->getToSpriteSet() != nullptr);

  cache->reset();
  QCOMPARE(cache->loadedSetId, 0);

  cache->setTo(pallet, 5, 6);

  QCOMPARE(cache->lSpriteCount(), 11);
  QVERIFY2(cache->loadedSetId >= 1 && cache->loadedSetId <= 10, "Pallet Town has a real sprite set");

  int filled = 0;
  for (int i = 0; i < cache->lSpriteCount(); i++)
    if (cache->lSpriteAt(i) != 0)
      ++filled;

  QVERIFY2(filled == 11, "a sprite set is eleven pictures -- nine that walk and two that don't");
}

/// Editing one slot writes one byte. Not the id, not its neighbours.
void TestSpriteSet::aSlotWrite_touchesOneByte()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* cache = sf.dataExpanded->area->preloadedSprites;

  sf.dataExpanded->save(&sf);
  const QByteArray before = snapshot(sf);

  const int wasId = cache->loadedSetId;
  const int slot = 3;
  const int want = (cache->lSpriteAt(slot) == 7) ? 8 : 7;

  cache->lSpriteSet(slot, want);
  QCOMPARE(cache->lSpriteAt(slot), want);
  QCOMPARE(cache->loadedSetId, wasId);   // the id is NOT quietly "fixed" to match

  sf.dataExpanded->save(&sf);
  const QByteArray after = snapshot(sf);

  int changed = 0;
  for (int i = 0; i < before.size(); i++)
    if (before.at(i) != after.at(i))
      ++changed;

  QCOMPARE(changed, 1);                                       // exactly one byte
  QCOMPARE(static_cast<quint8>(after.at(0x2649 + slot)), static_cast<quint8>(want));
}

QTEST_GUILESS_MAIN(TestSpriteSet)
#include "tst_sprite_set.moc"
