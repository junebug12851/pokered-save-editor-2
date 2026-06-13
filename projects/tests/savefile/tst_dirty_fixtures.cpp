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
 * @file tst_dirty_fixtures.cpp
 * @brief Negative coverage over the committed dirty/malformed saves in
 *        `assets/dirty/` (+ checksum-corruption cases derived from BaseSAV at runtime).
 *        Proves the graceful-degradation promise on hostile input: wrong-size files
 *        are cleanly REJECTED (no crash, error surfaced, any open save untouched);
 *        right-size garbage (all-00 / all-FF / random) LOADS, EXPANDS and FLATTENS
 *        without crashing; a bad checksum still loads (the editor recomputes on save).
 *
 * Each dirty file and its intent is documented in `assets/dirty/README.md`.
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/expanded/savefileexpanded.h>

using namespace pse_test;

class TestDirtyFixtures : public QObject
{
  Q_OBJECT

  QTemporaryDir m_tmp;

  static QByteArray liveBytes(FileManagement& fm)
  {
    return QByteArray(reinterpret_cast<const char*>(fm.data->data), kSaveSize);
  }

  static QString dirtyPath(const QString& name)
  {
    return assetPath(QStringLiteral("dirty/") + name);
  }

  // Open a path via the no-dialog recent-file route.
  static bool open(FileManagement& fm, const QString& path)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(path);
    return fm.openFileRecent(0);
  }

  // Load BaseSAV, returning its live bytes (for the "untouched on failed load" checks).
  QByteArray loadGood(FileManagement& fm)
  {
    bool ok = open(fm, assetPath(QStringLiteral("BaseSAV.sav")));
    [&]{ QVERIFY(ok); }();
    return liveBytes(fm);
  }

  // Copy BaseSAV to a temp file with one byte XOR-flipped at @p offset.
  QString baseWithFlippedByte(const QString& tag, int offset)
  {
    QByteArray bytes = readSaveBytes(QStringLiteral("BaseSAV.sav"));
    bytes[offset] = static_cast<char>(bytes[offset] ^ 0xFF);
    const QString path = m_tmp.filePath(tag + QStringLiteral(".sav"));
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) { f.write(bytes); f.close(); }
    return path;
  }

private slots:
  void initTestCase();

  void wrongSizeFiles_areRejected_data();
  void wrongSizeFiles_areRejected();

  void rightSizeGarbage_loadsExpandsFlattensWithoutCrashing_data();
  void rightSizeGarbage_loadsExpandsFlattensWithoutCrashing();

  void oversizedFile_loadsFirst32K();
  void badChecksums_stillLoadWithoutCrashing();
};

void TestDirtyFixtures::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());
  // The dirty fixtures must be present.
  QVERIFY2(QFile::exists(dirtyPath(QStringLiteral("all_ff.sav"))),
           "assets/dirty/ fixtures missing -- see assets/dirty/README.md to regenerate");
}

// Empty and short files (< 0x8000) must be rejected cleanly, an error surfaced, and
// the currently-open save left byte-for-byte untouched.
void TestDirtyFixtures::wrongSizeFiles_areRejected_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("empty")            << QStringLiteral("empty.sav");
  QTest::newRow("truncated_4kb")    << QStringLiteral("truncated_4kb.sav");
  QTest::newRow("one_byte_short")   << QStringLiteral("truncated_32767.sav");
}

void TestDirtyFixtures::wrongSizeFiles_areRejected()
{
  QFETCH(QString, file);

  FileManagement fm;
  const QByteArray before = loadGood(fm);

  QVERIFY2(!open(fm, dirtyPath(file)), qPrintable(file + " (<32KB) should be rejected"));
  QVERIFY2(!fm.getLastErrorMessage().isEmpty(), "a rejected load must surface a message");

  // Sacred: a failed load never mutates the already-open save.
  QCOMPARE(liveBytes(fm), before);
}

// Exactly-32KB garbage must load + expand + flatten + recalc without crashing.
// (Expansion reads counts/indices that are out of range here -- all-FF is the harshest.)
void TestDirtyFixtures::rightSizeGarbage_loadsExpandsFlattensWithoutCrashing_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("all_zeros") << QStringLiteral("all_zeros.sav");
  QTest::newRow("all_ff")    << QStringLiteral("all_ff.sav");
  QTest::newRow("garbage")   << QStringLiteral("garbage.sav");
}

void TestDirtyFixtures::rightSizeGarbage_loadsExpandsFlattensWithoutCrashing()
{
  QFETCH(QString, file);

  FileManagement fm;
  QVERIFY2(open(fm, dirtyPath(file)), qPrintable(file + " (32KB) should load"));

  // The load already expanded the data (dataExpanded). Now exercise flatten + recalc;
  // none of this may crash on hostile input (graceful degradation).
  QVERIFY(fm.data->dataExpanded != nullptr);
  fm.data->flattenData();
  fm.data->toolset->recalcChecksums();

  // A second open of the same hostile file is also fine (no accumulated state crash).
  QVERIFY(open(fm, dirtyPath(file)));
}

// A larger-than-32KB file loads its first 0x8000 bytes (documented behaviour).
void TestDirtyFixtures::oversizedFile_loadsFirst32K()
{
  FileManagement fm;
  QVERIFY(open(fm, dirtyPath(QStringLiteral("oversized_48kb.sav"))));
  // The oversized fixture is all zeros, so the loaded image is the 32KB zero prefix.
  QCOMPARE(fm.data->data[0], var8(0x00));
}

// The editor is not the game: a save with a wrong checksum should still LOAD (it
// recomputes checksums when it writes), not crash or refuse.
void TestDirtyFixtures::badChecksums_stillLoadWithoutCrashing()
{
  // Main-data checksum lives at 0x3523; bank-2 box checksum at 0x5A4C.
  const QString badMain = baseWithFlippedByte(QStringLiteral("bad_main_ck"), 0x3523);
  const QString badBox  = baseWithFlippedByte(QStringLiteral("bad_box_ck"),  0x5A4C);

  FileManagement fm1;
  QVERIFY2(open(fm1, badMain), "a bad main checksum should still load (editor recomputes on save)");
  QVERIFY(fm1.data->dataExpanded != nullptr);
  fm1.data->flattenData();
  fm1.data->toolset->recalcChecksums();   // recompute makes it valid again

  FileManagement fm2;
  QVERIFY2(open(fm2, badBox), "a bad box checksum should still load");
  QVERIFY(fm2.data->dataExpanded != nullptr);
}

QTEST_GUILESS_MAIN(TestDirtyFixtures)
#include "tst_dirty_fixtures.moc"
