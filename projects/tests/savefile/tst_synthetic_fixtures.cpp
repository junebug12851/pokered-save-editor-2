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
 * @file tst_synthetic_fixtures.cpp
 * @brief Matrix coverage over the committed synthetic saves in `assets/saves/synthetic-clean/`
 *        (see that folder's README + tools/gen_synthetic_fixtures.cpp). Each is an
 *        engine-generated edge-case save; this proves they all (1) load + expand
 *        without crashing, (2) round-trip byte-stable (open -> flatten -> recalc ==
 *        the bytes on disk -- the byte-fidelity contract on a non-default save), and
 *        (3) carry the exact field values they were built with.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

using namespace pse_test;

class TestSyntheticFixtures : public QObject
{
  Q_OBJECT

  static bool open(FileManagement& fm, const QString& name)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(assetPath(QStringLiteral("saves/synthetic-clean/") + name));
    return fm.openFileRecent(0);
  }

private slots:
  void initTestCase();
  void loadsAndRoundTripsByteStable_data();
  void loadsAndRoundTripsByteStable();
  void carryTheirEditedValues();
};

void TestSyntheticFixtures::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY2(QFile::exists(assetPath(QStringLiteral("saves/synthetic-clean/new_maxed.sav"))),
           "assets/saves/synthetic-clean/ fixtures missing -- build + run gen_synthetic_fixtures");
}

// Every synthetic save loads, and reopening + reflattening reproduces its on-disk
// image byte-for-byte (the fidelity contract on a non-default, edited save).
void TestSyntheticFixtures::loadsAndRoundTripsByteStable_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("maxed")     << QStringLiteral("new_maxed.sav");
  QTest::newRow("zeroed")    << QStringLiteral("new_zeroed.sav");
  QTest::newRow("allbadges") << QStringLiteral("new_allbadges.sav");
  QTest::newRow("midgame")   << QStringLiteral("new_midgame.sav");
}

void TestSyntheticFixtures::loadsAndRoundTripsByteStable()
{
  QFETCH(QString, file);

  const QByteArray onDisk = readSaveBytes(QStringLiteral("saves/synthetic-clean/") + file);
  QCOMPARE(onDisk.size(), kSaveSize);

  FileManagement fm;
  QVERIFY2(open(fm, file), qPrintable(file + " failed to load"));
  QVERIFY(fm.data->dataExpanded != nullptr);

  fm.data->flattenData();
  fm.data->toolset->recalcChecksums();
  const QByteArray reflattened = snapshot(*fm.data);

  QCOMPARE(reflattened, onDisk);   // byte-for-byte stable
}

// The saves carry exactly the values the generator wrote.
void TestSyntheticFixtures::carryTheirEditedValues()
{
  {
    FileManagement fm; QVERIFY(open(fm, QStringLiteral("new_maxed.sav")));
    PlayerBasics* b = fm.data->dataExpanded->player->basics;
    QCOMPARE(b->money, 999999u);
    QCOMPARE(b->coins, 9999);
    QCOMPARE(b->badgeCount(), int(maxBadges));
  }
  {
    FileManagement fm; QVERIFY(open(fm, QStringLiteral("new_zeroed.sav")));
    PlayerBasics* b = fm.data->dataExpanded->player->basics;
    QCOMPARE(b->money, 0u);
    QCOMPARE(b->coins, 0);
    QCOMPARE(b->badgeCount(), 0);
  }
  {
    FileManagement fm; QVERIFY(open(fm, QStringLiteral("new_allbadges.sav")));
    QCOMPARE(fm.data->dataExpanded->player->basics->badgeCount(), int(maxBadges));
  }
  {
    FileManagement fm; QVERIFY(open(fm, QStringLiteral("new_midgame.sav")));
    PlayerBasics* b = fm.data->dataExpanded->player->basics;
    QCOMPARE(b->money, 123456u);
    QCOMPARE(b->coins, 4321);
    QCOMPARE(b->badgeCount(), 4);
  }
}

QTEST_GUILESS_MAIN(TestSyntheticFixtures)
#include "tst_synthetic_fixtures.moc"
