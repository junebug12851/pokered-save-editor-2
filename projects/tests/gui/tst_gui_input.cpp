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
 * @file tst_gui_input.cpp
 * @brief Flagship REAL synthesized-input GUI test -- the "as a user would" half of
 *        the hybrid approach. Boots the live app, navigates to the Trainer Card, and
 *        drives the money field with actual key events (select-all, type, commit),
 *        then asserts the edit reached the model AND persisted through a save/reopen.
 *
 * This proves the synthesized-input path end to end and is the pattern to extend to
 * other controls (badge clicks, footer buttons, the name popup keyboard, the Pokemon
 * editor) -- see notes/plans/testing.md -> "Broader GUI coverage".
 *
 * Locator robustness: the money field is found by matching the TextField whose text
 * currently shows the player's money (no objectName needed). If the field can't be
 * located (e.g. the trainer-card layout changed, or money uses a non-TextField
 * control), the test SKIPs with an actionable message rather than producing a false
 * red -- this is the one input locator that is genuinely UI-shape dependent. Once
 * validated on the kit, tighten the QSKIP to a hard QVERIFY (or add an objectName).
 */

#include <QtTest>
#include <QTemporaryDir>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

#include <bridge/router.h>

using namespace pse_test;

class TestGuiInput : public QObject
{
  Q_OBJECT

  QTemporaryDir m_tmp;

private slots:
  void initTestCase();
  void typeMoney_commits_andPersists();
};

void TestGuiInput::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());
  QCOMPARE(readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")).size(), kSaveSize);
  GuiApp::installMessageHandler();
}

// Real keystrokes into the money field commit to the model and survive save/reopen.
void TestGuiInput::typeMoney_commits_andPersists()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();                       // dismiss the startup New File modal

  QmlWarningScope scope;
  app.navigate(QStringLiteral("trainerCard"));
  QVERIFY2(scope.clean(), qPrintable("trainerCard load emitted QML warnings:\n" + scope.messages().join('\n')));

  PlayerBasics* bas = app.file()->data->dataExpanded->player->basics;
  const QString moneyText = QString::number(bas->money);

  QQuickItem* screen = app.currentNonModal();
  QVERIFY(screen != nullptr);

  // Locate the money field by its objectName (the durable test hook on MoneyEdit's
  // DefTextEdit); fall back to a value-match on any editable text item in case the
  // hook is ever removed. A genuine miss is a hard failure now, not a skip.
  QQuickItem* moneyField = app.itemByName(QStringLiteral("trainerMoneyField"));
  if (moneyField == nullptr)
    moneyField = GuiApp::findItem(screen, [&](QQuickItem* i){
      const QString cls = QString::fromLatin1(i->metaObject()->className());
      if (!cls.contains(QLatin1String("TextField")) && !cls.contains(QLatin1String("TextInput"))
          && !cls.contains(QLatin1String("TextEdit")))
        return false;
      return i->property("text").toString() == moneyText;
    });

  QVERIFY2(moneyField != nullptr,
           "Could not locate the money field on the Trainer Card (objectName "
           "'trainerMoneyField' missing and no editable field shows the money value).");

  // Real input: focus, select all, type a new value, commit.
  moneyField->forceActiveFocus();
  app.settle(10);
  QTest::keyClick(app.view(), Qt::Key_A, Qt::ControlModifier);   // select all
  app.keyType(QStringLiteral("54321"));                          // type new value (per-char; see keyType)
  app.pressKey(Qt::Key_Return);                                  // accept
  screen->forceActiveFocus();                                    // blur -> editingFinished
  app.settle(40);

  QCOMPARE(bas->money, 54321u);

  // And it persists through a real save + reopen.
  const QString out = m_tmp.filePath(QStringLiteral("typed.sav"));
  QVERIFY(app.saveTo(out));

  FileManagement fm2;
  fm2.clearRecentFiles();
  fm2.addRecentFile(out);
  QVERIFY(fm2.openFileRecent(0));
  QCOMPARE(fm2.data->dataExpanded->player->basics->money, 54321u);
}

QTEST_MAIN(TestGuiInput)
#include "tst_gui_input.moc"
