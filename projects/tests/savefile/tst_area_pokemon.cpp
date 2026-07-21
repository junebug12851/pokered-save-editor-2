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
 * @file tst_area_pokemon.cpp
 * @brief The current map's wild-encounter tables (AreaPokemon: grass/water rate +
 *        two 10-slot index/level lists) and the loaded-sprite slots
 *        (AreaLoadedSprites), with value round-trips and the swap/list accessors.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areapokemon.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>

using namespace pse_test;

class TestAreaPokemon : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();
  void wildTables_roundTrip();
  void wildTables_byteOrderIsLevelThenSpecies();
  void wildTables_swap();
  void loadedSprites_roundTripAndSwap();
};

void TestAreaPokemon::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestAreaPokemon::wildTables_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* p = sf.dataExpanded->area->pokemon;

  QCOMPARE(p->grassMonsCount(), 10);
  QCOMPARE(p->waterMonsCount(), 10);

  p->grassRate = 25; p->waterRate = 10; p->wildEncounterCooldown = true;
  for(int i = 0; i < p->grassMonsCount(); i++) {
    p->grassMonsAt(i)->index = i + 1;        p->grassMonsAt(i)->level = (i + 1) * 3;
    p->waterMonsAt(i)->index = (i + 1) + 50; p->waterMonsAt(i)->level = (i + 1) * 2;
  }

  sf.flattenData(); sf.expandData();

  auto* p2 = sf.dataExpanded->area->pokemon;
  QCOMPARE(p2->grassRate, 25);
  QCOMPARE(p2->waterRate, 10);
  QCOMPARE(p2->wildEncounterCooldown, true);
  for(int i = 0; i < p2->grassMonsCount(); i++) {
    QCOMPARE(p2->grassMonsAt(i)->index, i + 1);
    QCOMPARE(p2->grassMonsAt(i)->level, (i + 1) * 3);
    QCOMPARE(p2->waterMonsAt(i)->index, (i + 1) + 50);
    QCOMPARE(p2->waterMonsAt(i)->level, (i + 1) * 2);
  }
}

// The cartridge stores each wild slot as `db level, species` (pokered
// data/wild/maps/*.asm; BaseSAV 0x2B35 = 165 = RATTATA). This pins that byte order
// so a re-inversion (species-first, as the model did through 0.39.x) fails here.
// See notes/reference/wild-encounters.md.
void TestAreaPokemon::wildTables_byteOrderIsLevelThenSpecies()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* p = sf.dataExpanded->area->pokemon;

  // Enable grass (rate > 0) so the 20 slot bytes are actually written, then set
  // slot 0 to a recognisable pair: level 7, species 165 (RATTATA internal index).
  p->grassRate = 25;
  p->grassMonsAt(0)->index = 165; // species
  p->grassMonsAt(0)->level = 7;   // level

  sf.flattenData();
  const QByteArray out = snapshot(sf);

  QCOMPARE(int(static_cast<unsigned char>(out.at(0x2B33))), 25);  // grass rate
  QCOMPARE(int(static_cast<unsigned char>(out.at(0x2B34))), 7);   // LEVEL first
  QCOMPARE(int(static_cast<unsigned char>(out.at(0x2B35))), 165); // SPECIES second
}

void TestAreaPokemon::wildTables_swap()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* p = sf.dataExpanded->area->pokemon;
  p->grassMonsAt(0)->index = 3;
  p->grassMonsAt(1)->index = 9;

  p->grassMonsSwap(0, 1);
  QCOMPARE(p->grassMonsAt(0)->index, 9);
  QCOMPARE(p->grassMonsAt(1)->index, 3);
}

void TestAreaPokemon::loadedSprites_roundTripAndSwap()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* ls = sf.dataExpanded->area->preloadedSprites;

  QVERIFY(ls->lSpriteCount() > 0);
  ls->loadedSetId = 7;
  for(int i = 0; i < ls->lSpriteCount(); i++)
    ls->loadedSprites[i] = (i * 2) & 0xFF;

  sf.flattenData(); sf.expandData();

  auto* ls2 = sf.dataExpanded->area->preloadedSprites;
  QCOMPARE(ls2->loadedSetId, 7);
  for(int i = 0; i < ls2->lSpriteCount(); i++)
    QCOMPARE(ls2->lSpriteAt(i), (i * 2) & 0xFF);

  // Swap two slots and confirm the values followed.
  const int a = ls2->lSpriteAt(0), b = ls2->lSpriteAt(1);
  ls2->lSpriteSwap(0, 1);
  QCOMPARE(ls2->lSpriteAt(0), b);
  QCOMPARE(ls2->lSpriteAt(1), a);
}

QTEST_GUILESS_MAIN(TestAreaPokemon)
#include "tst_area_pokemon.moc"
