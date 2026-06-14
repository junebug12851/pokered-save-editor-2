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
 * @file tst_engine_providers.cpp
 * @brief The graphics engine + QML image providers -- the last untested app tier.
 *        Needs a GUI app (QPixmap/QPainter) and the app's qrc (the tileset/font
 *        PNGs live in app.qrc, compiled into this test), and runs on the offscreen
 *        platform. Covers TilesetEngine's resource-backed builders, TilesetProvider,
 *        and FontPreviewProvider, including their malformed-id error fallbacks.
 */

#include <QtTest>
#include <QImage>
#include <QPixmap>
#include <QSize>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>

#include <engine/tilesetengine.h>
#include <engine/tilesetprovider.h>
#include <engine/fontpreviewprovider.h>

using namespace pse_test;

class TestEngineProviders : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void tilesetEngine_resourceBackedImages();
  void tilesetEngine_buildTilesetAndTiles();
  void tilesetProvider_validAndMalformedIds();
  void fontPreviewProvider_validAndMalformedIds();
};

void TestEngineProviders::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

void TestEngineProviders::tilesetEngine_resourceBackedImages()
{
  // These read PNGs from the app qrc (compiled into this test).
  const QImage tileset = TilesetEngine::getTileset(QStringLiteral("overworld"));
  QVERIFY2(!tileset.isNull(), "overworld tileset image failed to load from qrc");
  QCOMPARE(tileset.width(), TilesetEngine::width);
  QCOMPARE(tileset.height(), TilesetEngine::height);

  QVERIFY2(!TilesetEngine::getFont().isNull(), "font overlay image failed to load");
  for(int frame = 0; frame < 4; frame++)
    QVERIFY2(!TilesetEngine::getFlower(frame).isNull(),
             qPrintable(QStringLiteral("flower frame %1 failed to load").arg(frame)));

  const QImage blank = TilesetEngine::blankImage();
  QCOMPARE(blank.width(), TilesetEngine::width);
  QVERIFY(blank.hasAlphaChannel());
}

void TestEngineProviders::tilesetEngine_buildTilesetAndTiles()
{
  // getTiles slices a 128x128 image into 8x8 tiles -> (128/8)^2 = 256.
  const QVector<QPixmap> tiles = TilesetEngine::getTiles(TilesetEngine::getTileset(QStringLiteral("overworld")));
  QCOMPARE(tiles.size(), (TilesetEngine::width / 8) * (TilesetEngine::height / 8));

  // A well-formed id: <tileset>/<outdoor?>/<font?>/<frame>. Builds the composed
  // 128x128 tileset (tileset + font + flower layers via QPainter) then slices it.
  const QString id = QStringLiteral("overworld/outdoor/font/0");
  const QPixmap full = TilesetEngine::buildTilesetFullDebug(id);
  QVERIFY2(!full.isNull(), "buildTilesetFullDebug returned a null pixmap");

  const QVector<QPixmap> built = TilesetEngine::buildTileset(id);
  QCOMPARE(built.size(), (TilesetEngine::width / 8) * (TilesetEngine::height / 8));
}

void TestEngineProviders::tilesetProvider_validAndMalformedIds()
{
  TilesetProvider provider;
  QSize size;

  // Valid id has 7 parts: <tileset>/<outdoor?>/<font?>/<frame>/<tileIndex>/<w>/<h>.
  const QPixmap tile = provider.requestPixmap(QStringLiteral("overworld/outdoor/font/0/5/16/16"), &size, QSize());
  QVERIFY2(!tile.isNull(), "valid tile request returned null");

  // Malformed id (<7 parts) must fall back to the blank/error tile, not crash.
  const QPixmap blank = provider.requestPixmap(QStringLiteral("garbage"), &size, QSize());
  QVERIFY2(!blank.isNull(), "malformed tile request did not return a fallback");
}

void TestEngineProviders::fontPreviewProvider_validAndMalformedIds()
{
  SaveFile sf; loadInto(sf, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
  FontPreviewProvider provider(sf.dataExpanded);
  QSize size;

  // 12 parts: tileset/type/frame/width/height/box?/2-lines?/chop/bg/fg/placeholder/str
  const QString id = QStringLiteral("overworld/outdoor/0/160/16/nobox/1-line/0/white/none/x/RED");
  const QPixmap img = provider.requestPixmap(id, &size, QSize());
  QVERIFY2(!img.isNull(), "valid font-preview request returned null");

  // Too few parts -> error image, not a crash.
  const QPixmap err = provider.requestPixmap(QStringLiteral("too/few/parts"), &size, QSize());
  QVERIFY2(!err.isNull(), "malformed font-preview request did not return a fallback");
}

QTEST_MAIN(TestEngineProviders)
#include "tst_engine_providers.moc"
