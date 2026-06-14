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
 * @file tst_area.cpp
 * @brief Expand/flatten round-trips for the current-area state tree and the PC
 *        storage region -- the two big expanded sub-trees not yet covered by the
 *        savefile field tests. Each field is set on the model, flushed through
 *        flatten -> re-expand, and read back, proving the area's eleven facets
 *        (audio/general/player/map/tileset/warps) and the Storage container all
 *        survive the byte round-trip.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areaaudio.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>

using namespace pse_test;

class TestArea : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();

  void areaGeneral_roundTrip();
  void areaAudio_roundTrip();
  void areaPlayer_roundTrip();
  void areaMap_roundTrip();
  void areaTileset_roundTrip();
  void areaWarps_flags_roundTrip();
  void storage_curBoxAndFormatting_roundTrip();
  void storage_allTwelveBoxesResolve();
};

void TestArea::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestArea::areaGeneral_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* g = sf.dataExpanded->area->general;
  g->contrast = 7;            // a valid step
  g->noLetterDelay = true;
  g->countPlaytime = false;

  sf.flattenData(); sf.expandData();

  auto* g2 = sf.dataExpanded->area->general;
  QCOMPARE(g2->contrast, 7);
  QCOMPARE(g2->noLetterDelay, true);
  QCOMPARE(g2->countPlaytime, false);
}

void TestArea::areaAudio_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* a = sf.dataExpanded->area->audio;
  a->musicID = 0x1F;
  a->musicBank = 2;
  a->noAudioFadeout = true;
  a->preventMusicChange = false;

  sf.flattenData(); sf.expandData();

  auto* a2 = sf.dataExpanded->area->audio;
  QCOMPARE(a2->musicID, 0x1F);
  QCOMPARE(a2->musicBank, 2);
  QCOMPARE(a2->noAudioFadeout, true);
  QCOMPARE(a2->preventMusicChange, false);
}

void TestArea::areaPlayer_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* p = sf.dataExpanded->area->player;
  p->yCoord = 12; p->xCoord = 7;
  p->yBlockCoord = 6; p->xBlockCoord = 3;
  p->walkBikeSurf = 2;          // surfing
  p->surfingAllowed = true;
  p->flyOutofBattle = false;
  p->isBattle = false;
  p->standingOnWarp = true;
  p->usingLinkCable = false;

  sf.flattenData(); sf.expandData();

  auto* p2 = sf.dataExpanded->area->player;
  QCOMPARE(p2->yCoord, 12);
  QCOMPARE(p2->xCoord, 7);
  QCOMPARE(p2->yBlockCoord, 6);
  QCOMPARE(p2->xBlockCoord, 3);
  QCOMPARE(p2->walkBikeSurf, 2);
  QCOMPARE(p2->surfingAllowed, true);
  QCOMPARE(p2->flyOutofBattle, false);
  QCOMPARE(p2->isBattle, false);
  QCOMPARE(p2->standingOnWarp, true);
  QCOMPARE(p2->usingLinkCable, false);
}

void TestArea::areaMap_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* m = sf.dataExpanded->area->map;
  m->curMap = 40;
  m->height = 9; m->width = 10;
  m->height2x2 = 18; m->width2x2 = 20;
  m->dataPtr = 0x1234;          // 16-bit GB pointers
  m->txtPtr = 0x5678;
  m->scriptPtr = 0x4321;
  m->curMapScript = 5;
  m->cardKeyDoorX = 4; m->cardKeyDoorY = 6;
  m->forceBikeRide = true;
  m->blackoutDest = false;

  sf.flattenData(); sf.expandData();

  auto* m2 = sf.dataExpanded->area->map;
  QCOMPARE(m2->curMap, 40);
  QCOMPARE(m2->height, 9);
  QCOMPARE(m2->width, 10);
  QCOMPARE(m2->height2x2, 18);
  QCOMPARE(m2->width2x2, 20);
  QCOMPARE(m2->dataPtr, 0x1234);
  QCOMPARE(m2->txtPtr, 0x5678);
  QCOMPARE(m2->scriptPtr, 0x4321);
  QCOMPARE(m2->curMapScript, 5);
  QCOMPARE(m2->cardKeyDoorX, 4);
  QCOMPARE(m2->cardKeyDoorY, 6);
  QCOMPARE(m2->forceBikeRide, true);
  QCOMPARE(m2->blackoutDest, false);
}

void TestArea::areaTileset_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* t = sf.dataExpanded->area->tileset;
  t->current = 3;
  t->grassTile = 0x52;
  t->boulderIndex = 1;
  t->boulderColl = 0x0A;
  t->type = 1;
  t->bank = 4;
  t->blockPtr = 0x4000;
  t->gfxPtr = 0x4800;
  t->collPtr = 0x6000;

  sf.flattenData(); sf.expandData();

  auto* t2 = sf.dataExpanded->area->tileset;
  QCOMPARE(t2->current, 3);
  QCOMPARE(t2->grassTile, 0x52);
  QCOMPARE(t2->boulderIndex, 1);
  QCOMPARE(t2->boulderColl, 0x0A);
  QCOMPARE(t2->type, 1);
  QCOMPARE(t2->bank, 4);
  QCOMPARE(t2->blockPtr, 0x4000);
  QCOMPARE(t2->gfxPtr, 0x4800);
  QCOMPARE(t2->collPtr, 0x6000);
}

void TestArea::areaWarps_flags_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* w = sf.dataExpanded->area->warps;
  w->scriptedWarp = true;
  w->isDungeonWarp = false;
  w->warpDest = 0x03;
  w->dungeonWarpDestMap = 0x10;
  w->specialWarpDestMap = 0x20;
  w->whichDungeonWarp = 1;
  w->warpedFromWarp = 2;
  w->warpedfromMap = 40;

  sf.flattenData(); sf.expandData();

  auto* w2 = sf.dataExpanded->area->warps;
  QCOMPARE(w2->scriptedWarp, true);
  QCOMPARE(w2->isDungeonWarp, false);
  QCOMPARE(w2->warpDest, 0x03);
  QCOMPARE(w2->dungeonWarpDestMap, 0x10);
  QCOMPARE(w2->specialWarpDestMap, 0x20);
  QCOMPARE(w2->whichDungeonWarp, 1);
  QCOMPARE(w2->warpedFromWarp, 2);
  QCOMPARE(w2->warpedfromMap, 40);
}

void TestArea::storage_curBoxAndFormatting_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* s = sf.dataExpanded->storage;
  QCOMPARE(s->boxCount(), 12); // two sets x six boxes
  s->curBox = 9;
  s->boxesFormatted = true;

  sf.flattenData(); sf.expandData();

  auto* s2 = sf.dataExpanded->storage;
  QCOMPARE(s2->curBox, 9);
  QCOMPARE(s2->boxesFormatted, true);
}

void TestArea::storage_allTwelveBoxesResolve()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* s = sf.dataExpanded->storage;
  // boxAt() must map the flattened 0..11 index across both sets without gaps.
  for(int i = 0; i < s->boxCount(); i++)
    QVERIFY2(s->boxAt(i) != nullptr,
             qPrintable(QStringLiteral("PC box %1 did not resolve").arg(i)));
}

QTEST_GUILESS_MAIN(TestArea)
#include "tst_area.moc"
