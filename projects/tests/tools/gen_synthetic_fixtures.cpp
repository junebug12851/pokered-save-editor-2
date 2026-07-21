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
 * @file gen_synthetic_fixtures.cpp
 * @brief One-shot generator for the SYNTHETIC test saves in `assets/saves/synthetic-clean/`.
 *
 * These are NOT real game saves -- they are deterministic edge-case saves built from
 * a FRESH New File (so they embed no real progressed/personal data) using the app's
 * own save engine, so they are byte-accurate and reproducible. Run it once and commit
 * the output; it stays in the tree as the reproducible source-of-truth (the dirty/
 * fixtures README points here for synthetic ones). Real saves remain project leadership's to
 * commit; only these clearly-synthetic, engine-generated files live in assets/saves/synthetic-clean/.
 *
 * Build target `gen_synthetic_fixtures` (not a CTest test). Run with no args; it writes
 * into <repo>/assets/saves/synthetic-clean/ (located via PSE_ASSETS_DIR).
 *
 * Consumed by tst_synthetic_fixtures.cpp (savefile round-trip matrix) and referenced by
 * the GUI tests as "saves/synthetic-clean/<name>.sav".
 */

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <functional>

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

static QString g_outDir;

// Build one variant from a fresh New File, apply @p edit, write to assets/saves/synthetic-clean/<name>.
static bool makeVariant(const QString& name, const std::function<void(PlayerBasics*)>& edit)
{
  FileManagement fm;
  fm.newFile();   // blank, freshly-started save (no real data)
  edit(fm.data->dataExpanded->player->basics);

  const QString path = g_outDir + QStringLiteral("/") + name;
  fm.setPath(path);
  const bool ok = fm.saveFile();
  qInfo("  %-26s %s", qPrintable(name), ok ? "OK" : "FAILED");
  return ok;
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  (void)DB::inst();   // boot the game databases (needed by the expand/flatten engine)

  g_outDir = QString::fromUtf8(PSE_ASSETS_DIR) + QStringLiteral("/saves/synthetic-clean");
  QDir().mkpath(g_outDir);

  qInfo("Generating synthetic fixtures into %s", qPrintable(g_outDir));

  bool ok = true;

  // Everything maxed: money/coins at their caps, all eight badges earned.
  ok &= makeVariant(QStringLiteral("new_maxed.sav"), [](PlayerBasics* b){
    b->money = 999999;
    b->coins = 9999;
    for (int i = 0; i < maxBadges; ++i) b->badgeSet(i, true);
  });

  // Explicitly zeroed: money/coins 0, no badges (minimal-state probe).
  ok &= makeVariant(QStringLiteral("new_zeroed.sav"), [](PlayerBasics* b){
    b->money = 0;
    b->coins = 0;
    for (int i = 0; i < maxBadges; ++i) b->badgeSet(i, false);
  });

  // All badges, default money/coins (badge-byte pattern probe).
  ok &= makeVariant(QStringLiteral("new_allbadges.sav"), [](PlayerBasics* b){
    for (int i = 0; i < maxBadges; ++i) b->badgeSet(i, true);
  });

  // A mid-game-ish mix: partial money/coins and the first four badges.
  ok &= makeVariant(QStringLiteral("new_midgame.sav"), [](PlayerBasics* b){
    b->money = 123456;
    b->coins = 4321;
    for (int i = 0; i < 4; ++i) b->badgeSet(i, true);
  });

  qInfo("%s", ok ? "All synthetic fixtures written." : "One or more failed!");
  return ok ? 0 : 1;
}
