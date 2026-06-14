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
 * @file tst_fields.cpp
 * @brief Phase-2 per-field expand/flatten coverage.
 *
 * Two complementary styles per field:
 *  - VALUE round-trip: set the value on the expanded model, flatten, re-expand,
 *    and assert the value survives the raw encoding (incl. boundary values). No
 *    offsets needed -- proves load<->save fidelity for that field.
 *  - BYTE isolation: edit one field, flatten+recalc, and assert ONLY that field's
 *    bytes (plus the main checksum 0x3523) changed -- proves an edit doesn't write
 *    bytes it wasn't told to (the project's prime value).
 *
 * Field offsets are the ones the engine itself documents in playerbasics.cpp,
 * cross-checked against assets/saves/structure.bt:
 *   name 0x2598 (11B), ID 0x2605 (word), money 0x25F3 (BCD3), coins 0x2850 (BCD2),
 *   starter 0x29C3 (byte), badges 0x2602 AND its duplicate 0x29D6.
 */

#include <QtTest>
#include <QByteArray>
#include <QSet>
#include <QVector>
#include <functional>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/player/playerpokedex.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/rival.h>

using namespace pse_test;

namespace {
constexpr int kMainChecksum = 0x3523;

// Edit one field and assert only the allowed offsets (plus the main checksum)
// changed between two flatten+recalc snapshots.
void expectOnlyChanged(SaveFile& sf, QSet<int> allowed, const std::function<void()>& edit)
{
  sf.flattenData();
  sf.toolset->recalcChecksums();
  const QByteArray base = snapshot(sf);

  edit();

  sf.flattenData();
  sf.toolset->recalcChecksums();
  const QByteArray after = snapshot(sf);

  allowed.insert(kMainChecksum); // always allowed to move when bank-1 data changes

  const QVector<int> diffs = diffOffsets(base, after);
  for(int off : diffs)
    QVERIFY2(allowed.contains(off),
             qPrintable(QStringLiteral("edit touched unexpected offset 0x%1 (changed %2 byte(s) total)")
                          .arg(off, 0, 16).arg(diffs.size())));
}
} // namespace

class TestFields : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig; ///< The default fixture (progressed save) bytes.

private slots:
  void initTestCase();

  // ---- value round-trips ----
  void money_roundTrip_data();
  void money_roundTrip();
  void coins_roundTrip_data();
  void coins_roundTrip();
  void playerId_roundTrip_data();
  void playerId_roundTrip();
  void playerStarter_roundTrip_data();
  void playerStarter_roundTrip();
  void playerName_roundTrip_data();
  void playerName_roundTrip();
  void badges_roundTrip();
  void pokedex_roundTrip();
  void rival_roundTrip();
  void bagItem_roundTrip();

  // ---- byte isolation ----
  void money_isolation();
  void coins_isolation();
  void playerId_isolation();
  void playerStarter_isolation();
  void badges_isolation();
  void playerName_isolation();
};

void TestFields::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

// ---------------------------------------------------------------- money
void TestFields::money_roundTrip_data()
{
  QTest::addColumn<uint>("v");
  QTest::newRow("zero")    << 0u;
  QTest::newRow("one")     << 1u;
  QTest::newRow("typical") << 12345u;
  QTest::newRow("max")     << 999999u; // 24-bit BCD ceiling
}
void TestFields::money_roundTrip()
{
  QFETCH(uint, v);
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->player->basics->money = v;
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->money, v);
}

// ---------------------------------------------------------------- coins
void TestFields::coins_roundTrip_data()
{
  QTest::addColumn<int>("v");
  QTest::newRow("zero") << 0;
  QTest::newRow("one")  << 1;
  QTest::newRow("max")  << 9999; // 16-bit BCD ceiling
}
void TestFields::coins_roundTrip()
{
  QFETCH(int, v);
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->player->basics->coins = v;
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->coins, v);
}

// ---------------------------------------------------------------- player ID
void TestFields::playerId_roundTrip_data()
{
  QTest::addColumn<int>("v");
  QTest::newRow("zero")    << 0;
  QTest::newRow("one")     << 1;
  QTest::newRow("typical") << 0x1234;
  QTest::newRow("max")     << 0xFFFF; // 16-bit
}
void TestFields::playerId_roundTrip()
{
  QFETCH(int, v);
  SaveFile sf; loadInto(sf, m_orig);
  // Set the member directly (not fullSetPlayerId) so we round-trip just the ID
  // field without the intentional OT-rewrite of owned mons.
  sf.dataExpanded->player->basics->playerID = v;
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->getPlayerId(), v);
}

// ---------------------------------------------------------------- starter
void TestFields::playerStarter_roundTrip_data()
{
  QTest::addColumn<int>("v");
  QTest::newRow("zero") << 0;
  QTest::newRow("low")  << 1;
  QTest::newRow("mid")  << 153;
  QTest::newRow("max")  << 255; // single raw byte
}
void TestFields::playerStarter_roundTrip()
{
  QFETCH(int, v);
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->player->basics->playerStarter = v;
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->playerStarter, v);
}

// ---------------------------------------------------------------- player name
void TestFields::playerName_roundTrip_data()
{
  QTest::addColumn<QString>("v");
  QTest::newRow("empty")    << QString("");
  QTest::newRow("one")      << QString("A");
  QTest::newRow("typical")  << QString("RED");
  QTest::newRow("maxlen7")  << QString("ABCDEFG"); // Gen 1 name cap is 7
}
void TestFields::playerName_roundTrip()
{
  QFETCH(QString, v);
  SaveFile sf; loadInto(sf, m_orig);
  // Member set (not fullSetPlayerName) -> just the name field, no OT rewrite.
  sf.dataExpanded->player->basics->playerName = v;
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->player->basics->getPlayerName(), v);
}

// ---------------------------------------------------------------- badges
void TestFields::badges_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;

  // A distinctive pattern across all 8 badge bits.
  const bool pattern[8] = {true, false, true, true, false, false, true, false};
  for(int i = 0; i < 8; i++) b->badgeSet(i, pattern[i]);

  sf.flattenData(); sf.expandData();
  auto* b2 = sf.dataExpanded->player->basics;
  for(int i = 0; i < 8; i++)
    QVERIFY2(b2->badgeAt(i) == pattern[i],
             qPrintable(QStringLiteral("badge %1 did not round-trip").arg(i)));

  // All on, then all off.
  for(int i = 0; i < 8; i++) b2->badgeSet(i, true);
  sf.flattenData(); sf.expandData();
  for(int i = 0; i < 8; i++) QVERIFY(sf.dataExpanded->player->basics->badgeAt(i));

  for(int i = 0; i < 8; i++) sf.dataExpanded->player->basics->badgeSet(i, false);
  sf.flattenData(); sf.expandData();
  for(int i = 0; i < 8; i++) QVERIFY(!sf.dataExpanded->player->basics->badgeAt(i));
}

// ---------------------------------------------------------------- pokedex
void TestFields::pokedex_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* dex = sf.dataExpanded->player->pokedex;

  // Independent patterns for owned vs seen across all 151 species.
  for(int i = 0; i < maxPokedex; i++) {
    dex->ownedSet(i, (i % 5) == 0);
    dex->seenSet(i,  (i % 3) == 0);
  }

  sf.flattenData(); sf.expandData();
  auto* dex2 = sf.dataExpanded->player->pokedex;
  for(int i = 0; i < maxPokedex; i++) {
    QVERIFY2(dex2->ownedAt(i) == ((i % 5) == 0),
             qPrintable(QStringLiteral("owned[%1] did not round-trip").arg(i)));
    QVERIFY2(dex2->seenAt(i) == ((i % 3) == 0),
             qPrintable(QStringLiteral("seen[%1] did not round-trip").arg(i)));
  }
}

// ---------------------------------------------------------------- rival
void TestFields::rival_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  sf.dataExpanded->rival->name = QString("BLUE");
  sf.dataExpanded->rival->starter = 177; // raw index value; round-trip only
  sf.flattenData(); sf.expandData();
  QCOMPARE(sf.dataExpanded->rival->name, QString("BLUE"));
  QCOMPARE(sf.dataExpanded->rival->starter, 177);
}

// ---------------------------------------------------------------- bag item
void TestFields::bagItem_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* bag = sf.dataExpanded->player->items;

  if(bag->itemsCount() == 0)
    bag->itemNew(); // ensure at least one slot to edit

  Item* slot = bag->itemAt(0);
  QVERIFY(slot != nullptr);
  slot->ind = 20;        // some item index
  slot->setAmount(42);   // within the Gen 1 cap of 99

  sf.flattenData(); sf.expandData();

  auto* bag2 = sf.dataExpanded->player->items;
  QVERIFY(bag2->itemsCount() >= 1);
  Item* r = bag2->itemAt(0);
  QVERIFY(r != nullptr);
  QCOMPARE(r->ind, 20);
  QCOMPARE(r->getAmount(), 42);
}

// ================================================================ isolation
void TestFields::money_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  expectOnlyChanged(sf, {0x25F3, 0x25F4, 0x25F5}, [&]{
    b->money = (b->money == 123456u) ? 654321u : 123456u;
  });
}
void TestFields::coins_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  expectOnlyChanged(sf, {0x2850, 0x2851}, [&]{
    b->coins = (b->coins == 4242) ? 1337 : 4242;
  });
}
void TestFields::playerId_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  expectOnlyChanged(sf, {0x2605, 0x2606}, [&]{
    b->playerID = (b->playerID == 0x1234) ? 0x4321 : 0x1234;
  });
}
void TestFields::playerStarter_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  expectOnlyChanged(sf, {0x29C3}, [&]{
    b->playerStarter = (b->playerStarter == 153) ? 99 : 153;
  });
}
void TestFields::badges_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  // Badges are written to BOTH the primary byte and its in-game duplicate.
  expectOnlyChanged(sf, {0x2602, 0x29D6}, [&]{
    for(int i = 0; i < 8; i++) b->badgeSet(i, !b->badgeAt(i));
  });
}
void TestFields::playerName_isolation()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* b = sf.dataExpanded->player->basics;
  // Member set (no OT rewrite); the name occupies 0x2598..0x25A2 (11 bytes).
  QSet<int> nameBytes;
  for(int o = 0x2598; o <= 0x25A2; o++) nameBytes.insert(o);
  expectOnlyChanged(sf, nameBytes, [&]{
    b->playerName = (b->playerName == QStringLiteral("TESTN")) ? QStringLiteral("ABCDEFG")
                                                              : QStringLiteral("TESTN");
  });
}

QTEST_GUILESS_MAIN(TestFields)
#include "tst_fields.moc"
