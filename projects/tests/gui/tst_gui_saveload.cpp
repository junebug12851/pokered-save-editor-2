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
 * @file tst_gui_saveload.cpp
 * @brief Comprehensive GUI save/load journeys across multiple files. Boots the real
 *        app shell, navigates THROUGH the screens, edits via the same bound objects
 *        / item models the QML controls drive, then Save-As to a temp file, reopens
 *        in a fresh session, and asserts every edit persisted -- plus cross-file
 *        independence, a randomize-and-reopen journey, and byte-stability of an
 *        app-saved file. This is the "save/load across files", "per-screen edits"
 *        and "randomize + verify" coverage from the comprehensive GUI plan.
 *
 * Editing path note: these drive the EXACT C++ entry points the QML controls use
 * (the trainer-card fields write basics.money / basics.playerName / basics.badgeSet;
 * the bag uses ItemStorageModel.setData with the real roles; the Pokemon screen edits
 * a PokemonBox's bound fields) -- with the screens instantiated and live, so a binding
 * regression would surface as a QML warning too (captured). Synthesized keyboard/mouse
 * input is exercised separately in tst_gui_input.
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemstoragemodel.h>

using namespace pse_test;

class TestGuiSaveLoad : public QObject
{
  Q_OBJECT

  QTemporaryDir m_tmp;

  // Open a saved file fresh (no dialogs), the recent-file path the app uses.
  static bool reopen(FileManagement& fm, const QString& path)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(path);
    return fm.openFileRecent(0);
  }

private slots:
  void initTestCase();
  void editAcrossScreens_savesAndReopens();
  void multipleFiles_independent_includingRandomize();
  void appSavedFile_roundTripsByteStable();
};

void TestGuiSaveLoad::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());
  QCOMPARE(readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")).size(), kSaveSize);
  GuiApp::installMessageHandler();
}

// Edit trainer card + bag + a party Pokemon THROUGH the live screens, save, reopen,
// and verify every edit survived the full app -> disk -> app round-trip.
void TestGuiSaveLoad::editAcrossScreens_savesAndReopens()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();   // dismiss the startup New File modal

  Bridge* brg  = app.bridge();
  auto*   exp  = app.file()->data->dataExpanded;
  auto*   bas  = exp->player->basics;

  // --- Trainer Card screen ---
  {
    QmlWarningScope scope;
    app.navigate(QStringLiteral("trainerCard"));
    bas->money = 271828;                                   // money field (QML binds basics.money)
    bas->setProperty("playerName", QStringLiteral("ASH")); // name field (WRITE fullSetPlayerName)
    bas->badgeSet(0, true);                                // Boulder
    bas->badgeSet(2, true);                                // Thunder
    QVERIFY2(scope.clean(), qPrintable("trainerCard edits emitted QML warnings:\n" + scope.messages().join('\n')));
  }

  // --- Bag screen (edit through the real item model) ---
  {
    QmlWarningScope scope;
    app.navigate(QStringLiteral("bag"));
    QVERIFY(exp->player->items->itemsCount() > 0);
    ItemStorageModel* bag = brg->bagItemsModel;
    QVERIFY(bag->setData(bag->index(0), QVariant(20), ItemStorageModel::IdRole));
    QVERIFY(bag->setData(bag->index(0), QVariant(9),  ItemStorageModel::CountRole));
    QVERIFY2(scope.clean(), qPrintable("bag edits emitted QML warnings:\n" + scope.messages().join('\n')));
  }

  // --- Pokemon screen (edit a party mon's bound fields) ---
  bool editedMon = false;
  {
    QmlWarningScope scope;
    app.navigate(QStringLiteral("pokemon"));
    PlayerPokemon* party = exp->player->pokemon;
    if (party->pokemonCount() > 0) {
      PokemonParty* p = party->partyAt(0);
      QVERIFY(p != nullptr);
      p->level = 77;
      p->nickname = QStringLiteral("GUITEST");
      editedMon = true;
    }
    QVERIFY2(scope.clean(), qPrintable("pokemon edits emitted QML warnings:\n" + scope.messages().join('\n')));
  }

  // --- Save As to a temp file, reopen fresh ---
  const QString out = m_tmp.filePath(QStringLiteral("journey_A.sav"));
  QVERIFY(app.saveTo(out));

  FileManagement fm2;
  QVERIFY2(reopen(fm2, out), "the app-saved file failed to reopen");
  auto* e2 = fm2.data->dataExpanded;

  QCOMPARE(e2->player->basics->money, 271828u);
  QCOMPARE(e2->player->basics->property("playerName").toString(), QStringLiteral("ASH"));
  QVERIFY(e2->player->basics->badgeAt(0));
  QVERIFY(e2->player->basics->badgeAt(2));

  Item* it = e2->player->items->itemAt(0);
  QVERIFY(it != nullptr);
  QCOMPARE(it->ind, 20);
  QCOMPARE(it->getAmount(), 9);

  if (editedMon) {
    PokemonParty* p2 = e2->player->pokemon->partyAt(0);
    QVERIFY(p2 != nullptr);
    QCOMPARE(p2->level, 77);
    QCOMPARE(p2->nickname, QStringLiteral("GUITEST"));
  }
}

// Two files edited in separate app sessions stay independent; a randomized New File
// saves and reopens as a valid save without disturbing the other file.
void TestGuiSaveLoad::multipleFiles_independent_includingRandomize()
{
  const QString pathA = m_tmp.filePath(QStringLiteral("indep_A.sav"));
  const QString pathB = m_tmp.filePath(QStringLiteral("indep_B.sav"));

  // File A: a known, distinctive money value.
  {
    GuiApp a(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
    QVERIFY(a.start());
    a.closeTop();
    a.navigate(QStringLiteral("trainerCard"));
    a.file()->data->dataExpanded->player->basics->money = 111111;
    QVERIFY(a.saveTo(pathA));
  }

  // File B: a fresh New File, fully randomized through the app (the Ctrl+Shift+R path).
  {
    GuiApp b(QStringLiteral("new"));
    QVERIFY(b.start());
    b.closeTop();
    QmlWarningScope scope;
    b.file()->data->randomizeExpansion();   // same call the Random shortcut triggers
    QVERIFY2(scope.clean(), qPrintable("randomize emitted QML warnings:\n" + scope.messages().join('\n')));
    QVERIFY(b.saveTo(pathB));
  }

  // Reopen both fresh: A is untouched by B's work; B is a valid, loadable save.
  FileManagement fa;
  QVERIFY(reopen(fa, pathA));
  QCOMPARE(fa.data->dataExpanded->player->basics->money, 111111u);

  FileManagement fb;
  QVERIFY2(reopen(fb, pathB), "the randomized save failed to reopen (would be a corrupt/invalid save)");
  // Sanity: a randomized save is structurally valid -- money within the randomizer's range.
  QVERIFY(fb.data->dataExpanded->player->basics->money <= 999999u);

  // Independence: reopening B did not mutate A on disk.
  FileManagement fa2;
  QVERIFY(reopen(fa2, pathA));
  QCOMPARE(fa2.data->dataExpanded->player->basics->money, 111111u);
}

// A file produced by the app reopens and re-saves BYTE-IDENTICALLY (no drift):
// open -> flatten -> recalc == the bytes already on disk. Guards the sacred
// byte-fidelity contract at the app/disk boundary.
void TestGuiSaveLoad::appSavedFile_roundTripsByteStable()
{
  const QString out = m_tmp.filePath(QStringLiteral("stable.sav"));
  {
    GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
    QVERIFY(app.start());
    app.closeTop();
    app.navigate(QStringLiteral("trainerCard"));
    app.file()->data->dataExpanded->player->basics->money = 424242;
    QVERIFY(app.saveTo(out));
  }

  // Read the app-written bytes.
  QFile f(out);
  QVERIFY(f.open(QIODevice::ReadOnly));
  const QByteArray onDisk = f.read(kSaveSize + 16);
  f.close();
  QCOMPARE(onDisk.size(), kSaveSize);

  // Reopen and recompute the on-disk image: it must equal what's already there.
  FileManagement fm;
  QVERIFY(reopen(fm, out));
  fm.data->flattenData();
  fm.data->toolset->recalcChecksums();
  const QByteArray reflattened = snapshot(*fm.data);

  QCOMPARE(reflattened, onDisk);   // byte-for-byte stable
}

QTEST_MAIN(TestGuiSaveLoad)
#include "tst_gui_saveload.moc"
