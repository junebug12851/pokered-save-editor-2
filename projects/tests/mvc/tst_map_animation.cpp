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
 * @file tst_map_animation.cpp
 * @brief The map ANIMATES, and it animates the way the cartridge does.
 *
 * Every number here is read out of `UpdateMovingBgTiles` (`home/vcopy.asm`) -- see
 * notes/reference/map-animation.md. What the console does, every frame:
 *
 *   - nothing for 19 frames;
 *   - on frame 20, **bit-rotate the 16 bytes of water tile `$14`** -- right for four steps, left for
 *     four (`counter2 & 4`). Water has no frames; it has a ROTATION;
 *   - on frame 21 (water+flower tilesets only), copy one of **three flower tiles** into tile `$03`,
 *     chosen from the WATER's phase counter: `1, 1, 2, 3`;
 *   - the cycle is therefore **20 frames** (water only) or **21** (water + flower).
 *
 * And the hack values are not chaos -- the code tests **bit 0**: an ODD animation byte behaves as
 * water-only, an EVEN non-zero one as water+flower, and **0 breaks Surf**.
 *
 * Both tables were WRONG before this test existed (the water swung 0..+4 instead of -1..+3, and the
 * flower ran 2,3,1,1 instead of 1,1,2,3). They were plausible, they looked fine, and they were an
 * invention. That is exactly what this file is for.
 */
#include <QtTest>
#include <QImage>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>

#include <engine/mapengine.h>
#include <engine/tilesetengine.h>
#include <engine/mapclock.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

class TestMapAnimation : public QObject
{
  Q_OBJECT

  QByteArray m_orig;

  /// The water tile, straight out of the Overworld tileset (tile $14 = column 4, row 1).
  QImage waterTile() const
  {
    const QImage ts = TilesetEngine::getTileset(QStringLiteral("overworld"));
    return ts.copy(4 * TilesetEngine::tileWidth, 1 * TilesetEngine::tileHeight,
                   TilesetEngine::tileWidth, TilesetEngine::tileHeight);
  }

  /// How far right (positive) the tile's rows have been rotated, or -1 if it isn't a pure rotation.
  int rotationOf(const QImage& base, const QImage& moved) const
  {
    for (int shift = 0; shift < TilesetEngine::tileWidth; shift++) {
      bool all = true;

      for (int y = 0; y < TilesetEngine::tileHeight && all; y++) {
        for (int x = 0; x < TilesetEngine::tileWidth; x++) {
          const int from = ((x - shift) % TilesetEngine::tileWidth + TilesetEngine::tileWidth)
                           % TilesetEngine::tileWidth;
          if (moved.pixel(x, y) != base.pixel(from, y)) {
            all = false;
            break;
          }
        }
      }

      if (all)
        return shift;
    }

    return -1;
  }

private slots:
  void initTestCase();

  void water_isARotation_notAFrameSwap();
  void water_followsTheConsolesOffsets();
  void flower_runsOneOneTwoThree();
  void cadence_is20ForWater_21ForFlower_0ForNone();
  void hackValues_behaveTheWayTheConsoleDoes();
  void theFrameIsAnInput_andItNeverTouchesTheSave();
};

void TestMapAnimation::initTestCase()
{
  DB::inst();
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// Every animation step of the water is a pure ROTATION of the tile's rows. Not a different tile.
void TestMapAnimation::water_isARotation_notAFrameSwap()
{
  const QImage base = waterTile();
  QVERIFY2(!base.isNull(), "the overworld tileset did not load");

  for (int frame = 0; frame < 8; frame++) {
    const QImage moved = TilesetEngine::postProcessWave(base, frame);
    QVERIFY2(rotationOf(base, moved) >= 0,
             qPrintable(QStringLiteral("frame %1 is not a rotation of the water tile -- the console "
                                       "rotates the tile's BYTES; it never loads a second tile")
                        .arg(frame)));
  }
}

/**
 * The console's offsets, accumulated from `counter2`:
 *
 *   counter2:  0   1   2   3   4   5   6   7
 *   direction: R   R   R   R   L   L   L   L      (counter2 & 4)
 *   offset:    0  +1  +2  +3  +2  +1   0  -1
 *
 * The old code ran 0,1,2,3,4,3,2,1 -- the right shape, the wrong swing.
 */
void TestMapAnimation::water_followsTheConsolesOffsets()
{
  const QImage base = waterTile();
  const int want[8] = { 0, 1, 2, 3, 2, 1, 0, -1 };

  for (int frame = 0; frame < 8; frame++) {
    const QImage moved = TilesetEngine::postProcessWave(base, frame);

    const int expect = ((want[frame] % TilesetEngine::tileWidth) + TilesetEngine::tileWidth)
                       % TilesetEngine::tileWidth;

    QCOMPARE(rotationOf(base, moved), expect);
  }

  // ...and it closes: frame 8 is frame 0 again.
  QCOMPARE(rotationOf(base, TilesetEngine::postProcessWave(base, 8)), 0);
}

/// `and 3; cp 2` -> 0 and 1 give flower1, 2 gives flower2, 3 gives flower3. Flower1 shows twice.
void TestMapAnimation::flower_runsOneOneTwoThree()
{
  const QImage f0 = TilesetEngine::getFlower(0);
  const QImage f1 = TilesetEngine::getFlower(1);
  const QImage f2 = TilesetEngine::getFlower(2);
  const QImage f3 = TilesetEngine::getFlower(3);

  QVERIFY2(!f0.isNull(), "the flower tiles did not load");

  QVERIFY2(f0 == f1, "frames 0 and 1 must BOTH be flower1 -- flower1 shows for twice as long");
  QVERIFY2(f1 != f2, "frame 2 must be flower2");
  QVERIFY2(f2 != f3, "frame 3 must be flower3");
  QVERIFY2(f0 != f3, "frame 3 must not be flower1");

  // It closes on 4 (and the water closes on 8, so the two are locked in step -- which is exactly
  // why there is no "flower only" state in the game).
  QVERIFY(TilesetEngine::getFlower(4) == f0);
  QVERIFY(TilesetEngine::getFlower(6) == f2);
}

void TestMapAnimation::cadence_is20ForWater_21ForFlower_0ForNone()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* area = sf.dataExpanded->area;

  MapModel map(area->map, area->player, area->tileset, area->general);
  MapClock clock(&map);

  area->tileset->type = 0;                       // TILEANIM_NONE
  emit area->tileset->typeChanged();
  QVERIFY2(!clock.animates(), "an animation byte of 0 animates nothing -- and breaks Surf");
  QCOMPARE(clock.cadence(), 0);

  area->tileset->type = 1;                       // TILEANIM_WATER
  emit area->tileset->typeChanged();
  QVERIFY(clock.animates());
  QCOMPARE(clock.cadence(), 20);

  area->tileset->type = 2;                       // TILEANIM_WATER_FLOWER
  emit area->tileset->typeChanged();
  QVERIFY(clock.animates());
  QCOMPARE(clock.cadence(), 21);
}

/**
 * A byte the game never ships is not garbage: `UpdateMovingBgTiles` tests **bit 0** and nothing
 * else. Odd -> it resets after the water step (water only, 20 frames). Even and non-zero -> it falls
 * through to the flower (21 frames). The editor reproduces the console; it does not "correct" the
 * save.
 */
void TestMapAnimation::hackValues_behaveTheWayTheConsoleDoes()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* area = sf.dataExpanded->area;

  MapModel map(area->map, area->player, area->tileset, area->general);
  MapClock clock(&map);

  const QVector<QPair<int, int>> cases = {
    {  3, 20 }, {  5, 20 }, { 17, 20 }, { 255, 20 },   // odd  -> water only
    {  4, 21 }, {  6, 21 }, { 32, 21 }, { 254, 21 },   // even -> water + flower
  };

  for (const auto& c : cases) {
    area->tileset->type = c.first;
    emit area->tileset->typeChanged();

    QVERIFY2(clock.animates(),
             qPrintable(QStringLiteral("animation byte %1 should still animate").arg(c.first)));
    QCOMPARE(clock.cadence(), c.second);
  }
}

/// The frame is an INPUT to the renderer -- and moving it writes nothing to the save.
void TestMapAnimation::theFrameIsAnInput_andItNeverTouchesTheSave()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* area = sf.dataExpanded->area;

  MapModel map(area->map, area->player, area->tileset, area->general);
  MapClock clock(&map);

  sf.dataExpanded->save(&sf);
  const QByteArray before = snapshot(sf);

  // The URL carries the frame, so the image is re-fetched -- and it is the ONLY thing that changes.
  const QString still = map.source();
  QVERIFY(!still.isEmpty());

  for (int i = 0; i < 20; i++)
    clock.step();

  QVERIFY2(map.source() != still || map.frame() == 0,
           "advancing the clock did not change the rendered image's URL");

  clock.reset();
  QCOMPARE(map.frame(), 0);
  QCOMPARE(map.source(), still);   // frame 0 is the still map, exactly as it was

  sf.dataExpanded->save(&sf);
  QCOMPARE(snapshot(sf), before);  // ...and the save never moved
}

QTEST_MAIN(TestMapAnimation)
#include "tst_map_animation.moc"
