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
 * @file tst_items_logic.cpp
 * @brief Behaviour of the item model below the Qt model layer: Item's amount clamp
 *        + DB-resolved pricing, and ItemStorageBox's add/move/sort/remove/worth
 *        operations and bag<->PC relocation -- plus a post-mutation byte round-trip.
 */

#include <QtTest>
#include <QSet>
#include <QVector>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

using namespace pse_test;

class TestItemsLogic : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  // Ensure the bag has at least `n` items, adding fresh ones if needed.
  static void ensureItems(ItemStorageBox* box, int n)
  {
    while(box->itemsCount() < n && box->itemsCount() < box->itemsMax())
      box->itemNew();
  }

  // True if no two slots in the box share an item index (per-list uniqueness).
  static bool allIndsUnique(ItemStorageBox* box)
  {
    QSet<int> seen;
    for(int i = 0; i < box->itemsCount(); i++) {
      int ind = box->itemAt(i)->ind;
      if(seen.contains(ind))
        return false;
      seen.insert(ind);
    }
    return true;
  }

  // Capture the current item-index order.
  static QVector<int> indOrder(ItemStorageBox* box)
  {
    QVector<int> out;
    for(int i = 0; i < box->itemsCount(); i++)
      out.append(box->itemAt(i)->ind);
    return out;
  }

private slots:
  void initTestCase();
  void item_amountClamps();
  void item_pricingResolves();
  void box_addBulkCountAndWorth();
  void box_moveReorders();
  void box_sortIsStableCountAndIdempotent();
  void box_removeShrinks();
  void box_roundTripAfterMutation();
  void box_relocateOneToPairedBox();
  void box_itemNewNeverDuplicates();
  void box_randomizeIsUniqueAndSorted();
};

void TestItemsLogic::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestItemsLogic::item_amountClamps()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 1);
  Item* it = bag->itemAt(0);
  QVERIFY(it != nullptr);

  it->setAmount(999); QCOMPARE(it->getAmount(), 99); // Gen-1 stack cap
  it->setAmount(0);   QCOMPARE(it->getAmount(), 1);  // floor is 1
  it->setAmount(42);  QCOMPARE(it->getAmount(), 42); // in-range passes through
}

void TestItemsLogic::item_pricingResolves()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 1);
  Item* it = bag->itemAt(0);

  // Give it a known, real, valued item so pricing has something to resolve.
  it->load(QStringLiteral("POTION"), 3);
  QVERIFY2(it->toItem() != nullptr, "POTION did not resolve to a DB entry");
  QCOMPARE(it->getAmount(), 3);

  QVERIFY(it->buyPriceOneMoney() >= 0);
  QVERIFY(it->sellPriceOneMoney() >= 0);
  // Whole-stack price is the per-one price times the amount.
  QCOMPARE(it->buyPriceAllMoney(), it->buyPriceOneMoney() * 3);
  (void)it->canSell(); // just must not crash
}

void TestItemsLogic::box_addBulkCountAndWorth()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;

  const int before = bag->itemsCount();
  QVERIFY(before < bag->itemsMax());
  bag->itemNew();
  QCOMPARE(bag->itemsCount(), before + 1);

  Item* it = bag->itemAt(bag->itemsCount() - 1);
  it->load(QStringLiteral("POTION"), 5);

  // Bulk count includes stack amounts, so it must exceed the distinct count.
  QVERIFY(bag->itemsCountBulk() >= bag->itemsCount());
  QVERIFY(bag->itemsAllBuyMoney() >= 0);
  QVERIFY(bag->itemsAllSellMoney() >= 0);
}

void TestItemsLogic::box_moveReorders()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 2);
  bag->itemAt(0)->ind = 10;
  bag->itemAt(1)->ind = 20;

  QVERIFY(bag->itemMove(0, 1)); // swap the two
  QCOMPARE(bag->itemAt(0)->ind, 20);
  QCOMPARE(bag->itemAt(1)->ind, 10);

  // Out-of-range / no-op moves are rejected.
  QVERIFY(!bag->itemMove(0, 0));
  QVERIFY(!bag->itemMove(0, bag->itemsCount() + 5));
}

void TestItemsLogic::box_sortIsStableCountAndIdempotent()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 3);

  const int count = bag->itemsCount();
  bag->sort();
  QCOMPARE(bag->itemsCount(), count); // sorting never changes the count

  QVector<int> firstOrder;
  for(int i = 0; i < count; i++) firstOrder.append(bag->itemAt(i)->ind);
  bag->sort(); // sorting again must be a no-op
  for(int i = 0; i < count; i++)
    QCOMPARE(bag->itemAt(i)->ind, firstOrder[i]);
}

void TestItemsLogic::box_removeShrinks()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 2);

  const int before = bag->itemsCount();
  bag->itemRemove(bag->itemsCount() - 1);
  QCOMPARE(bag->itemsCount(), before - 1);

  // Out-of-range removes are safe no-ops.
  bag->itemRemove(-1);
  bag->itemRemove(9999);
  QCOMPARE(bag->itemsCount(), before - 1);
}

void TestItemsLogic::box_roundTripAfterMutation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ensureItems(bag, 2);
  bag->itemAt(0)->load(QStringLiteral("POTION"), 7);
  bag->itemAt(1)->load(QStringLiteral("ANTIDOTE"), 2);

  const int count = bag->itemsCount();
  QVector<int> inds, amts;
  for(int i = 0; i < count; i++) { inds.append(bag->itemAt(i)->ind); amts.append(bag->itemAt(i)->getAmount()); }

  sf.flattenData(); sf.expandData();

  auto* bag2 = sf.dataExpanded->player->items;
  QCOMPARE(bag2->itemsCount(), count);
  for(int i = 0; i < count; i++) {
    QCOMPARE(bag2->itemAt(i)->ind, inds[i]);
    QCOMPARE(bag2->itemAt(i)->getAmount(), amts[i]);
  }
}

void TestItemsLogic::box_relocateOneToPairedBox()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;
  ItemStorageBox* dest = bag->destBox();
  if(dest == nullptr)
    QSKIP("bag has no paired destination box in this context");

  ensureItems(bag, 1);
  if(bag->relocateFull())
    QSKIP("paired box is full; cannot relocate");

  const int bagBefore = bag->itemsCount();
  const int destBefore = dest->itemsCount();
  QVERIFY(bag->relocateOne(0));
  QCOMPARE(bag->itemsCount(), bagBefore - 1);
  QCOMPARE(dest->itemsCount(), destBefore + 1);
}

// Filling a box one item at a time (the "+" button) must never produce a
// duplicate within that box.
void TestItemsLogic::box_itemNewNeverDuplicates()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;

  bag->reset();
  QCOMPARE(bag->itemsCount(), 0);

  // Add as many as the box will take; each add must keep every index distinct.
  for(int i = 0; i < bag->itemsMax(); i++) {
    const int before = bag->itemsCount();
    bag->itemNew();
    // The pool (103 real items) is larger than the bag, so each add lands.
    QCOMPARE(bag->itemsCount(), before + 1);
    QVERIFY2(allIndsUnique(bag), "itemNew produced a duplicate item index");
  }
}

// Re-Roll (randomize) must leave each box duplicate-free AND sorted, for both
// the bag and the PC box, independently (uniqueness is per-list).
void TestItemsLogic::box_randomizeIsUniqueAndSorted()
{
  SaveFile sf; loadInto(sf, m_orig);
  ItemStorageBox* boxes[2] = {
    sf.dataExpanded->player->items,
    sf.dataExpanded->storage->items
  };

  for(auto* box : boxes) {
    // Several iterations to be robust against the RNG.
    for(int iter = 0; iter < 10; iter++) {
      box->randomize();

      QVERIFY2(allIndsUnique(box), "randomize produced a duplicate within a box");

      // randomize() sorts by default: the order it leaves must already equal a
      // fresh sort (i.e. sorting again is a no-op).
      const QVector<int> afterRandomize = indOrder(box);
      box->sort();
      QCOMPARE(indOrder(box), afterRandomize);
    }
  }
}

QTEST_GUILESS_MAIN(TestItemsLogic)
#include "tst_items_logic.moc"
