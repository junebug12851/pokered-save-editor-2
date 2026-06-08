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
 * @file tst_errors.cpp
 * @brief Phase-5 negative / error-path coverage -- the graceful-degradation
 *        promise. A bad load (missing / truncated file) must fail cleanly: return
 *        false, surface a plain-English message, and leave the currently-loaded
 *        save COMPLETELY untouched. An oversized file loads its first 32 KB. And
 *        SaveFile::setData(nullptr) is a safe no-op (the s14 crash fix).
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/filemanagement.h>

using namespace pse_test;

class TestErrors : public QObject
{
  Q_OBJECT

private:
  QTemporaryDir m_tmp;
  QString m_truncated;
  QString m_oversized;
  QString m_missing;

  static QByteArray liveBytes(FileManagement& fm)
  {
    return QByteArray(reinterpret_cast<const char*>(fm.data->data), kSaveSize);
  }

  // Load the good fixture into fm via the recent-file path (no dialog), return its bytes.
  QByteArray loadGood(FileManagement& fm)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(assetPath(QStringLiteral("BaseSAV.sav")));
    [&]{ QVERIFY(fm.openFileRecent(0)); }();
    return liveBytes(fm);
  }

private slots:
  void initTestCase();

  void setData_nullIsIgnored();
  void openGoodRecent_succeeds();
  void openMissingRecent_failsAndLeavesSaveUntouched();
  void openTruncatedRecent_failsAndLeavesSaveUntouched();
  void openOversizedRecent_succeeds();
};

void TestErrors::initTestCase()
{
  // Isolate QSettings (FileManagement persists recents) from the real app's.
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));

  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());

  m_truncated = m_tmp.filePath(QStringLiteral("trunc.sav"));
  { QFile f(m_truncated); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(QByteArray(100, '\0')); }

  m_oversized = m_tmp.filePath(QStringLiteral("over.sav"));
  { QFile f(m_oversized); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(QByteArray(40000, '\x01')); }

  m_missing = m_tmp.filePath(QStringLiteral("does_not_exist.sav"));
}

void TestErrors::setData_nullIsIgnored()
{
  const QByteArray orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(orig.size(), kSaveSize);

  SaveFile sf;
  loadInto(sf, orig);
  const QByteArray before = snapshot(sf);

  sf.setData(nullptr); // must be a safe no-op (guarded since s14), never a crash

  QCOMPARE(snapshot(sf), before);
}

void TestErrors::openGoodRecent_succeeds()
{
  FileManagement fm;
  fm.clearRecentFiles();
  fm.addRecentFile(assetPath(QStringLiteral("BaseSAV.sav")));
  QVERIFY(fm.openFileRecent(0));
}

void TestErrors::openMissingRecent_failsAndLeavesSaveUntouched()
{
  FileManagement fm;
  const QByteArray before = loadGood(fm);

  fm.clearRecentFiles();
  fm.addRecentFile(m_missing);
  QVERIFY2(!fm.openFileRecent(0), "opening a missing file should fail");
  QVERIFY2(!fm.getLastErrorMessage().isEmpty(), "a failed load must surface a message");

  // Graceful degradation: the previously-loaded save is byte-for-byte untouched.
  QCOMPARE(liveBytes(fm), before);
}

void TestErrors::openTruncatedRecent_failsAndLeavesSaveUntouched()
{
  FileManagement fm;
  const QByteArray before = loadGood(fm);

  fm.clearRecentFiles();
  fm.addRecentFile(m_truncated);
  QVERIFY2(!fm.openFileRecent(0), "opening a truncated (<32KB) file should fail");
  QVERIFY2(!fm.getLastErrorMessage().isEmpty(), "a failed load must surface a message");

  QCOMPARE(liveBytes(fm), before);
}

void TestErrors::openOversizedRecent_succeeds()
{
  FileManagement fm;
  fm.clearRecentFiles();
  fm.addRecentFile(m_oversized);

  // Larger-than-32KB files are valid: only the first SAV_DATA_SIZE bytes are read.
  QVERIFY(fm.openFileRecent(0));
  QCOMPARE(fm.data->data[0], var8(0x01)); // we filled the file with 0x01
}

QTEST_GUILESS_MAIN(TestErrors)
#include "tst_errors.moc"
