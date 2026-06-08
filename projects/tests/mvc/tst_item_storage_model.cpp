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
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemstoragemodel.h>

using namespace pse_test;

class TestItemStorageModel : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;
  ItemStorageModel* m_bag = nullptr;   // brg.bagItemsModel
  ItemStorageBox* m_box = nullptr;     // the underlying bag box

  void ensureItems(int n)
  {
    while(m_box->itemsCount() < n && m_box->itemsCount() < m_box->itemsMax())
      m_box->itemNew();
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

QTEST_GUILESS_MAIN(TestItemStorageModel)
#include "tst_item_storage_model.moc"
