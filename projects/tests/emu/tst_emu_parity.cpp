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
 * @file tst_emu_parity.cpp
 * @brief The editor, judged against the actual Game Boy.
 *
 * Every other test in this suite asks "is the editor consistent with itself?". This one
 * asks the only question that really settles it: **does the real game agree?**
 *
 * It boots the genuine ROM in an emulator with one of our save files, lets the game load
 * it, and then reads the console's own RAM back out — and demands that what MapEngine
 * built matches it byte for byte:
 *
 * - `wCurrentTileBlockMapViewPointer` — the pointer the *game* computed for where its
 *   view starts, against the one we compute from the player's coordinates.
 * - `wOverworldMap` — the entire block buffer the game assembled: the map ringed by its
 *   3-block border. Our `buildOverworldMap()` must reproduce it exactly.
 * - `wSurroundingTiles` — the 6×5-block scratch area, expanded to 24×20 tile ids.
 * - `wTileMap` — the 20×18 tile ids actually on screen. This is the whole view pipeline
 *   in one array (blocks → tiles → scratch area → half-block screen offset), with no
 *   sprites and no palettes in the way. If this matches, the map emulator is right.
 *
 * **Local-only, and it must stay that way.** It needs `assets/references/backup.gb`, a
 * legally-backed-up cartridge dump that is gitignored and is never committed, published,
 * or copied into any build artifact. Without the ROM (CI, a fresh clone, anyone else's
 * machine) every case simply SKIPs — the suite stays green and nothing is distributed.
 *
 * @see scripts/emu/dump_state.py (drives the emulator), reference/emulator-verification.md
 */

#include <QtTest>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <engine/mapengine.h>

using namespace pse_test;

namespace {

/// <repo>, derived from the assets dir the build points us at.
QString repoRoot()
{
  return QFileInfo(QString::fromUtf8(PSE_ASSETS_DIR)).absolutePath();
}

QString romPath()     { return repoRoot() + "/assets/references/backup.gb"; }
QString dumperPath()  { return repoRoot() + "/scripts/emu/dump_state.py"; }
QString outDir()      { return repoRoot() + "/tmp/emu"; }

/// The venv scripts/emu/setup.ps1 builds. No venv -> we skip, we never guess an interpreter.
QString pythonPath()  { return repoRoot() + "/tmp/emu-venv/Scripts/python.exe"; }

/// Why we'd skip -- or empty if the emulator harness is actually available.
QString unavailableReason()
{
  if (!QFile::exists(romPath()))
    return "no ROM at assets/references/backup.gb (local-only verification)";
  if (!QFile::exists(pythonPath()))
    return "no emulator venv -- run scripts/emu/setup.ps1";
  if (!QFile::exists(dumperPath()))
    return "scripts/emu/dump_state.py is missing";

  return QString();
}

} // namespace

class TestEmuParity : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void theGameAgreesWithOurViewPointer();
  void theGameAgreesWithOurBlockBuffer();
  void theBorderRing_isStillMissingItsConnections();
  void theGameAgreesWithOurScratchArea();
  void theGameAgreesWithOurScreen();

private:
  /// Boot the ROM with @p sav and read the console's RAM. Returns false if unavailable.
  bool runEmulator(const QString& sav);

  QJsonObject m_state;          ///< what the game says about where the player is
  QByteArray m_overworldMap;    ///< the game's block buffer
  QByteArray m_surroundingTiles;///< the game's 24x20 scratch tiles
  QByteArray m_tileMap;         ///< the game's 20x18 screen tiles
  bool m_ok = false;
};

void TestEmuParity::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);

  const QString why = unavailableReason();
  if (!why.isEmpty())
    QSKIP(qPrintable("emulator verification unavailable: " + why));

  m_ok = runEmulator("assets/saves/natural-clean/BaseSAV.sav");
  QVERIFY2(m_ok, "the emulator harness did not produce a state dump");
}

bool TestEmuParity::runEmulator(const QString& sav)
{
  QProcess py;
  py.setWorkingDirectory(repoRoot());
  py.start(pythonPath(), {
    dumperPath(),
    "--rom", romPath(),
    "--sav", repoRoot() + "/" + sav,
    "--out", outDir()
  });

  // Booting through the intro takes a few thousand emulated frames; be generous.
  if (!py.waitForFinished(180000)) {
    qWarning() << "emulator timed out:" << py.readAllStandardError();
    return false;
  }

  if (py.exitCode() == 2) {   // the dumper's own "unavailable" code
    QTest::qSkip(qPrintable(QString::fromUtf8(py.readAllStandardError()).trimmed()),
                 __FILE__, __LINE__);
    return false;
  }

  if (py.exitCode() != 0) {
    qWarning() << "emulator failed:" << py.readAllStandardError();
    return false;
  }

  auto read = [](const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
      return QByteArray();
    const QByteArray data = f.readAll();
    f.close();
    return data;
  };

  m_state = QJsonDocument::fromJson(read(outDir() + "/state.json")).object();
  m_overworldMap = read(outDir() + "/wOverworldMap.bin");
  m_surroundingTiles = read(outDir() + "/wSurroundingTiles.bin");
  m_tileMap = read(outDir() + "/wTileMap.bin");

  return !m_state.isEmpty() && !m_overworldMap.isEmpty()
      && !m_surroundingTiles.isEmpty() && !m_tileMap.isEmpty();
}

// ─────────────────────────────────────────────────────────────────────────────

void TestEmuParity::theGameAgreesWithOurViewPointer()
{
  const int map = m_state["curMap"].toInt();
  const int x = m_state["xCoord"].toInt();
  const int y = m_state["yCoord"].toInt();
  const int width = m_state["width"].toInt();

  // The game loaded the save we gave it, and its own header agrees with our DB.
  const auto buffer = MapEngine::buildOverworldMap(map);
  QVERIFY(buffer.valid);
  QCOMPARE(buffer.width, width);
  QCOMPARE(buffer.height, m_state["height"].toInt());
  QCOMPARE(MapEngine::tilesetOf(map), m_state["tileset"].toInt());

  // And the pointer the console computed for itself is the one we compute for it.
  QCOMPARE(MapEngine::viewPointer(x, y, width), m_state["viewPointer"].toInt());
}

void TestEmuParity::theGameAgreesWithOurBlockBuffer()
{
  const auto buffer = MapEngine::buildOverworldMap(m_state["curMap"].toInt());
  QVERIFY(buffer.valid);

  QCOMPARE(buffer.stride, m_state["stride"].toInt());
  QCOMPARE(buffer.rows, m_state["rows"].toInt());
  QCOMPARE(buffer.blocks.size(), m_overworldMap.size());

  // THE MAP ITSELF must match the console block for block. No excuses here.
  const int border = MapEngine::mapBorder;

  for (int row = border; row < border + buffer.height; row++) {
    for (int col = border; col < border + buffer.width; col++) {
      const int i = row * buffer.stride + col;
      if (buffer.blocks[i] == m_overworldMap[i])
        continue;

      QFAIL(qPrintable(QString("map block (%1,%2): the game has %3, we built %4")
                       .arg(col - border).arg(row - border)
                       .arg(static_cast<quint8>(m_overworldMap[i]))
                       .arg(static_cast<quint8>(buffer.blocks[i]))));
    }
  }
}

void TestEmuParity::theBorderRing_isStillMissingItsConnections()
{
  // The border ring is the one place we KNOWINGLY differ from the console, and the
  // emulator found it the first time it was pointed at us.
  //
  // We fill the 3-block ring with the map's border block. The game does that too -- and
  // then bleeds the CONNECTED maps' edge strips over the top of it
  // (LoadNorthSouthConnectionsTileMap / LoadEastWestConnectionsTileMap). Pallet Town
  // connects north to Route 1 and south to Route 21, so its ring is really Route 1's
  // bottom rows and Route 21's top rows -- not a wall of trees.
  //
  // Connection strips are the next step of the map emulator. This test is deliberately
  // an EXPECTED failure rather than a skip: the moment they land, it starts passing and
  // this expectation has to be deleted -- so the gap cannot be quietly forgotten.
  const auto buffer = MapEngine::buildOverworldMap(m_state["curMap"].toInt());
  QVERIFY(buffer.valid);

  const int border = MapEngine::mapBorder;
  int differing = 0;

  for (int row = 0; row < buffer.rows; row++) {
    for (int col = 0; col < buffer.stride; col++) {
      const bool insideMap = col >= border && col < border + buffer.width
                          && row >= border && row < border + buffer.height;
      if (insideMap)
        continue;

      const int i = row * buffer.stride + col;
      if (buffer.blocks[i] != m_overworldMap[i])
        differing++;
    }
  }

  QEXPECT_FAIL("", "connection strips are not implemented yet -- the ring should carry the "
                   "neighbouring maps' edges, not the border block (see next-steps.md)",
               Continue);
  QCOMPARE(differing, 0);
}

void TestEmuParity::theGameAgreesWithOurScratchArea()
{
  const int map = m_state["curMap"].toInt();
  const auto buffer = MapEngine::buildOverworldMap(map);

  const QByteArray ours = MapEngine::surroundingTiles(
    buffer, m_state["tileset"].toInt(),
    m_state["xCoord"].toInt(), m_state["yCoord"].toInt());

  QCOMPARE(ours.size(), m_surroundingTiles.size());
  QCOMPARE(ours, m_surroundingTiles);
}

void TestEmuParity::theGameAgreesWithOurScreen()
{
  const int map = m_state["curMap"].toInt();
  const auto buffer = MapEngine::buildOverworldMap(map);

  const QByteArray ours = MapEngine::screenTiles(
    buffer, m_state["tileset"].toInt(),
    m_state["xCoord"].toInt(), m_state["yCoord"].toInt());

  QCOMPARE(ours.size(), m_tileMap.size());

  // Tile-for-tile, over the exact 20x18 the Game Boy was displaying.
  for (int i = 0; i < ours.size(); i++) {
    if (ours[i] == m_tileMap[i])
      continue;

    QFAIL(qPrintable(QString("screen tile (%1,%2): the game shows 0x%3, we show 0x%4")
                     .arg(i % MapEngine::screenTilesW)
                     .arg(i / MapEngine::screenTilesW)
                     .arg(static_cast<quint8>(m_tileMap[i]), 2, 16, QChar('0'))
                     .arg(static_cast<quint8>(ours[i]), 2, 16, QChar('0'))));
  }
}

QTEST_MAIN(TestEmuParity)
#include "tst_emu_parity.moc"
