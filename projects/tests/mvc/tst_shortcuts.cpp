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
 * @file tst_shortcuts.cpp
 * @brief Guards the global keyboard-shortcut KEY SEQUENCES against accidental
 *        rebinds/typos. MainWindow::setupShortcuts() builds its QShortcuts from
 *        pse::shortcutKeyMap() (the shared single source of truth in
 *        boot/shortcutdefs.h) and wires each named action to its FileManagement
 *        verb; this test pins that map to the documented bindings and proves no
 *        two shortcuts (named + recent-file) collide on the same key sequence.
 *
 * It deliberately does NOT instantiate MainWindow (a QtWidgets + QML shell that
 * lives in the exe, not appcore) -- the rebind risk is in the key DEFINITIONS,
 * which now live in a header the test links directly. The action→verb wiring is
 * a thin set of connect() calls covered by the FileManagement verb tests.
 */

#include <QtTest>
#include <QKeySequence>
#include <QHash>
#include <QSet>

#include <boot/shortcutdefs.h>

#include "../helpers/savefilefixture.h"
#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

using namespace pse_test;

// Mirror of MainWindow's MAX_RECENT_FILES (recentFileShortcuts[5]).
static constexpr int kMaxRecentFiles = 5;

class TestShortcuts : public QObject
{
  Q_OBJECT

  // Load BaseSAV into a FileManagement via the no-dialog recent path.
  static bool loadBase(FileManagement& fm)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(assetPath(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
    return fm.openFileRecent(0);
  }

private slots:
  void initTestCase();
  void namedShortcuts_matchTheDocumentedBindings();
  void recentFileShortcuts_areCtrlShift0to4();
  void noTwoShortcutsShareAKeySequence();
  void everyShortcutHasAnAction();
  void firingSafeActions_runsTheRightVerb();
};

void TestShortcuts::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QCOMPARE(readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")).size(), kSaveSize);
}

// Every named action is present and bound to exactly the expected key sequence.
void TestShortcuts::namedShortcuts_matchTheDocumentedBindings()
{
  const QHash<QString, QKeySequence> m = pse::shortcutKeyMap();

  // The expected contract (action id -> portable key string).
  const QHash<QString, QString> expected = {
    { "new",               "Ctrl+N" },
    { "open",              "Ctrl+O" },
    { "reopen",            "Ctrl+Shift+O" },
    { "save",              "Ctrl+S" },
    { "saveas",            "Ctrl+Shift+S" },
    { "savecopyas",        "Ctrl+Alt+S" },
    { "scrub",             "Ctrl+Shift+W" },
    { "clear-recentfiles", "Ctrl+Shift+-" },
    { "exit",              "Ctrl+Q" },
    { "exit2",             "Alt+F4" },
    { "random",            "Ctrl+Shift+R" },
  };

  // Exactly these actions, no more, no fewer (a removed/added binding is caught).
  QCOMPARE(m.size(), expected.size());

  for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) {
    QVERIFY2(m.contains(it.key()), qPrintable("missing shortcut action: " + it.key()));
    const QString actual = m.value(it.key()).toString(QKeySequence::PortableText);
    QVERIFY2(actual == it.value(),
             qPrintable("wrong key for '" + it.key() + "': got " + actual + ", want " + it.value()));
  }
}

// Recent-file shortcuts are Ctrl+Shift+0 .. Ctrl+Shift+4.
void TestShortcuts::recentFileShortcuts_areCtrlShift0to4()
{
  for (int i = 0; i < kMaxRecentFiles; ++i) {
    const QString actual = pse::recentFileShortcutKey(i).toString(QKeySequence::PortableText);
    const QString want   = QStringLiteral("Ctrl+Shift+%1").arg(i);
    QVERIFY2(actual == want,
             qPrintable(QStringLiteral("recent-file shortcut %1: got %2, want %3").arg(i).arg(actual, want)));
  }
}

// No accidental rebind: the named shortcuts and the recent-file shortcuts must all
// be distinct key sequences (a duplicate would make one binding shadow another).
void TestShortcuts::noTwoShortcutsShareAKeySequence()
{
  QSet<QString> seen;
  QStringList dupes;

  const QHash<QString, QKeySequence> m = pse::shortcutKeyMap();
  for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
    const QString k = it.value().toString(QKeySequence::PortableText);
    if (seen.contains(k)) dupes << (it.key() + "=" + k);
    seen.insert(k);
  }
  for (int i = 0; i < kMaxRecentFiles; ++i) {
    const QString k = pse::recentFileShortcutKey(i).toString(QKeySequence::PortableText);
    if (seen.contains(k)) dupes << ("recent" + QString::number(i) + "=" + k);
    seen.insert(k);
  }

  QVERIFY2(dupes.isEmpty(), qPrintable("duplicate shortcut key sequence(s): " + dupes.join(", ")));
}

// Every named shortcut (a key sequence) has exactly one defined action, and vice
// versa -- so no shortcut fires nothing, and no action lacks a key.
void TestShortcuts::everyShortcutHasAnAction()
{
  FileManagement fm;
  QVERIFY(loadBase(fm));

  const QList<QString> keys    = pse::shortcutKeyMap().keys();
  const QList<QString> actions = pse::shortcutActions(&fm, []{}).keys();

  QCOMPARE(QSet<QString>(actions.begin(), actions.end()),
           QSet<QString>(keys.begin(), keys.end()));
}

// Fire the side-effect-free, non-dialog shortcut verbs through the SHARED action map
// (the same callables MainWindow connects each QShortcut to) and assert each ran. The
// dialog verbs (open/reopen/saveas/savecopyas) and `save` (needs a path) can't run
// headless; their bindings are pinned above and the wiring loop is uniform.
void TestShortcuts::firingSafeActions_runsTheRightVerb()
{
  FileManagement fm;
  QVERIFY(loadBase(fm));

  int exitCalls = 0;
  const auto act = pse::shortcutActions(&fm, [&exitCalls]{ ++exitCalls; });

  // clear-recentfiles: a remembered recent goes away.
  fm.addRecentFile(assetPath(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
  QVERIFY(fm.recentFilesCount() > 0);
  act.value("clear-recentfiles")();
  QCOMPARE(fm.recentFilesCount(), 0);

  // scrub: wipeUnusedSpace must run without crashing (and keep a valid 32 KB save).
  act.value("scrub")();
  QVERIFY(fm.data != nullptr && fm.data->dataExpanded != nullptr);

  // random: the trainer's money lands inside the randomizer's documented range.
  act.value("random")();
  const unsigned int m = fm.data->dataExpanded->player->basics->money;
  QVERIFY2(m >= 100u && m <= 6000u, qPrintable(QStringLiteral("random money out of range: %1").arg(m)));

  // exit / exit2: both invoke the onExit callback (window close in the app).
  act.value("exit")();
  act.value("exit2")();
  QCOMPARE(exitCalls, 2);

  // new: a fresh blank save -- BaseSAV's 8 badges are gone and money is zeroed.
  act.value("new")();
  QCOMPARE(fm.data->dataExpanded->player->basics->badgeCount(), 0);
  QCOMPARE(fm.data->dataExpanded->player->basics->money, 0u);
}

QTEST_GUILESS_MAIN(TestShortcuts)
#include "tst_shortcuts.moc"
