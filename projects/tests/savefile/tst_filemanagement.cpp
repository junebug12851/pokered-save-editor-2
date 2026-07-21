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
 * @file tst_filemanagement.cpp
 * @brief Coverage for FileManagement's non-GUI logic: the recent-files list
 *        (add/de-dupe/trim/cap, swap, remove, clear, the persisted-blob expand +
 *        startup prune of unopenable entries) and the load-error reporting path
 *        (cannot-open vs too-short plain-English messages) reached through
 *        openFileRecent without a file dialog. The native open/save dialogs and
 *        the to-disk save path are GUI/covered elsewhere (tst_e2e) and not here.
 *
 * QSettings is isolated under a test org/app name so the real app's recent list
 * is never touched (mirrors tst_errors).
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include <QByteArray>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/filemanagement.h>

using namespace pse_test;

class TestFileManagement : public QObject
{
  Q_OBJECT

private:
  QTemporaryDir m_tmp;
  QString m_realFile;   // a guaranteed-readable, full-size save (the fixture)
  QString m_shortFile;  // a real but truncated (<32 KB) file

private slots:
  void initTestCase();
  void recents_addDeDupeTrimClear();
  void recents_capBoundsTheList();
  void recents_swapAndRemove();
  void recents_expandBlobAndPruneOnConstruct();
  void loadError_cannotOpenMessage();
  void loadError_tooShortMessage();
  void newFile_reopenEmpty_wipe_noCrash();
};

void TestFileManagement::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);

  m_realFile = assetPath(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(QFile::exists(m_realFile));

  QVERIFY(m_tmp.isValid());
  m_shortFile = m_tmp.path() + QStringLiteral("/short.sav");
  QFile f(m_shortFile);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write(QByteArray(100, '\0')); // 100 bytes -- far below the 32 KB minimum
  f.close();
}

void TestFileManagement::recents_addDeDupeTrimClear()
{
  FileManagement fm;
  fm.clearRecentFiles();
  QCOMPARE(fm.recentFilesCount(), 0);

  fm.addRecentFile(QStringLiteral("x.sav"));
  fm.addRecentFile(QStringLiteral("y.sav"));     // most-recent first
  QCOMPARE(fm.recentFilesCount(), 2);
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("y.sav"));
  QCOMPARE(fm.getRecentFile(1), QStringLiteral("x.sav"));

  // Re-adding an existing path de-dupes (moves it back to the top, no duplicate).
  fm.addRecentFile(QStringLiteral("x.sav"));
  QCOMPARE(fm.recentFilesCount(), 2);
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("x.sav"));

  // Whitespace is trimmed; a blank/whitespace-only entry is dropped entirely.
  fm.addRecentFile(QStringLiteral("  z.sav  "));
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("z.sav"));
  const int before = fm.recentFilesCount();
  fm.addRecentFile(QStringLiteral("    "));
  QCOMPARE(fm.recentFilesCount(), before); // blank dropped

  fm.clearRecentFiles();
  QCOMPARE(fm.recentFilesCount(), 0);
  QVERIFY(fm.getRecentFiles().isEmpty());
}

// Adding more than the cap keeps exactly MAX_RECENT_FILES, most-recent first.
// (Regression guard for the off-by-one fixed 2026-06-08: the cap loop appended
// then broke on `> MAX`, retaining MAX+1; corrected to `>= MAX`. Confirmed the
// extra slot was not a sentinel -- mainwindow bounds its menu at MAX and the
// RecentFilesModel reads recentFilesCount() directly, so nothing relied on it.)
void TestFileManagement::recents_capBoundsTheList()
{
  FileManagement fm;
  fm.clearRecentFiles();

  const int max = fm.recentFilesMax();
  QCOMPARE(max, int(MAX_RECENT_FILES));

  for(int i = 0; i < max + 3; i++)
    fm.addRecentFile(QStringLiteral("file%1.sav").arg(i));

  QCOMPARE(fm.recentFilesCount(), max); // exactly the cap -- no extra slot

  // The most-recently-added entry is always retained at the top.
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("file%1.sav").arg(max + 2));
}

void TestFileManagement::recents_swapAndRemove()
{
  FileManagement fm;
  fm.clearRecentFiles();
  fm.addRecentFile(QStringLiteral("a.sav"));
  fm.addRecentFile(QStringLiteral("b.sav"));
  fm.addRecentFile(QStringLiteral("c.sav")); // list: [c, b, a]

  fm.recentFilesSwap(0, 2);                  // -> [a, b, c]
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("a.sav"));
  QCOMPARE(fm.getRecentFile(2), QStringLiteral("c.sav"));

  fm.recentFilesRemove(1);                   // drop "b" -> [a, c]
  QCOMPARE(fm.recentFilesCount(), 2);
  QCOMPARE(fm.getRecentFile(0), QStringLiteral("a.sav"));
  QCOMPARE(fm.getRecentFile(1), QStringLiteral("c.sav"));
}

// The constructor's reset() expands the persisted ';'-joined blob into the list
// and then prunes any entry it can't open for reading -- so a real file survives
// while a bogus path is silently dropped.
void TestFileManagement::recents_expandBlobAndPruneOnConstruct()
{
  const QString bogus = m_tmp.path() + QStringLiteral("/does_not_exist_zzz.sav");
  {
    QSettings settings; // same org/app as set in initTestCase
    settings.setValue(QString::fromUtf8(KEY_RECENT_FILES),
                      m_realFile + QStringLiteral(";") + bogus);
    settings.sync();
  }

  FileManagement fm; // reset() -> expandRecentFiles(blob) -> pruneRecentFiles()

  const QList<QString> recents = fm.getRecentFiles();
  QVERIFY2(recents.contains(m_realFile), "openable file was pruned");
  QVERIFY2(!recents.contains(bogus), "unopenable file was not pruned");
}

void TestFileManagement::loadError_cannotOpenMessage()
{
  FileManagement fm;
  fm.clearRecentFiles();
  const QString missing = m_tmp.path() + QStringLiteral("/missing_open_target.sav");
  fm.addRecentFile(missing);

  const bool ok = fm.openFileRecent(0); // reads recent[0] -> loadData -> fails
  QVERIFY(!ok);
  QVERIFY2(fm.getLastErrorMessage().contains(QStringLiteral("couldn't be opened")),
           qPrintable(fm.getLastErrorMessage()));
  QVERIFY(!fm.getLastErrorDetail().isEmpty()); // real OS/Qt reason captured
}

void TestFileManagement::loadError_tooShortMessage()
{
  FileManagement fm;
  fm.clearRecentFiles();
  fm.addRecentFile(m_shortFile);

  const bool ok = fm.openFileRecent(0);
  QVERIFY(!ok);
  QVERIFY2(fm.getLastErrorMessage().contains(QStringLiteral("truncated or corrupted")),
           qPrintable(fm.getLastErrorMessage()));
  QVERIFY(fm.getLastErrorDetail().contains(QStringLiteral("bytes"))); // size detail
}

void TestFileManagement::newFile_reopenEmpty_wipe_noCrash()
{
  FileManagement fm;

  fm.newFile();
  QCOMPARE(fm.getPath(), QString(""));

  fm.reopenFile();          // empty path -> resetData, no disk read, no crash
  QCOMPARE(fm.getPath(), QString(""));

  fm.wipeUnusedSpace();     // resetData(true) -- must not crash
}

QTEST_GUILESS_MAIN(TestFileManagement)
#include "tst_filemanagement.moc"
