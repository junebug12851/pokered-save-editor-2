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
 * @file tst_area_npc.cpp
 * @brief The current map's map-global character-state flags (AreaNPC): a value round-trip,
 *        that each field lands at exactly the save byte+bit the disassembly + probe pinned
 *        (notes/reference/npc-character-state.md), and that the model touches ONLY those
 *        bytes (byte-exact fidelity -- a top-tier project value).
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areanpc.h>

using namespace pse_test;

class TestAreaNPC : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  // Flatten a fresh load of the clean save after applying @p setup to its AreaNPC.
  QByteArray flatWith(const std::function<void(AreaNPC*)>& setup)
  {
    SaveFile sf; loadInto(sf, m_orig);
    setup(sf.dataExpanded->area->npc);
    sf.flattenData();
    return snapshot(sf);
  }

private slots:
  void initTestCase();
  void roundTrip();
  void offsetsAndBits();
  void touchesOnlyItsOwnBytes();
};

void TestAreaNPC::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

// Every field survives an expand -> edit -> flatten -> expand cycle.
void TestAreaNPC::roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* n = sf.dataExpanded->area->npc;

  n->initTradeCenterFacing  = true;
  n->npcsDoNotFacePlayer    = true;
  n->initScriptedMovement   = true;
  n->scriptedNpcMoving      = true;
  n->disableJoypad          = true;
  n->scriptedMovementActive = true;
  n->testBattle             = true;
  n->trainerBattle          = true;
  n->trainerHeaderPtr       = 0x5AA5;

  sf.flattenData(); sf.expandData();

  auto* n2 = sf.dataExpanded->area->npc;
  QCOMPARE(n2->initTradeCenterFacing,  true);
  QCOMPARE(n2->npcsDoNotFacePlayer,    true);
  QCOMPARE(n2->initScriptedMovement,   true);
  QCOMPARE(n2->scriptedNpcMoving,      true);
  QCOMPARE(n2->disableJoypad,          true);
  QCOMPARE(n2->scriptedMovementActive, true);
  QCOMPARE(n2->testBattle,             true);
  QCOMPARE(n2->trainerBattle,          true);
  QCOMPARE(n2->trainerHeaderPtr,       0x5AA5);
}

// Each flag writes exactly the byte+bit the reference note pins.
void TestAreaNPC::offsetsAndBits()
{
  const QByteArray base = flatWith([](AreaNPC*){});   // clean flatten, no edit

  auto bitOf = [](const QByteArray& d, int off, int bit) {
    return (static_cast<quint8>(d[off]) >> bit) & 1;
  };

  struct Case { const char* name; int off; int bit; std::function<void(AreaNPC*)> set; };
  const QVector<Case> cases = {
    {"initTradeCenterFacing",  0x29D9, 0, [](AreaNPC* n){ n->initTradeCenterFacing  = true; }},
    {"npcsDoNotFacePlayer",    0x29D9, 5, [](AreaNPC* n){ n->npcsDoNotFacePlayer    = true; }},
    {"initScriptedMovement",   0x29DA, 7, [](AreaNPC* n){ n->initScriptedMovement   = true; }},
    {"scriptedNpcMoving",      0x29DC, 0, [](AreaNPC* n){ n->scriptedNpcMoving      = true; }},
    {"disableJoypad",          0x29DC, 5, [](AreaNPC* n){ n->disableJoypad          = true; }},
    {"scriptedMovementActive", 0x29DC, 7, [](AreaNPC* n){ n->scriptedMovementActive = true; }},
    {"testBattle",             0x29DF, 0, [](AreaNPC* n){ n->testBattle             = true; }},
    {"trainerBattle",          0x29DF, 3, [](AreaNPC* n){ n->trainerBattle          = true; }},
  };

  for(const auto& c : cases) {
    QVERIFY2(bitOf(base, c.off, c.bit) == 0, c.name);       // clean save has it clear
    const QByteArray d = flatWith(c.set);
    QVERIFY2(bitOf(d, c.off, c.bit) == 1, c.name);          // our write set exactly that bit
  }

  // The pointer word lands little-endian at 0x2CDC.
  const QByteArray dp = flatWith([](AreaNPC* n){ n->trainerHeaderPtr = 0x5AA5; });
  QCOMPARE(static_cast<quint8>(dp[0x2CDC]),     quint8(0xA5));
  QCOMPARE(static_cast<quint8>(dp[0x2CDC + 1]), quint8(0x5A));
}

// Byte-exact fidelity: setting one flag changes ONLY its own byte (+ the main-data
// checksum at 0x3523), nothing else in the 32 KB save.
void TestAreaNPC::touchesOnlyItsOwnBytes()
{
  const QByteArray base   = flatWith([](AreaNPC*){});
  const QByteArray edited = flatWith([](AreaNPC* n){ n->trainerBattle = true; });  // 0x29DF b3

  const QVector<int> diffs = diffOffsets(base, edited);
  QVERIFY2(diffs.contains(0x29DF), "the flag's own byte must change");
  for(int off : diffs)
    QVERIFY2(off == 0x29DF || off >= 0x3523,   // 0x3523 = main-data checksum (and beyond)
             qPrintable(QStringLiteral("unexpected byte change at 0x%1").arg(off, 0, 16)));
}

QTEST_GUILESS_MAIN(TestAreaNPC)
#include "tst_area_npc.moc"
