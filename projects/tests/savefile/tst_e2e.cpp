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
 * @file tst_e2e.cpp
 * @brief Phase-6 integration / end-to-end: the full user journey through real disk
 *        I/O. Open a save, edit many fields across regions, save() to a NEW file on
 *        disk (flatten + recalcChecksums + write), reopen it in a fresh
 *        FileManagement, and assert every edit persisted -- and that the written
 *        file is exactly 32 KB with a game-valid main checksum.
 *
 * This exercises FileManagement <-> SaveFile <-> SaveFileExpanded <-> db together,
 * the way the app actually saves.
 */

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldother.h>

using namespace pse_test;

class TestE2E : public QObject
{
  Q_OBJECT

private:
  QTemporaryDir m_tmp;

  void loadGood(FileManagement& fm)
  {
    fm.clearRecentFiles();
    fm.addRecentFile(assetPath(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
    QVERIFY(fm.openFileRecent(0));
  }

  // Gen 1 main-data checksum over 0x2598..0x3522, stored at 0x3523.
  static quint8 mainChecksum(const QByteArray& d)
  {
    quint8 cs = 0xFF;
    for(int i = 0x2598; i < 0x3523; i++)
      cs = quint8(cs - quint8(d[i]));
    return cs;
  }

private slots:
  void initTestCase();
  void editSaveReopen_basicsPersistAndChecksumValid();
  void editSaveReopen_partyMonPersists();
};

void TestE2E::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(m_tmp.isValid());
}

void TestE2E::editSaveReopen_basicsPersistAndChecksumValid()
{
  FileManagement fm;
  loadGood(fm);

  // Edit across several regions.
  PlayerBasics* b = fm.data->dataExpanded->player->basics;
  b->money = 314159;
  b->coins = 7777;
  b->playerID = 0xBEEF;
  b->playerName = QStringLiteral("TESTER"); // member set: just the name field
  for(int i = 0; i < 8; i++) b->badgeSet(i, (i % 2) == 0);
  fm.data->dataExpanded->world->other->playtime->hours = 99;

  // Save to a NEW file (never the fixture).
  const QString out = m_tmp.filePath(QStringLiteral("edited.sav"));
  fm.setPath(out);
  QVERIFY(fm.saveFile());

  // The written file is exactly 32 KB with a valid main checksum.
  QFile f(out);
  QVERIFY(f.open(QIODevice::ReadOnly));
  const QByteArray disk = f.read(kSaveSize + 16);
  f.close();
  QCOMPARE(disk.size(), kSaveSize);
  QCOMPARE(quint8(disk[0x3523]), mainChecksum(disk));

  // Reopen in a fresh manager; every edit survived the disk round-trip.
  FileManagement fm2;
  fm2.clearRecentFiles();
  fm2.addRecentFile(out);
  QVERIFY(fm2.openFileRecent(0));

  PlayerBasics* b2 = fm2.data->dataExpanded->player->basics;
  QCOMPARE(b2->money, 314159u);
  QCOMPARE(b2->coins, 7777);
  QCOMPARE(b2->getPlayerId(), 0xBEEF);
  QCOMPARE(b2->getPlayerName(), QStringLiteral("TESTER"));
  for(int i = 0; i < 8; i++)
    QCOMPARE(b2->badgeAt(i), (i % 2) == 0);
  QCOMPARE(fm2.data->dataExpanded->world->other->playtime->hours, 99);
}

void TestE2E::editSaveReopen_partyMonPersists()
{
  FileManagement fm;
  loadGood(fm);

  PlayerPokemon* party = fm.data->dataExpanded->player->pokemon;
  if(party->pokemonCount() == 0)
    QSKIP("fixture has an empty party");

  PokemonParty* p = party->partyAt(0);
  p->species = 151;
  p->level = 77;
  p->dvSet(0, 15);
  p->movesAt(0)->moveID = 1;
  p->movesAt(0)->pp = 5; // within Pound's PP cap
  p->nickname = QStringLiteral("DISK");

  const QString out = m_tmp.filePath(QStringLiteral("party.sav"));
  fm.setPath(out);
  QVERIFY(fm.saveFile());

  FileManagement fm2;
  fm2.clearRecentFiles();
  fm2.addRecentFile(out);
  QVERIFY(fm2.openFileRecent(0));

  PokemonParty* p2 = fm2.data->dataExpanded->player->pokemon->partyAt(0);
  QVERIFY(p2 != nullptr);
  QCOMPARE(p2->species, 151);
  QCOMPARE(p2->level, 77);
  QCOMPARE(p2->dvAt(0), 15);
  QCOMPARE(p2->movesAt(0)->moveID, 1);
  QCOMPARE(p2->movesAt(0)->pp, 5);
  QCOMPARE(p2->nickname, QStringLiteral("DISK"));
}

QTEST_GUILESS_MAIN(TestE2E)
#include "tst_e2e.moc"
