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

// Mirror of MainWindow's MAX_RECENT_FILES (recentFileShortcuts[5]).
static constexpr int kMaxRecentFiles = 5;

class TestShortcuts : public QObject
{
  Q_OBJECT

private slots:
  void namedShortcuts_matchTheDocumentedBindings();
  void recentFileShortcuts_areCtrlShift0to4();
  void noTwoShortcutsShareAKeySequence();
};

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

QTEST_GUILESS_MAIN(TestShortcuts)
#include "tst_shortcuts.moc"
