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
 * @file tst_items.cpp
 * @brief Phase-2 coverage of item LISTS (the bag and the PC item box): building a
 *        known list, flattening, re-expanding, and asserting count + each slot's
 *        index/amount survive in order; the empty-list case; and add/remove counts.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

using namespace pse_test;

class TestItems : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  // Build a known list of n items, round-trip, and verify order/values survive.
  static void roundTripList(SaveFile& sf, ItemStorageBox* box,
                            const QVector<int>& ids, const QVector<int>& amts)
  {
    box->reset();
    for(int i = 0; i < ids.size(); i++) {
      box->itemNew();
      Item* it = box->itemAt(i);
      QVERIFY(it != nullptr);
      it->ind = ids[i];
      it->setAmount(amts[i]);
    }
    QCOMPARE(box->itemsCount(), ids.size());

    sf.flattenData();
    sf.expandData();
  }

private slots:
  void initTestCase();

  void bagList_roundTrip();
  void pcItemList_roundTrip();
  void emptyBag_roundTrip();
  void addRemove_counts();
};

void TestItems::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestItems::bagList_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  const QVector<int> ids  = {1, 2, 4, 8, 16};
  const QVector<int> amts = {1, 50, 99, 10, 42}; // amount cap is 99
  roundTripList(sf, sf.dataExpanded->player->items, ids, amts);

  ItemStorageBox* bag = sf.dataExpanded->player->items;
  QCOMPARE(bag->itemsCount(), ids.size());
  for(int i = 0; i < ids.size(); i++) {
    Item* it = bag->itemAt(i);
    QVERIFY(it != nullptr);
    QCOMPARE(it->ind, ids[i]);
    QCOMPARE(it->getAmount(), amts[i]);
  }
}

void TestItems::pcItemList_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  const QVector<int> ids  = {5, 10, 15};
  const QVector<int> amts = {99, 1, 77};
  roundTripList(sf, sf.dataExpanded->storage->items, ids, amts);

  ItemStorageBox* pc = sf.dataExpanded->storage->items;
  QCOMPARE(pc->itemsCount(), ids.size());
  for(int i = 0; i < ids.size(); i++) {
    Item* it = pc->itemAt(i);
    QVERIFY(it != nullptr);
    QCOMPARE(it->ind, ids[i]);
    QCOMPARE(it->getAmount(), amts[i]);
  }
}

void TestItems::emptyBag_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->player->items->reset();
  QCOMPARE(sf.dataExpanded->player->items->itemsCount(), 0);

  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->items->itemsCount(), 0);
}

void TestItems::addRemove_counts()
{
  SaveFile sf; loadInto(sf, m_orig);
  ItemStorageBox* bag = sf.dataExpanded->player->items;

  bag->reset();
  bag->itemNew();
  bag->itemNew();
  bag->itemNew();
  QCOMPARE(bag->itemsCount(), 3);

  bag->itemRemove(1);
  QCOMPARE(bag->itemsCount(), 2);
}

QTEST_GUILESS_MAIN(TestItems)
#include "tst_items.moc"
