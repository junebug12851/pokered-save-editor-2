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
 * @file tst_tileset_engine.cpp
 * @brief Pure-logic coverage of TilesetEngine's image math -- the water-"wave"
 *        post-process and the blank-tile builder. These are QImage operations with
 *        no Qt-resource or GPU dependency, so they run headless. (The resource-
 *        backed getters getTileset()/getFont() read `:/assets`, which lives in the
 *        app executable's qrc, not appcore -- so they're intentionally not tested
 *        here.) The wave is a per-row right-rotate applied `shift` times, where the
 *        shift ramps 0..4..0 across an 8-frame cycle -- giving clean invariants.
 */

#include <QtTest>
#include <QImage>
#include <QColor>

#include <engine/tilesetengine.h>

class TestTilesetEngine : public QObject
{
  Q_OBJECT

  // An 8x8 tile whose columns all differ, so a horizontal rotate is detectable
  // (and not accidentally symmetric).
  static QImage makeVaryingTile()
  {
    const int w = TilesetEngine::tileWidth;
    const int h = TilesetEngine::tileHeight;
    QImage img(w, h, QImage::Format_ARGB32);
    for(int y = 0; y < h; y++)
      for(int x = 0; x < w; x++)
        img.setPixelColor(x, y, QColor(20 + x * 25, 10 + y * 20, 40, 255));
    return img;
  }

private slots:
  void blankImage_isTransparentAndSized();
  void wave_frameZeroAndCycleAreIdentity();
  void wave_isSymmetricAroundFrameFour();
  void wave_actuallyShifts();
  void waveOnce_eightApplicationsWrapToOriginal();
};

void TestTilesetEngine::blankImage_isTransparentAndSized()
{
  const QImage img = TilesetEngine::blankImage();
  QCOMPARE(img.width(), TilesetEngine::width);
  QCOMPARE(img.height(), TilesetEngine::height);
  QVERIFY(img.hasAlphaChannel());
  QCOMPARE(img.pixelColor(0, 0).alpha(), 0);                 // fully transparent
  QCOMPARE(img.pixelColor(img.width() - 1, img.height() - 1).alpha(), 0);
}

void TestTilesetEngine::wave_frameZeroAndCycleAreIdentity()
{
  const QImage tile = makeVaryingTile();
  // frame 0 -> 0 shifts, and frame 8 wraps to the same point in the cycle.
  QCOMPARE(TilesetEngine::postProcessWave(tile, 0), tile);
  QCOMPARE(TilesetEngine::postProcessWave(tile, 8), tile);
}

void TestTilesetEngine::wave_isSymmetricAroundFrameFour()
{
  const QImage tile = makeVaryingTile();
  // The shift ramp is 0,1,2,3,4,3,2,1 -> frames equidistant from 4 match.
  QCOMPARE(TilesetEngine::postProcessWave(tile, 1), TilesetEngine::postProcessWave(tile, 7));
  QCOMPARE(TilesetEngine::postProcessWave(tile, 3), TilesetEngine::postProcessWave(tile, 5));
}

void TestTilesetEngine::wave_actuallyShifts()
{
  const QImage tile = makeVaryingTile();
  const QImage shifted = TilesetEngine::postProcessWave(tile, 1);
  QCOMPARE(shifted.size(), tile.size());          // size preserved
  QVERIFY2(shifted != tile, "a 1-shift wave left a column-varying tile unchanged");
}

void TestTilesetEngine::waveOnce_eightApplicationsWrapToOriginal()
{
  const QImage tile = makeVaryingTile();
  QImage img = tile;
  // Each call rotates every row right by one; width is 8, so 8 rotations == identity.
  for(int i = 0; i < TilesetEngine::tileWidth; i++)
    img = TilesetEngine::postProcessWaveOnce(img);
  QCOMPARE(img, tile);
}

QTEST_GUILESS_MAIN(TestTilesetEngine)
#include "tst_tileset_engine.moc"
