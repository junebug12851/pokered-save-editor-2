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
 * @file tst_model_tester.cpp
 * @brief Model-CONTRACT coverage via Qt's own QAbstractItemModelTester. The other
 *        model tests assert specific data; this asserts that every QAbstractItemModel
 *        subclass obeys the framework contract -- index()/parent()/rowCount()/
 *        columnCount()/data()/roleNames() consistency, valid-vs-invalid index
 *        handling, and (the part hand-written tests usually miss) the *change-signal
 *        protocol*: beginResetModel/endResetModel, dataChanged, layoutChanged bracket
 *        their edits correctly. The tester attaches as an observer and fails the test
 *        on any violation, so it also runs while the dynamic models are mutated.
 *
 *        Covered: every standalone select/list model (freshly constructed) AND every
 *        live Bridge-wired model over the populated BaseSAV fixture, plus the dynamic
 *        models (Pokedex sort-reset, PC storage box switch, box selector) under an
 *        attached tester. Guiless / headless.
 */

#include <QtTest>
#include <QAbstractItemModelTester>
#include <QCoreApplication>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/typesmodel.h>
#include <mvc/natureselectmodel.h>
#include <mvc/speciesselectmodel.h>
#include <mvc/moveselectmodel.h>
#include <mvc/statusselectmodel.h>
#include <mvc/pokemonstartersmodel.h>
#include <mvc/creditsmodel.h>
#include <mvc/itemselectmodel.h>
#include <mvc/pokemonstoragemodel.h>
#include <mvc/pokemonboxselectmodel.h>

using namespace pse_test;

namespace {
// Attach the framework tester (QtTest reporting -> any contract violation fails the
// test), then sweep data() across all rows x all roles to drive the model under the
// tester's observation. The tester object's lifetime must span the exercise, so it's
// stack-allocated here and lives until the function returns.
void checkContract(QAbstractItemModel* m, const char* label)
{
  QVERIFY2(m != nullptr, label);
  QAbstractItemModelTester tester(m, QAbstractItemModelTester::FailureReportingMode::QtTest);

  const QHash<int, QByteArray> roles = m->roleNames();
  const int rows = m->rowCount(QModelIndex());
  QVERIFY2(rows >= 0, label);
  for(int r = 0; r < rows; r++) {
    const QModelIndex idx = m->index(r, 0);
    for(auto it = roles.constBegin(); it != roles.constEnd(); ++it)
      m->data(idx, it.key());
  }
}
} // namespace

class TestModelTester : public QObject
{
  Q_OBJECT

private:
  SaveFile m_sf;                  // blank save -> a live AreaMap for MapSelectModel
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;

private slots:
  void initTestCase();
  void cleanupTestCase();

  void standaloneModels_satisfyContract();
  void bridgeModels_satisfyContract();
  void dynamicModels_satisfyContractUnderChanges();
};

void TestModelTester::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));

  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();          // static screen registry (see tst_bridge)

  m_file = new FileManagement;
  QVERIFY(m_file->data != nullptr);

  // Real, populated fixture BEFORE the Bridge (it captures tree pointers at ctor).
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(bytes.size(), kSaveSize);
  loadInto(*m_file->data, bytes);
  QVERIFY(m_file->data->dataExpanded != nullptr);

  m_brg = new Bridge(m_file);     // ONE Bridge for the whole suite (avoids the
                                  // market-registry churn crash; see tst_bridge note)
}

void TestModelTester::cleanupTestCase()
{
  delete m_brg;  m_brg = nullptr;
  delete m_file; m_file = nullptr;
}

void TestModelTester::standaloneModels_satisfyContract()
{
  TypesModel types;                       checkContract(&types,    "TypesModel");
  NatureSelectModel nature;               checkContract(&nature,   "NatureSelectModel");
  SpeciesSelectModel species;             checkContract(&species,  "SpeciesSelectModel");
  MoveSelectModel moves;                  checkContract(&moves,    "MoveSelectModel");
  StatusSelectModel status;               checkContract(&status,   "StatusSelectModel");
  PokemonStartersModel starters;          checkContract(&starters, "PokemonStartersModel");
  CreditsModel credits;                   checkContract(&credits,  "CreditsModel");
  ItemSelectModel items;                  checkContract(&items,    "ItemSelectModel");
}

void TestModelTester::bridgeModels_satisfyContract()
{
  // The live, app-wired instances over real save data.
  checkContract(m_brg->pokedexModel,           "pokedexModel");
  checkContract(m_brg->bagItemsModel,          "bagItemsModel");
  checkContract(m_brg->pcItemsModel,           "pcItemsModel");
  checkContract(m_brg->marketModel,            "marketModel");
  checkContract(m_brg->pokemonStorageModel1,   "pokemonStorageModel1");
  checkContract(m_brg->pokemonStorageModel2,   "pokemonStorageModel2");
  checkContract(m_brg->pokemonBoxSelectModel1, "pokemonBoxSelectModel1");
  checkContract(m_brg->pokemonBoxSelectModel2, "pokemonBoxSelectModel2");
  checkContract(m_brg->recentFilesModel,       "recentFilesModel");
  checkContract(m_brg->creditsModel,           "creditsModel");
  checkContract(m_brg->starterModel,           "starterModel");
  checkContract(m_brg->itemSelectModel,        "itemSelectModel");
  checkContract(m_brg->typesModel,             "typesModel");
  checkContract(m_brg->speciesSelectModel,     "speciesSelectModel");
  checkContract(m_brg->statusSelectModel,      "statusSelectModel");
  checkContract(m_brg->natureSelectModel,      "natureSelectModel");
  checkContract(m_brg->moveSelectModel,        "moveSelectModel");
  // brg.map (MapModel) is not a list model -- it has no rows/roles contract to check.
  // It is covered by tst_map instead.
}

void TestModelTester::dynamicModels_satisfyContractUnderChanges()
{
  // Keep a tester ATTACHED across each mutation so its emitted change signals
  // (beginResetModel/endResetModel, dataChanged, layoutChanged) are validated for
  // protocol correctness -- the thing static row/role sweeps can't catch.

  {
    auto* pm = m_brg->pokedexModel;
    QAbstractItemModelTester t(pm, QAbstractItemModelTester::FailureReportingMode::QtTest);
    for(int i = 0; i < 5; i++)
      pm->dexSortCycle();        // each cycle re-sorts via beginResetModel/endResetModel
    pm->pageClosing();
    pm->dataChanged(0);          // per-entry dataChanged path
  }

  {
    auto* sm = m_brg->pokemonStorageModel1;
    QAbstractItemModelTester t(sm, QAbstractItemModelTester::FailureReportingMode::QtTest);
    sm->switchBox(0);            // box content swap -> the model's change protocol
    sm->switchBox(1);
    sm->switchBox(PokemonStorageModel::PartyBox);
    sm->switchBox(0);
  }

  {
    auto* bs = m_brg->pokemonBoxSelectModel1;
    QAbstractItemModelTester t(bs, QAbstractItemModelTester::FailureReportingMode::QtTest);
    bs->onBoxChange();
  }
}

QTEST_GUILESS_MAIN(TestModelTester)
#include "tst_model_tester.moc"
