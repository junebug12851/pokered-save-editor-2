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
#pragma once

/**
 * @file shortcutdefs.h
 * @brief Single source of truth for the global keyboard shortcut KEY SEQUENCES.
 *
 * `MainWindow::setupShortcuts()` builds its `QShortcut`s from this map (and wires
 * each named action to its FileManagement verb), and `tst_shortcuts` asserts the
 * map against the documented bindings + checks for accidental duplicates/rebinds.
 * Keeping the keys here (header-only, in appcore's include root) lets the test see
 * exactly what the window uses, so a typo or a clashing rebind is caught.
 *
 * Only the KEY SEQUENCES live here -- the action→verb wiring stays in MainWindow
 * (it needs the live FileManagement instance).
 */

#include <QHash>
#include <QString>
#include <QKeySequence>
#include <Qt>
#include <functional>

#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>

namespace pse {

/// Named global shortcuts: action id -> key sequence. The action ids match the
/// names MainWindow::setupShortcuts() connects to FileManagement verbs.
inline QHash<QString, QKeySequence> shortcutKeyMap()
{
  return {
    { QStringLiteral("new"),               QKeySequence(Qt::CTRL | Qt::Key_N) },
    { QStringLiteral("open"),              QKeySequence(Qt::CTRL | Qt::Key_O) },
    { QStringLiteral("reopen"),            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O) },
    { QStringLiteral("save"),              QKeySequence(Qt::CTRL | Qt::Key_S) },
    { QStringLiteral("saveas"),            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S) },
    { QStringLiteral("savecopyas"),        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S) },
    { QStringLiteral("scrub"),             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W) },
    { QStringLiteral("clear-recentfiles"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus) },
    { QStringLiteral("exit"),              QKeySequence(Qt::CTRL | Qt::Key_Q) },
    { QStringLiteral("exit2"),             QKeySequence(Qt::ALT | Qt::Key_F4) },
    { QStringLiteral("random"),            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R) },
  };
}

/// Recent-file shortcut @p i (0..MAX_RECENT_FILES-1) == Ctrl+Shift+(0+i), i.e.
/// Ctrl+Shift+0 .. Ctrl+Shift+4. (Key_0 == 0x30, so 0x30 + i.)
inline QKeySequence recentFileShortcutKey(int i)
{
  return QKeySequence(Qt::CTRL | Qt::SHIFT | static_cast<Qt::Key>(0x30 + i));
}

/// What each named shortcut DOES, action id -> callable, over a live FileManagement.
/// The single source of truth for the shortcut→verb wiring: MainWindow connects each
/// QShortcut's `activated` to the matching callable here, and tst_shortcuts fires the
/// side-effect-free / non-dialog verbs directly to prove the mapping is correct. The
/// action ids match shortcutKeyMap()'s keys (every key has exactly one action).
/// @param onExit  invoked for the exit/exit2 shortcuts (MainWindow passes window close).
inline QHash<QString, std::function<void()>> shortcutActions(FileManagement* file,
                                                             std::function<void()> onExit)
{
  return {
    { QStringLiteral("new"),               [file]{ file->newFile(); } },
    { QStringLiteral("open"),              [file]{ file->openFile(); } },
    { QStringLiteral("reopen"),            [file]{ file->reopenFile(); } },
    { QStringLiteral("save"),              [file]{ file->saveFile(); } },
    { QStringLiteral("saveas"),            [file]{ file->saveFileAs(); } },
    { QStringLiteral("savecopyas"),        [file]{ file->saveFileCopy(); } },
    { QStringLiteral("scrub"),             [file]{ file->wipeUnusedSpace(); } },
    { QStringLiteral("clear-recentfiles"), [file]{ file->clearRecentFiles(); } },
    { QStringLiteral("exit"),              [onExit]{ if (onExit) onExit(); } },
    { QStringLiteral("exit2"),             [onExit]{ if (onExit) onExit(); } },
    { QStringLiteral("random"),            [file]{ file->data->randomizeExpansion(); } },
  };
}

} // namespace pse
