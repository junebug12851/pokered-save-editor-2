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
 * @file tst_area_fragments.cpp
 * @brief The current map's variable-length child lists -- warps, signs, sprites,
 *        and the tileset's talk-over tiles -- exercised through their add/at/swap/
 *        remove APIs, with byte round-trips for the scalar fragments (warps, signs).
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/area/areasign.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/fragments/warpdata.h>
#include <pse-savefile/expanded/fragments/signdata.h>

using namespace pse_test;

class TestAreaFragments : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();
  void warps_listOpsAndRoundTrip();
  void signs_listOpsAndRoundTrip();
  void sprites_structuralListOps();
  void tileset_talkOverTiles();
};

void TestAreaFragments::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestAreaFragments::warps_listOpsAndRoundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* w = sf.dataExpanded->area->warps;

  while(w->warpCount() < 2 && w->warpCount() < w->warpMax()) w->warpNew();
  QVERIFY(w->warpCount() >= 2);

  w->warpAt(0)->x = 5; w->warpAt(0)->y = 6; w->warpAt(0)->destWarp = 1; w->warpAt(0)->destMap = 40;
  w->warpAt(1)->x = 7; w->warpAt(1)->y = 8; w->warpAt(1)->destWarp = 2; w->warpAt(1)->destMap = 12;
  const int count = w->warpCount();

  sf.flattenData(); sf.expandData();

  auto* w2 = sf.dataExpanded->area->warps;
  QCOMPARE(w2->warpCount(), count);
  QCOMPARE(w2->warpAt(0)->x, 5);
  QCOMPARE(w2->warpAt(0)->destMap, 40);
  QCOMPARE(w2->warpAt(1)->y, 8);
  QCOMPARE(w2->warpAt(1)->destWarp, 2);

  // Swap the two and confirm the fields followed.
  w2->warpSwap(0, 1);
  QCOMPARE(w2->warpAt(0)->x, 7);
  QCOMPARE(w2->warpAt(1)->x, 5);

  // Remove the last warp shrinks the list by one. (warpRemove is a QML-contract
  // method -- it does an unchecked .at(ind), so callers pass only valid indices.)
  w2->warpRemove(w2->warpCount() - 1);
  QCOMPARE(w2->warpCount(), count - 1);
}

void TestAreaFragments::signs_listOpsAndRoundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* s = sf.dataExpanded->area->signs;

  while(s->signCount() < 2 && s->signCount() < s->signMax()) s->signNew();
  QVERIFY(s->signCount() >= 2);

  s->signAt(0)->x = 3; s->signAt(0)->y = 4; s->signAt(0)->txtId = 9;
  s->signAt(1)->x = 11; s->signAt(1)->y = 12; s->signAt(1)->txtId = 5;
  const int count = s->signCount();

  sf.flattenData(); sf.expandData();

  auto* s2 = sf.dataExpanded->area->signs;
  QCOMPARE(s2->signCount(), count);
  QCOMPARE(s2->signAt(0)->x, 3);
  QCOMPARE(s2->signAt(0)->txtId, 9);
  QCOMPARE(s2->signAt(1)->y, 12);

  s2->signSwap(0, 1);
  QCOMPARE(s2->signAt(0)->txtId, 5);
  QCOMPARE(s2->signAt(1)->txtId, 9);

  s2->signRemove(s2->signCount() - 1);
  QCOMPARE(s2->signCount(), count - 1);
}

void TestAreaFragments::sprites_structuralListOps()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* sp = sf.dataExpanded->area->sprites;

  QVERIFY(sp->spriteMax() > 0);
  const int start = sp->spriteCount();
  QVERIFY(start >= 0);
  QVERIFY(start <= sp->spriteMax());

  for(int i = 0; i < start; i++)
    QVERIFY2(sp->spriteAt(i) != nullptr, qPrintable(QStringLiteral("sprite %1 null").arg(i)));

  if(start < sp->spriteMax()) {
    sp->spriteNew();
    QCOMPARE(sp->spriteCount(), start + 1);
    if(sp->spriteCount() >= 2)
      sp->spriteSwap(0, 1); // must not crash
    sp->spriteRemove(sp->spriteCount() - 1);
    QCOMPARE(sp->spriteCount(), start);
  }
}

void TestAreaFragments::tileset_talkOverTiles()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* t = sf.dataExpanded->area->tileset;

  const int n = t->talkingOverTilesCount();
  QVERIFY(n >= 0);
  for(int i = 0; i < n; i++)
    (void)t->talkingOverTilesAt(i); // read each; must not crash
  if(n >= 2)
    t->talkingOverTilesSwap(0, 1); // reorder; must not crash
}

QTEST_GUILESS_MAIN(TestAreaFragments)
#include "tst_area_fragments.moc"
