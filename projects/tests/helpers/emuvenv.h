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
 * @file emuvenv.h
 * @brief The single gate every emulator-backed test asks "can I actually run the
 *        console here?" — the ROM, and a PyBoy interpreter that really executes.
 *
 * The emulator tests are local-only by design: they need the cartridge
 * (`assets/references/backup.gb`, never committed) and the PyBoy venv that
 * `scripts/emu/setup.ps1` builds. Without either they **SKIP** — that is the
 * designed behaviour, and it is why CI is green without a ROM.
 *
 * @section emuvenv_trap Why this is not just QFile::exists()
 *
 * It used to be, in three copies, each hardcoding the **Windows** venv layout
 * (`tmp/emu-venv/Scripts/python.exe`). That held only because the one machine
 * that had a venv was the one that made it.
 *
 * The Linux container broke it (2026-07-17). `docker/run-tests.sh` rsyncs the
 * repo into the container and only excluded `.git/` and the build dirs — so the
 * host's **Windows** `tmp/emu-venv/` rode along. `QFile::exists()` on
 * `Scripts/python.exe` then answered **true on Linux**, the skip never fired, and
 * the test tried to exec a Windows PE binary. WSL's binfmt interop picked it up
 * and failed sideways:
 *
 *     <3>WSL (3483 - ) ERROR: UtilGetPpid:1330: Failed to parse: /proc/1/stat
 *
 * — which surfaced as 3 red tests that looked like a code fault and were not.
 * (`run-tests.sh` now excludes `tmp/` too, but that only removes today's way to
 * trip it; this is the fix that removes the class.)
 *
 * So the gate asks the only question that actually matters: **does this
 * interpreter run?** A path that exists but cannot execute is not a venv.
 */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace pse_test::emu {

/// <repo>, derived from the assets dir the build points us at.
inline QString repoRoot()
{
  return QFileInfo(QString::fromUtf8(PSE_ASSETS_DIR)).absolutePath();
}

/// The cartridge. Git-ignored, never committed, never shipped.
inline QString romPath() { return repoRoot() + "/assets/references/backup.gb"; }

/**
 * @brief The PyBoy venv interpreter for THIS platform, or empty if there isn't one.
 *
 * Windows puts it in `Scripts/python.exe`; POSIX in `bin/python3`. We check the
 * platform's own layout first but accept either, because the venv directory is a
 * host artifact that can be copied across platforms (see the file header) — and
 * finding the *wrong* one is exactly what runs() is here to catch.
 */
inline QString pythonPath()
{
  const QDir venv(repoRoot() + "/tmp/emu-venv");
  const QStringList candidates {
#ifdef Q_OS_WIN
    "Scripts/python.exe", "bin/python3", "bin/python",
#else
    "bin/python3", "bin/python", "Scripts/python.exe",
#endif
  };
  for (const QString& c : candidates) {
    const QString p = venv.absoluteFilePath(c);
    if (QFile::exists(p))
      return p;
  }
  return QString();
}

/**
 * @brief Does that interpreter actually execute here?
 *
 * The question QFile::exists() cannot answer. Cached — it spawns a process, and
 * every test case would otherwise re-ask it.
 */
inline bool pythonRuns()
{
  static const bool ok = [] {
    const QString py = pythonPath();
    if (py.isEmpty())
      return false;
    QProcess p;
    p.start(py, {QStringLiteral("--version")});
    if (!p.waitForFinished(30000)) {
      p.kill();
      p.waitForFinished(5000);
      return false;
    }
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
  }();
  return ok;
}

/**
 * @brief Why this test would skip — or empty when the console harness is really available.
 * @param alsoNeeded extra repo-relative paths the caller's runner script needs.
 */
inline QString unavailableReason(const QStringList& alsoNeeded = {})
{
  if (!QFile::exists(romPath()))
    return QStringLiteral("no ROM at assets/references/backup.gb (local-only verification)");
  if (pythonPath().isEmpty())
    return QStringLiteral("no emulator venv -- run scripts/emu/setup.ps1");
  if (!pythonRuns())
    return QStringLiteral("tmp/emu-venv holds an interpreter that will not run on this platform "
                          "(a venv built for another OS) -- rebuild it with scripts/emu/setup.ps1");
  for (const QString& rel : alsoNeeded) {
    if (!QFile::exists(repoRoot() + "/" + rel))
      return rel + QStringLiteral(" is missing");
  }
  return QString();
}

} // namespace pse_test::emu
