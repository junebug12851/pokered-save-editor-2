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
 * @file tst_item_storage_model.cpp
 * @brief The bag/PC ItemStorageModel Qt-model surface: data()/setData() for every
 *        role (id/count/checked), the trailing empty-slot placeholder row, and the
 *        checked bulk ops (toggle-all, clear, move-to-bottom, delete) -- driven on a
 *        real Bridge over the BaseSAV bag, fresh fixture per test.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemstoragemodel.h>
#include <mvc/itemoverviewmodel.h>

using namespace pse_test;

class TestItemStorageModel : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;
  ItemStorageModel* m_bag = nullptr;   // brg.bagItemsModel
  ItemStorageBox* m_box = nullptr;     // the underlying bag box
  ItemStorageModel* m_pc = nullptr;    // brg.pcItemsModel (the paired box)
  ItemStorageBox* m_pcBox = nullptr;   // the underlying PC item box

  void ensureItems(int n)
  {
    while(m_box->itemsCount() < n && m_box->itemsCount() < m_box->itemsMax())
      m_box->itemNew();
  }
  void ensurePcItems(int n)
  {
    while(m_pcBox->itemsCount() < n && m_pcBox->itemsCount() < m_pcBox->itemsMax())
      m_pcBox->itemNew();
  }
  void check(int row, bool on)
  {
    m_bag->setData(m_bag->index(row), QVariant(on), ItemStorageModel::CheckedRole);
  }

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void data_rolesReflectTheItem();
  void placeholderRow_whenNotFull();
  void setData_writesIdCountAndChecked();
  void checkedToggleAll_andClear();
  void checkedMoveToBottom_reorders();
  void checkedDelete_removes();

  void dragReorder_movesWithinList();
  void dragReorder_groupMovesCheckedSet();
  void dragTransfer_movesToOtherBox();
  void dragTransfer_autoStacksOntoExisting();
  void dragTransfer_stacksOntoLastDuplicate();
  void dragTransfer_overflowAddsSecondRow();
  void dragTransfer_overflowRefusedWhenDstFull();
  void amountOfInd_sumsAcrossRows();
  void itemOverview_aggregatesSortsHidesZeros();
  void deleteItem_singleAndGroup();
};

void TestItemStorageModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
}

void TestItemStorageModel::init()
{
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("BaseSAV.sav")));
  m_brg = new Bridge(m_file);
  m_bag = m_brg->bagItemsModel;
  m_box = m_file->data->dataExpanded->player->items;
  m_pc = m_brg->pcItemsModel;
  m_pcBox = m_file->data->dataExpanded->storage->items;
}

void TestItemStorageModel::cleanup()
{
  delete m_brg; m_brg = nullptr;
  delete m_file; m_file = nullptr;
  m_bag = nullptr; m_box = nullptr;
}

void TestItemStorageModel::data_rolesReflectTheItem()
{
  ensureItems(1);
  m_box->itemAt(0)->load(QStringLiteral("POTION"), 5);

  const QModelIndex idx = m_bag->index(0);
  QCOMPARE(m_bag->data(idx, ItemStorageModel::IdRole).toInt(), m_box->itemAt(0)->ind);
  QCOMPARE(m_bag->data(idx, ItemStorageModel::CountRole).toInt(), 5);
  QCOMPARE(m_bag->data(idx, ItemStorageModel::CheckedRole).toBool(), false);
  QCOMPARE(m_bag->data(idx, ItemStorageModel::PlaceholderRole).toBool(), false);
  QVERIFY(!m_bag->roleNames().isEmpty());
  QCOMPARE(m_bag->data(QModelIndex(), ItemStorageModel::IdRole), QVariant()); // invalid index
}

void TestItemStorageModel::placeholderRow_whenNotFull()
{
  ensureItems(1);
  QVERIFY(m_box->itemsCount() < m_box->itemsMax());
  // Not full -> a trailing placeholder row exists past the real items.
  QCOMPARE(m_bag->rowCount(QModelIndex()), m_box->itemsCount() + 1);

  const QModelIndex ph = m_bag->index(m_box->itemsCount()); // the placeholder row
  QCOMPARE(m_bag->data(ph, ItemStorageModel::PlaceholderRole).toBool(), true);
  QCOMPARE(m_bag->data(ph, ItemStorageModel::IdRole).toInt(), -1);
}

void TestItemStorageModel::setData_writesIdCountAndChecked()
{
  ensureItems(1);
  const QModelIndex idx = m_bag->index(0);

  QVERIFY(m_bag->setData(idx, QVariant(20), ItemStorageModel::IdRole));
  QCOMPARE(m_box->itemAt(0)->ind, 20);

  QVERIFY(m_bag->setData(idx, QVariant(7), ItemStorageModel::CountRole));
  QCOMPARE(m_box->itemAt(0)->amount, 7);

  QVERIFY(m_bag->setData(idx, QVariant(true), ItemStorageModel::CheckedRole));
  QVERIFY(m_bag->hasChecked());
  QCOMPARE(m_bag->getChecked().size(), 1);
}

void TestItemStorageModel::checkedToggleAll_andClear()
{
  ensureItems(2);
  QVERIFY(!m_bag->hasChecked());

  m_bag->checkedToggleAll();
  QVERIFY(m_bag->hasChecked());
  QCOMPARE(m_bag->getChecked().size(), m_box->itemsCount());

  m_bag->clearCheckedState();
  QVERIFY(!m_bag->hasChecked());
}

void TestItemStorageModel::checkedMoveToBottom_reorders()
{
  ensureItems(2);
  const int n = m_box->itemsCount();
  Item* first = m_box->items.at(0);

  check(0, true);
  m_bag->checkedMoveToBottom();

  QCOMPARE(m_box->items.size(), n);                          // count preserved
  QCOMPARE(m_box->items.at(n - 1), first);                   // checked row went to the bottom
}

void TestItemStorageModel::checkedDelete_removes()
{
  ensureItems(2);
  const int before = m_box->itemsCount();
  Item* doomed = m_box->items.at(before - 1);

  check(before - 1, true);
  m_bag->checkedDelete();

  QCOMPARE(m_box->itemsCount(), before - 1);
  QVERIFY2(!m_box->items.contains(doomed), "deleted item still present");
}

// ---- Drag & drop (the list analogue of PokemonStorageModel's grid drag) ------

void TestItemStorageModel::dragReorder_movesWithinList()
{
  // BaseSAV's bag is already populated, so assert relative to the live count.
  ensureItems(3);
  const int n = m_box->itemsCount();
  QVERIFY(n >= 3);
  Item* a = m_box->items.at(0);
  Item* b = m_box->items.at(1);
  Item* c = m_box->items.at(2);

  // Drop A onto slot 2 (== C): insert A *before* C -> [B, A, C, ...].
  m_bag->dragReorder(0, 2, false);

  QCOMPARE(m_box->itemsCount(), n);   // count preserved
  QCOMPARE(m_box->items.at(0), b);
  QCOMPARE(m_box->items.at(1), a);
  QCOMPARE(m_box->items.at(2), c);
}

void TestItemStorageModel::dragReorder_groupMovesCheckedSet()
{
  ensureItems(4);
  const int n = m_box->itemsCount();
  QVERIFY(n >= 4);
  Item* a = m_box->items.at(0);
  Item* c = m_box->items.at(2);

  // Check A and C, then group-drag to the end (toIndex == count): the checked set
  // lands at the bottom, keeping its internal order -> [..., A, C].
  check(0, true);
  check(2, true);
  m_bag->dragReorder(0, m_box->itemsCount(), true);

  QCOMPARE(m_box->itemsCount(), n);
  QCOMPARE(m_box->items.at(n - 2), a);   // checked set kept its internal order
  QCOMPARE(m_box->items.at(n - 1), c);
}

void TestItemStorageModel::dragTransfer_movesToOtherBox()
{
  ensureItems(2);
  ensurePcItems(2);
  Item* moved = m_box->items.at(0);
  const int bagBefore = m_box->itemsCount();
  const int pcBefore = m_pcBox->itemsCount();

  // Drag bag item 0 into the PC box at slot 0.
  m_bag->dragTransfer(0, 0, false);

  QCOMPARE(m_box->itemsCount(), bagBefore - 1);
  QCOMPARE(m_pcBox->itemsCount(), pcBefore + 1);
  QVERIFY2(!m_box->items.contains(moved), "transferred item still in the bag");
  QCOMPARE(m_pcBox->items.at(0), moved);     // landed at the requested drop slot
}

void TestItemStorageModel::dragTransfer_autoStacksOntoExisting()
{
  // Bag: one Antidote x3.  PC: one Antidote x5.
  m_box->reset();   m_box->itemNew();   m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 3);
  m_pcBox->reset(); m_pcBox->itemNew(); m_pcBox->items.at(0)->load(QStringLiteral("ANTIDOTE"), 5);
  Item* dstStack = m_pcBox->items.at(0);
  const int antidote = dstStack->ind;

  m_bag->dragTransfer(0, 0, false);

  QCOMPARE(m_box->itemsCount(), 0);            // moved out of the bag
  QCOMPARE(m_pcBox->itemsCount(), 1);          // no duplicate row -- stacked
  QCOMPARE(m_pcBox->items.at(0), dstStack);    // same existing row
  QCOMPARE(dstStack->ind, antidote);
  QCOMPARE(dstStack->amount, 8);               // 5 + 3
}

void TestItemStorageModel::dragTransfer_stacksOntoLastDuplicate()
{
  // PC already has TWO Antidotes (pre-existing duplicate, left as-is) with a
  // Potion between them; the moved Antidote must fold into the LAST one.
  m_pcBox->reset();
  m_pcBox->itemNew(); m_pcBox->items.at(0)->load(QStringLiteral("ANTIDOTE"), 5);
  m_pcBox->itemNew(); m_pcBox->items.at(1)->load(QStringLiteral("POTION"),   2);
  m_pcBox->itemNew(); m_pcBox->items.at(2)->load(QStringLiteral("ANTIDOTE"), 7);
  m_box->reset(); m_box->itemNew(); m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 3);

  Item* firstAntidote = m_pcBox->items.at(0);
  Item* lastAntidote  = m_pcBox->items.at(2);

  m_bag->dragTransfer(0, 0, false);

  QCOMPARE(m_pcBox->itemsCount(), 3);          // no new row
  QCOMPARE(firstAntidote->amount, 5);          // earlier duplicate untouched
  QCOMPARE(lastAntidote->amount, 10);          // 7 + 3 -> stacked onto the LAST
}

void TestItemStorageModel::dragTransfer_overflowAddsSecondRow()
{
  // PC: Antidote x95.  Bag: Antidote x10.  95+10 > 99, so it must NOT clamp/lose
  // -- the moved item becomes its own 2nd row, full amount preserved.
  m_pcBox->reset(); m_pcBox->itemNew(); m_pcBox->items.at(0)->load(QStringLiteral("ANTIDOTE"), 95);
  m_box->reset();   m_box->itemNew();   m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 10);
  Item* original = m_pcBox->items.at(0);
  Item* moved = m_box->items.at(0);
  const int antidote = original->ind;

  m_bag->dragTransfer(0, 0, false);

  QCOMPARE(m_box->itemsCount(), 0);            // fully moved out -- no loss
  QCOMPARE(m_pcBox->itemsCount(), 2);          // overflow became a 2nd row
  QVERIFY(m_pcBox->items.contains(original));
  QVERIFY(m_pcBox->items.contains(moved));
  QCOMPARE(original->amount, 95);              // existing stack untouched (not topped)
  QCOMPARE(moved->amount, 10);                 // moved amount preserved exactly
  QCOMPARE(moved->ind, antidote);
}

void TestItemStorageModel::dragTransfer_overflowRefusedWhenDstFull()
{
  // Fill PC to capacity, make the LAST row an Antidote x95.
  m_pcBox->reset();
  ensurePcItems(m_pcBox->itemsMax());
  QCOMPARE(m_pcBox->itemsCount(), m_pcBox->itemsMax());
  m_pcBox->items.at(m_pcBox->itemsCount() - 1)->load(QStringLiteral("ANTIDOTE"), 95);
  Item* dstStack = m_pcBox->items.at(m_pcBox->itemsCount() - 1);
  const int fullCount = m_pcBox->itemsCount();

  // Bag Antidote x10 would overflow the (full) PC's Antidote stack and there is no
  // room for a 2nd row -> the transfer is REFUSED rather than clamping/losing.
  m_box->reset(); m_box->itemNew(); m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 10);
  Item* srcItem = m_box->items.at(0);

  m_bag->dragTransfer(0, 0, false);

  QCOMPARE(m_box->itemsCount(), 1);            // stayed in the bag
  QCOMPARE(m_box->items.at(0), srcItem);
  QCOMPARE(m_pcBox->itemsCount(), fullCount);  // dst unchanged
  QCOMPARE(dstStack->amount, 95);              // stack untouched (not topped to 99)
}

void TestItemStorageModel::amountOfInd_sumsAcrossRows()
{
  // amountOfInd sums every matching row (duplicate rows are supported).
  m_box->reset();
  m_box->itemNew(); m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 4);
  m_box->itemNew(); m_box->items.at(1)->load(QStringLiteral("POTION"),   9);
  m_box->itemNew(); m_box->items.at(2)->load(QStringLiteral("ANTIDOTE"), 6);
  const int antidote = m_box->items.at(0)->ind;
  const int potion = m_box->items.at(1)->ind;

  QCOMPARE(m_box->amountOfInd(antidote), 10);  // 4 + 6 across two rows
  QCOMPARE(m_box->amountOfInd(potion), 9);
  QCOMPARE(m_box->amountOfInd(-1), 0);         // absent -> 0
}

void TestItemStorageModel::itemOverview_aggregatesSortsHidesZeros()
{
  // Bag: Antidote x3, Potion x9.  Storage: Antidote x5 + Antidote x2 (a duplicate
  // row, which must be SUMMED to 7). Potion is bag-only (storage 0).
  m_box->reset();
  m_box->itemNew();   m_box->items.at(0)->load(QStringLiteral("ANTIDOTE"), 3);
  m_box->itemNew();   m_box->items.at(1)->load(QStringLiteral("POTION"),   9);
  m_pcBox->reset();
  m_pcBox->itemNew(); m_pcBox->items.at(0)->load(QStringLiteral("ANTIDOTE"), 5);
  m_pcBox->itemNew(); m_pcBox->items.at(1)->load(QStringLiteral("ANTIDOTE"), 2);

  auto ov = m_brg->itemOverviewModel;
  ov->rebuild();

  // Exactly two distinct items, alphabetical (Antidote before Potion).
  QCOMPARE(ov->rowCount(QModelIndex()), 2);

  const QModelIndex a = ov->index(0);
  const QModelIndex p = ov->index(1);

  QVERIFY(!ov->data(a, ItemOverviewModel::NameRole).toString().isEmpty());
  // Row 0 = Antidote: bag 3, storage 5+2 = 7 (duplicate rows summed).
  QCOMPARE(ov->data(a, ItemOverviewModel::BagCountRole).toInt(), 3);
  QCOMPARE(ov->data(a, ItemOverviewModel::StorageCountRole).toInt(), 7);
  // Row 1 = Potion: bag 9, storage 0 (bag-only -- the view hides the 0).
  QCOMPARE(ov->data(p, ItemOverviewModel::BagCountRole).toInt(), 9);
  QCOMPARE(ov->data(p, ItemOverviewModel::StorageCountRole).toInt(), 0);

  // Alphabetical order (collator, case-insensitive enough for A vs P).
  QVERIFY(ov->data(a, ItemOverviewModel::NameRole).toString().toLower()
          < ov->data(p, ItemOverviewModel::NameRole).toString().toLower());
}

void TestItemStorageModel::deleteItem_singleAndGroup()
{
  ensureItems(3);
  const int n = m_box->itemsCount();
  QVERIFY(n >= 3);
  Item* a = m_box->items.at(0);
  Item* b = m_box->items.at(1);

  // Single delete (unchecked): drop item at index 1.
  m_bag->deleteItem(1, false);
  QCOMPARE(m_box->itemsCount(), n - 1);
  QVERIFY2(!m_box->items.contains(b), "single-deleted item still present");

  // Group delete (checked): check A, delete the whole checked set (just A here).
  check(0, true);
  m_bag->deleteItem(0, true);
  QCOMPARE(m_box->itemsCount(), n - 2);
  QVERIFY2(!m_box->items.contains(a), "group-deleted item still present");
}

QTEST_GUILESS_MAIN(TestItemStorageModel)
#include "tst_item_storage_model.moc"
