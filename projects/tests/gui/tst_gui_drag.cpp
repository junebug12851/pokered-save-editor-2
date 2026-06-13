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
 * @file tst_gui_drag.cpp
 * @brief End-to-end drag/drop FLOWS through the live app: drive the exact model
 *        Q_INVOKABLEs the QML drag interaction calls (ItemStorageModel::dragReorder /
 *        dragTransfer / deleteItem) on the real bridge models, then save -> reopen and
 *        assert the result persisted to disk. The model mechanics themselves are
 *        unit-tested in tst_item_storage_model; this is the integration/persistence
 *        half -- a dragged/transferred/deleted item is written and reloads correctly.
 *
 * Items are used because the bag is reliably populated in BaseSAV (PC boxes may be
 * empty). The Pokemon drag paths (dragReorder/dragTransfer/deleteMon) share the same
 * shape and are unit-tested at the model level (tst_storage_model).
 */

#include <QtTest>
#include <QVector>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

#include <bridge/bridge.h>
#include <mvc/itemstoragemodel.h>
#include <mvc/pokemonstoragemodel.h>

using namespace pse_test;

class TestGuiDrag : public QObject
{
  Q_OBJECT

  QTemporaryDir m_tmp;

  static bool reopen(FileManagement& fm, const QString& path)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(path);
    return fm.openFileRecent(0);
  }

  // The (id, amount) pairs of a bag/PC box, in slot order.
  static QVector<QPair<int,int>> snapshotBox(ItemStorageBox* box)
  {
    QVector<QPair<int,int>> out;
    for (int i = 0; i < box->itemsCount(); ++i)
      out.append({ box->itemAt(i)->ind, box->itemAt(i)->getAmount() });
    return out;
  }

  // Species ids of a Pokemon box, in slot order.
  static QVector<int> pokeSpecies(PokemonStorageBox* box)
  {
    QVector<int> out;
    for (int i = 0; i < box->pokemonCount(); ++i)
      out.append(box->pokemonAt(i)->species);
    return out;
  }

  // Add @p species.size() fresh mons to @p box with the given distinct species ids,
  // appended after whatever is already there. Returns the index of the first added.
  static int addMons(PokemonStorageBox* box, const QVector<int>& species)
  {
    const int first = box->pokemonCount();
    for (int s : species) {
      box->pokemonNew();
      box->pokemonAt(box->pokemonCount() - 1)->species = s;
    }
    return first;
  }

private slots:
  void initTestCase();
  void itemReorder_persists();
  void itemTransferBagToPc_persists();
  void itemDelete_persists();
  void pokemonReorder_persists();
  void pokemonDelete_persists();
};

void TestGuiDrag::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());
  QCOMPARE(readSaveBytes(QStringLiteral("BaseSAV.sav")).size(), kSaveSize);
  GuiApp::installMessageHandler();
}

// Drag-reorder within the bag: the slot order changes (no item gained/lost) and the
// new order survives save -> reopen.
void TestGuiDrag::itemReorder_persists()
{
  GuiApp app(QStringLiteral("BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  ItemStorageBox* bag = app.file()->data->dataExpanded->player->items;
  if (bag->itemsCount() < 2)
    QSKIP("BaseSAV bag has < 2 items; reorder needs at least two.");

  const QVector<QPair<int,int>> before = snapshotBox(bag);

  // Move the first slot to the last (the same op the QML drag issues).
  QmlWarningScope scope;
  app.bridge()->bagItemsModel->dragReorder(0, bag->itemsCount() - 1, false);
  QVERIFY2(scope.clean(), qPrintable("dragReorder emitted QML warnings:\n" + scope.messages().join('\n')));

  const QVector<QPair<int,int>> after = snapshotBox(bag);
  QCOMPARE(after.size(), before.size());                 // nothing gained or lost
  QVERIFY2(after != before, "reorder did not change the slot order");

  // A reorder is a permutation -- the multiset of (id,amount) is preserved.
  QVector<QPair<int,int>> sb = before, sa = after;
  std::sort(sb.begin(), sb.end()); std::sort(sa.begin(), sa.end());
  QCOMPARE(sa, sb);

  // Persist + reopen: the reordered slots come back identical.
  const QString out = m_tmp.filePath(QStringLiteral("reorder.sav"));
  QVERIFY(app.saveTo(out));
  FileManagement fm; QVERIFY(reopen(fm, out));
  QCOMPARE(snapshotBox(fm.data->dataExpanded->player->items), after);
}

// Drag-transfer a bag item to the PC item box: it leaves the bag, lands in the PC,
// the owned total is conserved, and it survives save -> reopen.
void TestGuiDrag::itemTransferBagToPc_persists()
{
  GuiApp app(QStringLiteral("BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  auto* exp = app.file()->data->dataExpanded;
  ItemStorageBox* bag = exp->player->items;
  ItemStorageBox* pc  = exp->storage->items;
  if (bag->itemsCount() < 1)
    QSKIP("BaseSAV bag is empty; nothing to transfer.");

  const int movedId  = bag->itemAt(0)->ind;
  const int movedAmt = bag->itemAt(0)->getAmount();
  const int bagBefore = bag->itemsCount();
  const int ownedBefore = bag->amountOfInd(movedId) + pc->amountOfInd(movedId);

  QmlWarningScope scope;
  app.bridge()->bagItemsModel->dragTransfer(0, 0, false);   // bag[0] -> PC
  QVERIFY2(scope.clean(), qPrintable("dragTransfer emitted QML warnings:\n" + scope.messages().join('\n')));

  QVERIFY2(bag->itemsCount() == bagBefore - 1, "the item did not leave the bag");
  QVERIFY2(pc->amountOfInd(movedId) >= movedAmt, "the item did not arrive in the PC box");
  QCOMPARE(bag->amountOfInd(movedId) + pc->amountOfInd(movedId), ownedBefore); // conserved

  const QString out = m_tmp.filePath(QStringLiteral("transfer.sav"));
  QVERIFY(app.saveTo(out));
  FileManagement fm; QVERIFY(reopen(fm, out));
  auto* e2 = fm.data->dataExpanded;
  QCOMPARE(e2->player->items->itemsCount(), bagBefore - 1);
  QVERIFY2(e2->storage->items->amountOfInd(movedId) >= movedAmt, "transfer did not persist into the PC box");
}

// Delete a bag item: the slot is removed and stays gone after save -> reopen.
void TestGuiDrag::itemDelete_persists()
{
  GuiApp app(QStringLiteral("BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  ItemStorageBox* bag = app.file()->data->dataExpanded->player->items;
  if (bag->itemsCount() < 1)
    QSKIP("BaseSAV bag is empty; nothing to delete.");

  const int countBefore = bag->itemsCount();
  const QVector<QPair<int,int>> remainingExpected = snapshotBox(bag).mid(1); // all but slot 0

  QmlWarningScope scope;
  app.bridge()->bagItemsModel->deleteItem(0, false);
  QVERIFY2(scope.clean(), qPrintable("deleteItem emitted QML warnings:\n" + scope.messages().join('\n')));

  QCOMPARE(bag->itemsCount(), countBefore - 1);
  QCOMPARE(snapshotBox(bag), remainingExpected);

  const QString out = m_tmp.filePath(QStringLiteral("delete.sav"));
  QVERIFY(app.saveTo(out));
  FileManagement fm; QVERIFY(reopen(fm, out));
  QCOMPARE(fm.data->dataExpanded->player->items->itemsCount(), countBefore - 1);
  QCOMPARE(snapshotBox(fm.data->dataExpanded->player->items), remainingExpected);
}

// Drag-reorder mons within a PC box: the slot order changes (a permutation, nothing
// gained/lost) and survives save -> reopen. The box is populated at runtime (BaseSAV's
// PC boxes may be empty) and marked formatted so it persists.
void TestGuiDrag::pokemonReorder_persists()
{
  GuiApp app(QStringLiteral("BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  auto* exp = app.file()->data->dataExpanded;
  exp->storage->boxesFormatted = true;                 // ensure boxes are written
  PokemonStorageBox* box = exp->storage->boxAt(0);
  addMons(box, { 1, 4, 7 });                           // distinct, identifiable mons
  QVERIFY(box->pokemonCount() >= 2);

  PokemonStorageModel* model = app.bridge()->pokemonStorageModel1;
  model->switchBox(0);

  const QVector<int> before = pokeSpecies(box);
  QmlWarningScope scope;
  model->dragReorder(0, box->pokemonCount() - 1, false);   // first slot -> last
  QVERIFY2(scope.clean(), qPrintable("dragReorder emitted QML warnings:\n" + scope.messages().join('\n')));

  const QVector<int> after = pokeSpecies(box);
  QCOMPARE(after.size(), before.size());
  QVERIFY2(after != before, "reorder did not change the slot order");
  QVector<int> sb = before, sa = after;
  std::sort(sb.begin(), sb.end()); std::sort(sa.begin(), sa.end());
  QCOMPARE(sa, sb);                                     // a permutation -- no mon lost

  const QString out = m_tmp.filePath(QStringLiteral("poke_reorder.sav"));
  QVERIFY(app.saveTo(out));
  FileManagement fm; QVERIFY(reopen(fm, out));
  QCOMPARE(pokeSpecies(fm.data->dataExpanded->storage->boxAt(0)), after);
}

// Delete a mon from a PC box: it is removed and stays gone after save -> reopen.
void TestGuiDrag::pokemonDelete_persists()
{
  GuiApp app(QStringLiteral("BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  auto* exp = app.file()->data->dataExpanded;
  exp->storage->boxesFormatted = true;
  PokemonStorageBox* box = exp->storage->boxAt(0);
  addMons(box, { 1, 4, 7 });
  QVERIFY(box->pokemonCount() >= 1);

  PokemonStorageModel* model = app.bridge()->pokemonStorageModel1;
  model->switchBox(0);

  const QVector<int> before = pokeSpecies(box);
  const QVector<int> expected = before.mid(1);         // all but slot 0

  QmlWarningScope scope;
  model->deleteMon(0, false);
  QVERIFY2(scope.clean(), qPrintable("deleteMon emitted QML warnings:\n" + scope.messages().join('\n')));

  QCOMPARE(box->pokemonCount(), before.size() - 1);
  QCOMPARE(pokeSpecies(box), expected);

  const QString out = m_tmp.filePath(QStringLiteral("poke_delete.sav"));
  QVERIFY(app.saveTo(out));
  FileManagement fm; QVERIFY(reopen(fm, out));
  QCOMPARE(pokeSpecies(fm.data->dataExpanded->storage->boxAt(0)), expected);
}

QTEST_MAIN(TestGuiDrag)
#include "tst_gui_drag.moc"
