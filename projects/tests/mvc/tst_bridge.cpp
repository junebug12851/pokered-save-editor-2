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
 * @file tst_bridge.cpp
 * @brief Integration coverage of the APP layer's hub. Constructing a real
 *        @ref Bridge over a blank @ref FileManagement wires up every Qt item
 *        model the way the running app does -- so this single fixture exercises
 *        the models that need heavy construction (Pokedex, item storage, the
 *        Poke-mart market, both PC storage halves and their box selectors,
 *        recent-files, the font finder) plus @ref Router navigation. Each list
 *        model is swept across all rows x all roles to drive its data() paths
 *        and prove no row/role combination crashes.
 */

#include <QtTest>
#include <QAbstractListModel>
#include <QCoreApplication>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>

#include <QColor>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <bridge/settings.h>
#include <mvc/pokemonstoragemodel.h>
#include <mvc/pokemonboxselectmodel.h>

using namespace pse_test;

namespace {
// Sweep a list model across (capped) rows x all roles, driving data() and proving
// no combination crashes; also confirms out-of-range access stays safe.
void exerciseListModel(QAbstractListModel* m, const char* label)
{
  QVERIFY2(m != nullptr, label);
  const QHash<int, QByteArray> roles = m->roleNames();
  QVERIFY2(!roles.isEmpty(), label);

  const int rows = m->rowCount(QModelIndex());
  QVERIFY2(rows >= 0, label);

  const int cap = qMin(rows, 64);
  for(int r = 0; r < cap; r++) {
    const QModelIndex idx = m->index(r, 0);
    QVERIFY2(idx.isValid(), label);
    for(auto it = roles.constBegin(); it != roles.constEnd(); ++it)
      m->data(idx, it.key()); // value may be invalid for placeholder rows; we just drive the path
  }

  // Out-of-range row + an invalid index must both return cleanly (no crash).
  const int probeRole = roles.constBegin().key();
  m->data(m->index(rows + 8, 0), probeRole);
  m->data(QModelIndex(), probeRole);
}
} // namespace

class TestBridge : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;

private slots:
  void initTestCase();
  void cleanupTestCase();

  void bridge_constructsEveryModel();
  void listModels_sweepRowsAndRoles();
  void pokemonStorage_boxOperations();
  void boxSelect_rowsAndChange();
  void pokedexModel_sortsAndLookups();
  void router_navigateAndClose();
  void settings_colorSchemeAndPreviewIndex();
};

void TestBridge::initTestCase()
{
  // Keep RecentFilesModel/Settings off the real user's QSettings store.
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));

  QVERIFY(DB::inst() != nullptr);

  // Router's screen registry is static and is normally populated at app boot
  // (boot.cpp), not by the constructor -- do the same here before navigating.
  Router::loadScreens();

  m_file = new FileManagement;          // blank save, fully expanded tree
  QVERIFY(m_file->data != nullptr);

  // Load the real default fixture so the models have actual data (a party, a
  // pokedex with seen/owned entries, bag items). Must happen BEFORE the Bridge
  // is built: the Bridge captures pointers into the expanded tree at construction.
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(bytes.size(), kSaveSize);
  loadInto(*m_file->data, bytes); // setData + re-expand, in place
  QVERIFY(m_file->data->dataExpanded != nullptr);

  m_brg = new Bridge(m_file);           // wires every model exactly as the app does
}

void TestBridge::cleanupTestCase()
{
  delete m_brg; m_brg = nullptr;
  delete m_file; m_file = nullptr;
}

void TestBridge::bridge_constructsEveryModel()
{
  QVERIFY(m_brg->router != nullptr);
  QVERIFY(m_brg->settings != nullptr);
  QVERIFY(m_brg->keyboard != nullptr);
  QVERIFY(m_brg->recentFilesModel != nullptr);
  QVERIFY(m_brg->pokedexModel != nullptr);
  QVERIFY(m_brg->bagItemsModel != nullptr);
  QVERIFY(m_brg->pcItemsModel != nullptr);
  QVERIFY(m_brg->marketModel != nullptr);
  QVERIFY(m_brg->pokemonStorageModel1 != nullptr);
  QVERIFY(m_brg->pokemonStorageModel2 != nullptr);
  QVERIFY(m_brg->pokemonBoxSelectModel1 != nullptr);
  QVERIFY(m_brg->pokemonBoxSelectModel2 != nullptr);
  QVERIFY(m_brg->creditsModel != nullptr);
  QVERIFY(m_brg->starterModel != nullptr);
  QVERIFY(m_brg->itemSelectModel != nullptr);
  QVERIFY(m_brg->typesModel != nullptr);
  QVERIFY(m_brg->speciesSelectModel != nullptr);
  QVERIFY(m_brg->statusSelectModel != nullptr);
  QVERIFY(m_brg->natureSelectModel != nullptr);
  QVERIFY(m_brg->moveSelectModel != nullptr);
  QVERIFY(m_brg->map != nullptr);

  // The two PC storage halves are cross-linked for transfers.
  QCOMPARE(m_brg->pokemonStorageModel1->otherModel, m_brg->pokemonStorageModel2);
  QCOMPARE(m_brg->pokemonStorageModel2->otherModel, m_brg->pokemonStorageModel1);
}

void TestBridge::listModels_sweepRowsAndRoles()
{
  exerciseListModel(m_brg->pokedexModel,          "pokedexModel");
  exerciseListModel(m_brg->bagItemsModel,         "bagItemsModel");
  exerciseListModel(m_brg->pcItemsModel,          "pcItemsModel");
  exerciseListModel(m_brg->marketModel,           "marketModel");
  exerciseListModel(m_brg->pokemonStorageModel1,  "pokemonStorageModel1");
  exerciseListModel(m_brg->pokemonStorageModel2,  "pokemonStorageModel2");
  exerciseListModel(m_brg->pokemonBoxSelectModel1,"pokemonBoxSelectModel1");
  exerciseListModel(m_brg->pokemonBoxSelectModel2,"pokemonBoxSelectModel2");
  exerciseListModel(m_brg->recentFilesModel,      "recentFilesModel");
  exerciseListModel(m_brg->creditsModel,          "creditsModel");
  exerciseListModel(m_brg->itemSelectModel,       "itemSelectModel");
}

void TestBridge::pokemonStorage_boxOperations()
{
  auto* sm = m_brg->pokemonStorageModel1;

  // Show a real PC box (0..N): must yield a valid box object. getBoxMon() does an
  // unchecked .at() (its contract: QML only calls it for rendered rows), so only
  // probe it when the box actually holds a mon.
  sm->switchBox(0);
  QCOMPARE(sm->curBox, 0);
  QVERIFY(sm->getBox(0) != nullptr);
  auto* box0 = sm->getCurBox();
  QVERIFY(box0 != nullptr);
  if(!box0->pokemon.isEmpty())
    QVERIFY(sm->getBoxMon(0) != nullptr);

  // The party (-1): BaseSAV ships with a party, so getPartyMon(0) is valid.
  sm->switchBox(PokemonStorageModel::PartyBox);
  QCOMPARE(sm->curBox, static_cast<int>(PokemonStorageModel::PartyBox));
  auto* party = sm->getCurBox();
  QVERIFY(party != nullptr);
  QVERIFY2(!party->pokemon.isEmpty(), "BaseSAV party unexpectedly empty");
  sm->getPartyMon(0); // valid index -> exercises the typed-return path

  sm->switchBox(0); // leave it on a deterministic box
}

void TestBridge::boxSelect_rowsAndChange()
{
  auto* bs = m_brg->pokemonBoxSelectModel1;
  QVERIFY2(bs->rowCount(QModelIndex()) > 0, "box selector is empty");
  bs->onBoxChange(); // reaction hook must be safe to call directly
}

void TestBridge::pokedexModel_sortsAndLookups()
{
  auto* pm = m_brg->pokedexModel;

  // Cycle through every sort mode (Dex -> Name -> Internal -> wrap); each cycle
  // re-runs dexSort() and so drives all three sort comparators.
  for(int i = 0; i < 5; i++)
    pm->dexSortCycle();

  // pageClosing() forces dex order back when it isn't already (the reset path),
  // then a second call hits the already-in-dex-order early return.
  pm->pageClosing();
  pm->pageClosing();

  // dexToListIndex: a real dex number resolves to a row; an absent one -> -1.
  QVERIFY(pm->dexToListIndex(0) >= 0);     // Bulbasaur (0-indexed dex)
  QCOMPARE(pm->dexToListIndex(99999), -1);

  pm->dataChanged(0);                       // per-entry change-signal path
}

void TestBridge::router_navigateAndClose()
{
  auto* r = m_brg->router;

  // Navigate to a registered non-modal screen, then back. closeScreen() guards
  // against popping the root, so this round-trips without underflowing the stack.
  r->changeScreen(QStringLiteral("pokedex"));
  QCOMPARE(r->title, QStringLiteral("Pokédex"));
  r->changeScreen(QStringLiteral("bag"));
  QCOMPARE(r->title, QStringLiteral("Items"));
  r->closeScreen();
  r->closeScreen();
  // Extra close at/under root must be a no-op (no crash).
  r->closeScreen();
}

void TestBridge::settings_colorSchemeAndPreviewIndex()
{
  Settings* s = m_brg->settings;
  QVERIFY(s != nullptr);

  // setColorScheme assigns the primary + derives lighter/darker shades, and sets accent.
  s->setColorScheme(QColor(10, 20, 30), QColor(40, 50, 60));
  QCOMPARE(s->primaryColor, QColor(10, 20, 30));
  QCOMPARE(s->accentColor, QColor(40, 50, 60));
  QVERIFY(s->primaryColorLight != s->primaryColor); // derived shade
  QVERIFY(s->primaryColorDark  != s->primaryColor);

  // The default preview tileset resolves to a valid order index; an unknown one -> -1.
  const int idx = s->getPreviewTilesetIndex();
  QVERIFY(idx >= 0 && idx < 24);
  s->previewTileset = QStringLiteral("___not-a-tileset___");
  QCOMPARE(s->getPreviewTilesetIndex(), -1);
}

QTEST_GUILESS_MAIN(TestBridge)
#include "tst_bridge.moc"
