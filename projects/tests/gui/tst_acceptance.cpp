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
 * @file tst_acceptance.cpp
 * @brief Acceptance / BDD-style scenarios -- product behavior stated as
 *        Given / When / Then, run end-to-end over the REAL app shell (GuiApp).
 *
 * No third-party BDD framework (Squish/Cucumber) is available, so the scenarios are
 * expressed with lightweight GIVEN/WHEN/THEN markers (they log the step, so a failing
 * run reads as a readable scenario transcript) on top of QtTest + the GuiApp harness.
 * These restate the two promises that matter most to a user, at product level:
 *   1. an edit survives a save/close/reopen round-trip;
 *   2. merely *looking* at the save (browsing every screen) never changes a byte
 *      -- the "Save File Integrity Is Sacred" guarantee, as an acceptance test.
 *
 * Headless / offscreen (set per-test in CMake).
 */

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QStringList>

#include "../helpers/guiapp.h"
#include "../helpers/savefilefixture.h"

#include <bridge/router.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

using namespace pse_test;

// Readable scenario markers (also serve as a transcript in the test log).
#define GIVEN(desc) qInfo().noquote() << "  GIVEN" << (desc)
#define WHEN(desc)  qInfo().noquote() << "  WHEN " << (desc)
#define THEN(desc)  qInfo().noquote() << "  THEN " << (desc)
#define ANDD(desc)  qInfo().noquote() << "  AND  " << (desc)

class TestAcceptance : public QObject
{
  Q_OBJECT

private slots:
  void scenario_editedMoneySurvivesSaveCloseReopen();
  void scenario_browsingEveryScreenChangesNoSaveBytes();
};

// Scenario 1 -----------------------------------------------------------------
void TestAcceptance::scenario_editedMoneySurvivesSaveCloseReopen()
{
  GIVEN("a populated save opened in the app");
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY2(app.start(), "App.qml shell failed to load");
  PlayerBasics* basics = app.file()->data->dataExpanded->player->basics;
  QVERIFY(basics != nullptr);

  WHEN("the trainer's money is set to 123456 and the save is written to a new file");
  const int kMoney = 123456;
  basics->money = kMoney;
  basics->moneyChanged();

  QTemporaryDir tmp;
  QVERIFY(tmp.isValid());
  const QString path = tmp.filePath(QStringLiteral("accept_money.sav"));
  QVERIFY2(app.saveTo(path), "saveFile() failed");

  THEN("re-opening that file from disk shows the new money");
  QFile f(path);
  QVERIFY(f.open(QIODevice::ReadOnly));
  const QByteArray bytes = f.readAll();
  f.close();
  QCOMPARE(bytes.size(), kSaveSize);

  SaveFile reopened;
  loadInto(reopened, bytes);            // fresh expansion of the written bytes
  QVERIFY(reopened.dataExpanded != nullptr);
  QCOMPARE(reopened.dataExpanded->player->basics->money, kMoney);
}

// Scenario 2 -----------------------------------------------------------------
void TestAcceptance::scenario_browsingEveryScreenChangesNoSaveBytes()
{
  GIVEN("a populated save opened in the app");
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY2(app.start(), "App.qml shell failed to load");

  ANDD("a baseline of the exact bytes the save would write right now");
  const QByteArray before = app.flattenedImage();
  QCOMPARE(before.size(), kSaveSize);

  WHEN("every home-reachable screen is visited (no edits made)");
  int visited = 0;
  const QList<QString> names = Router::screens.keys();
  for (const QString& name : names) {
    Screen* s = Router::screens.value(name, nullptr);
    if (!s || s->url.isEmpty() || s->modal || !s->homeBtn)
      continue;
    app.navigate(name);
    app.settle(40);
    ++visited;
  }
  QVERIFY2(visited > 0, "no screens were visited -- the registry/filter is wrong");

  THEN("the bytes the save would write are byte-for-byte identical (nothing was mutated)");
  const QByteArray after = app.flattenedImage();
  if (before != after) {
    // Report the first differing offset to make a regression actionable.
    int off = -1;
    for (int i = 0; i < before.size() && i < after.size(); ++i)
      if (before[i] != after[i]) { off = i; break; }
    QFAIL(qPrintable(QStringLiteral("browsing mutated the save at offset 0x%1")
                       .arg(off, 0, 16)));
  }
}

QTEST_MAIN(TestAcceptance)
#include "tst_acceptance.moc"
