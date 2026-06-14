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
 * @file tst_app_e2e.cpp
 * @brief Full-stack cross-layer end-to-end: edits made THROUGH the app layer
 *        (Bridge + Qt item models) are flushed by FileManagement::saveFile() to a
 *        real file on disk, reopened in a fresh FileManagement, and verified. This
 *        is the app <-> savefile <-> db <-> disk path that the savefile-only e2e
 *        (tst_e2e) doesn't cover. Also a disk write-and-compare: the bytes written
 *        equal the in-memory flatten+recalc buffer exactly (I/O fidelity).
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemstoragemodel.h>

using namespace pse_test;

class TestAppE2E : public QObject
{
  Q_OBJECT

private:
  QTemporaryDir m_tmp;

  void loadGood(FileManagement& fm)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(assetPath(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
    QVERIFY(fm.openFileRecent(0));
  }

private slots:
  void initTestCase();
  void appModelEdits_persistToDiskAndReopen();
  void diskWrite_matchesInMemoryFlatten();
};

void TestAppE2E::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
  QVERIFY(m_tmp.isValid());
}

// Edit through the Bridge's models, save to disk, reopen, verify the app-layer
// edits round-tripped all the way to the file and back.
void TestAppE2E::appModelEdits_persistToDiskAndReopen()
{
  FileManagement fm;
  loadGood(fm);                  // load BEFORE building the Bridge (it captures the tree)
  Bridge brg(&fm);

  // (1) a scalar edit on the live tree the app binds to
  fm.data->dataExpanded->player->basics->money = 271828;

  // (2) an edit driven through an actual Qt item model (the app layer)
  ItemStorageModel* bag = brg.bagItemsModel;
  QVERIFY(fm.data->dataExpanded->player->items->itemsCount() > 0);
  QVERIFY(bag->setData(bag->index(0), QVariant(20), ItemStorageModel::IdRole));   // ITEM index
  QVERIFY(bag->setData(bag->index(0), QVariant(9),  ItemStorageModel::CountRole)); // amount

  // Save to a NEW file (never the fixture) and reopen fresh.
  const QString out = m_tmp.filePath(QStringLiteral("app_edited.sav"));
  fm.setPath(out);
  QVERIFY(fm.saveFile());

  FileManagement fm2;
  fm2.clearRecentFiles();
  fm2.addRecentFile(out);
  QVERIFY(fm2.openFileRecent(0));

  QCOMPARE(fm2.data->dataExpanded->player->basics->money, 271828u);
  Item* it = fm2.data->dataExpanded->player->items->itemAt(0);
  QVERIFY(it != nullptr);
  QCOMPARE(it->ind, 20);
  QCOMPARE(it->getAmount(), 9);
}

// The bytes saveFile() writes to disk equal the in-memory flatten+recalc buffer
// exactly -- proves the write/read I/O layer is byte-faithful (no truncation, pad,
// or reordering).
void TestAppE2E::diskWrite_matchesInMemoryFlatten()
{
  FileManagement fm;
  loadGood(fm);

  // Compute the expected on-disk image in memory.
  fm.data->flattenData();
  fm.data->toolset->recalcChecksums();
  const QByteArray expected = snapshot(*fm.data);

  const QString out = m_tmp.filePath(QStringLiteral("identity.sav"));
  fm.setPath(out);
  QVERIFY(fm.saveFile());

  QFile f(out);
  QVERIFY(f.open(QIODevice::ReadOnly));
  const QByteArray disk = f.read(kSaveSize + 16);
  f.close();

  QCOMPARE(disk.size(), kSaveSize);
  QCOMPARE(disk, expected);   // byte-for-byte
}

QTEST_GUILESS_MAIN(TestAppE2E)
#include "tst_app_e2e.moc"
