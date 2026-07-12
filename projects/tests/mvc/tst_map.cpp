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
 * @file tst_map.cpp
 * @brief The map emulator: the block data, the overworld buffer, the view maths,
 *        the renderer, and the QML-facing model.
 *
 * The test that matters most is @ref viewPointer_matchesWhatTheGameStored: a real
 * save carries the pointer the *Game Boy itself* computed for where the view starts
 * (`wCurrentTileBlockMapViewPointer`, at 0x260B). We recompute it from nothing but
 * the player's coordinates and the map's width, and it must come out identical. If
 * that ever fails, our idea of how the game builds its view is wrong -- everything
 * else here is downstream of it.
 *
 * Needs a GUI app (QPixmap/QPainter) and the app's qrc (the tileset PNGs live in
 * app.qrc), and runs offscreen -- same wiring as tst_engine_providers.
 */

#include <QtTest>
#include <QImage>
#include <QPixmap>
#include <QSet>
#include <QSize>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/entries/mapdbentry.h>

#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areatileset.h>

#include <engine/mapengine.h>
#include <engine/mapprovider.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

/// Every map id that has block data.
QVector<int> mapsWithBlocks()
{
  QVector<int> out;
  for (int i = 0; i < 256; i++)
    if (BlocksDB::inst()->hasMap(i))
      out.append(i);
  return out;
}

/// The tileset a map's own header names (no deep link needed).
int tilesetOf(int mapInd)
{
  return MapEngine::tilesetOf(mapInd);
}

/// The three "Copy" maps maps.json sizes but leaves with an EMPTY tileset string.
///
/// In ROM they alias the original map's header, so their tileset is really House,
/// Gate and Mart. maps.json simply doesn't say so -- a small data gap, left alone
/// rather than quietly patched (the JSON is curated). It costs the map screen
/// nothing: when one of these is the loaded map, the tileset comes from the SAVE
/// (wCurMapTileset), which the game filled in correctly. It only means the DB alone
/// cannot name their tileset. Pinned here so the gap can't silently grow.
const QVector<int> kMapsWithNoTilesetInJson = {
  69,  // Trashed House Copy          -> ROM: CeruleanTrashedHouse (House)
  75,  // Path Entrance Route 6 Copy  -> ROM: UndergroundPathRoute6 (Gate)
  173  // Cinnabar Mart Copy          -> ROM: CinnabarMart (Mart)
};

} // namespace

class TestMap : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  // The data
  void blocks_everyRealMapHasBlockData();
  void blocks_everyMapIsExactlyItsDeclaredSize();
  void blocks_everyMapNamesATileset();
  void blocks_everyBlockUsedExistsInItsTileset();
  void blocks_noMapUsesATileBeyondItsTilesetGraphics();
  void blocks_everyBorderBlockExistsInItsTileset();

  // The view maths -- the game's own
  void viewPointer_matchesWhatTheGameStored();
  void viewPointer_matchesWhatTheGameStored_data();
  void view_screenSitsInsideTheScratchArea();
  void view_scratchAreaIsAlwaysBlockAligned();
  void view_everyViewFitsInsideTheBuffer();

  // The buffer + the renderer
  void buffer_isTheMapRingedByItsBorderBlock();
  void buffer_glitchMapHasNone();
  void render_isOneScreenPixelPerGameBoyPixel();
  void provider_servesTheMapAndFallsBackCleanly();

  // The QML face
  void model_publishesTheLoadedMap();
};

void TestMap::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

// ── The data ─────────────────────────────────────────────────────────────────

void TestMap::blocks_everyRealMapHasBlockData()
{
  // 248 map ids exist in ROM, but 22 of them are ids maps.json marks as unused glitch
  // maps and gives no dimensions -- we deliberately do not import those (the ROM would
  // load another map's header for them; see the blocks README + scripts/import_map_blocks.ps1).
  // That leaves 226 maps we can honestly draw, across 24 tilesets.
  QCOMPARE(BlocksDB::inst()->mapCount(), 226);
  QCOMPARE(BlocksDB::inst()->tilesetCount(), 24);

  // Every map the editor considers real must be drawable...
  for (auto* entry : MapsDB::inst()->getStore()) {
    if (entry->getWidth() <= 0 || entry->getHeight() <= 0)
      continue; // unsized => a glitch id, nothing to draw

    QVERIFY2(BlocksDB::inst()->hasMap(entry->getInd()),
             qPrintable(QString("map %1 (%2) has no block data")
                        .arg(entry->getInd()).arg(entry->bestName())));
  }

  // ...and the unsized/absent ids must not acquire any by accident.
  QVERIFY(!BlocksDB::inst()->hasMap(11));  // "Unused Map 0B"
  QVERIFY(!BlocksDB::inst()->hasMap(248)); // past the end of the header table
  QVERIFY(!BlocksDB::inst()->hasMap(255));
}

void TestMap::blocks_everyMapIsExactlyItsDeclaredSize()
{
  for (int ind : mapsWithBlocks()) {
    auto* entry = MapsDB::inst()->getIndAt(QString::number(ind));
    QVERIFY2(entry != nullptr, qPrintable(QString("map %1 has blocks but no DB entry").arg(ind)));

    const int expect = entry->getWidth() * entry->getHeight();
    const int actual = BlocksDB::inst()->mapBlocks(ind).size();

    QVERIFY2(actual == expect,
             qPrintable(QString("map %1 (%2): %3 blocks, header says %4x%5 = %6")
                        .arg(ind).arg(entry->bestName()).arg(actual)
                        .arg(entry->getWidth()).arg(entry->getHeight()).arg(expect)));
  }
}

void TestMap::blocks_everyMapNamesATileset()
{
  // Exactly three maps can't name their tileset from the DB alone, and we know which.
  QVector<int> missing;
  for (int ind : mapsWithBlocks())
    if (tilesetOf(ind) < 0)
      missing.append(ind);

  QCOMPARE(missing, kMapsWithNoTilesetInJson);
}

void TestMap::blocks_everyBlockUsedExistsInItsTileset()
{
  for (int ind : mapsWithBlocks()) {
    const int ts = tilesetOf(ind);
    if (ts < 0)
      continue; // see kMapsWithNoTilesetInJson

    const int blockCount = BlocksDB::inst()->tilesetBlockCount(ts);
    QVERIFY(blockCount > 0);

    for (char c : BlocksDB::inst()->mapBlocks(ind)) {
      const int block = static_cast<quint8>(c);
      QVERIFY2(block < blockCount,
               qPrintable(QString("map %1 uses block %2, tileset %3 has only %4")
                          .arg(ind).arg(block).arg(ts).arg(blockCount)));
    }
  }
}

void TestMap::blocks_noMapUsesATileBeyondItsTilesetGraphics()
{
  // A blockset's tile ids are Game Boy BG tile ids: 0x00-0x5F is the tileset's own
  // graphics (MAP_TILESET_SIZE), and everything above it is the text-box tiles and
  // the font -- a different region of VRAM entirely. A few UNUSED blocks in the
  // reds_house/house/gate blocksets do point up there, but no map ever places them.
  //
  // This is what lets the renderer draw from the tileset image alone. If a map ever
  // did use a tile past 0x5F, the render would silently be wrong -- so pin it.
  for (int ind : mapsWithBlocks()) {
    const int ts = tilesetOf(ind);
    if (ts < 0)
      continue; // see kMapsWithNoTilesetInJson

    const QByteArray blockset = BlocksDB::inst()->tilesetBlocks(ts);
    QVERIFY(!blockset.isEmpty());

    QSet<int> used;
    for (char c : BlocksDB::inst()->mapBlocks(ind))
      used.insert(static_cast<quint8>(c));

    for (int block : used) {
      const int base = block * BlocksDB::tilesPerBlock;
      for (int i = 0; i < BlocksDB::tilesPerBlock; i++) {
        const int tile = static_cast<quint8>(blockset[base + i]);
        QVERIFY2(tile < BlocksDB::mapTilesetSize,
                 qPrintable(QString("map %1 (tileset %2) uses tile 0x%3, past the tileset's 0x60")
                            .arg(ind).arg(ts).arg(tile, 2, 16, QChar('0'))));
      }
    }
  }
}

void TestMap::blocks_everyBorderBlockExistsInItsTileset()
{
  // The border block fills the 3-block ring around the map. It comes from the map's
  // object data, not its blockset, so nothing guarantees it is in range -- check.
  for (int ind : mapsWithBlocks()) {
    auto* entry = MapsDB::inst()->getIndAt(QString::number(ind));
    const int border = entry->getBorder();
    const int ts = tilesetOf(ind);
    if (border < 0 || ts < 0)
      continue; // no border recorded / see kMapsWithNoTilesetInJson

    const int blockCount = BlocksDB::inst()->tilesetBlockCount(ts);
    QVERIFY2(border < blockCount,
             qPrintable(QString("map %1 (%2): border block %3, tileset has only %4")
                        .arg(ind).arg(entry->bestName()).arg(border).arg(blockCount)));
  }
}

// ── The view maths ───────────────────────────────────────────────────────────

void TestMap::viewPointer_matchesWhatTheGameStored_data()
{
  QTest::addColumn<QString>("fixture");

  // The two saves made by a real Game Boy. (The synthetic fixtures are zeroed or
  // garbage -- they carry no pointer the game ever computed, so there is nothing
  // to compare against there.)
  QTest::newRow("BaseSAV") << QStringLiteral("saves/natural-clean/BaseSAV.sav");
  QTest::newRow("BaseSAV.new") << QStringLiteral("saves/natural-clean/BaseSAV.new.sav");
}

void TestMap::viewPointer_matchesWhatTheGameStored()
{
  QFETCH(QString, fixture);

  const QByteArray bytes = readSaveBytes(fixture);
  QCOMPARE(bytes.size(), kSaveSize);

  SaveFile sf;
  loadInto(sf, bytes);

  const int mapInd = sf.dataExpanded->area->map->curMap;
  const int x = sf.dataExpanded->area->player->xCoord;
  const int y = sf.dataExpanded->area->player->yCoord;

  auto* entry = MapsDB::inst()->getIndAt(QString::number(mapInd));
  QVERIFY(entry != nullptr);

  // wCurrentTileBlockMapViewPointer, little-endian, exactly as the game left it.
  const int stored = sf.toolset->getByte(0x260B) | (sf.toolset->getByte(0x260C) << 8);

  QCOMPARE(MapEngine::viewPointer(x, y, entry->getWidth()), stored);

  // And the half-block coords the game stored are just the low bit of each coord.
  QCOMPARE(sf.dataExpanded->area->player->xBlockCoord, x & 1);
  QCOMPARE(sf.dataExpanded->area->player->yBlockCoord, y & 1);
}

void TestMap::view_screenSitsInsideTheScratchArea()
{
  // Whatever half of whatever block the player stands in, the 20x18-tile screen is
  // always a sub-rectangle of the 6x5-block scratch area. That containment IS the
  // reason the game can draw a smoothly-scrolling map out of a fixed block grid.
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      const QRect scratch = MapEngine::scratchRect(x, y);
      const QRect screen = MapEngine::screenRect(x, y);

      QVERIFY2(scratch.contains(screen),
               qPrintable(QString("screen escapes the scratch area at %1,%2").arg(x).arg(y)));

      // The screen shifts by exactly two tiles per half-block, and no more.
      QCOMPARE(screen.x() - scratch.x(), (x % 2) * 2 * MapEngine::tilePx);
      QCOMPARE(screen.y() - scratch.y(), (y % 2) * 2 * MapEngine::tilePx);

      QCOMPARE(screen.width(), MapEngine::screenTilesW * MapEngine::tilePx);   // 160
      QCOMPARE(screen.height(), MapEngine::screenTilesH * MapEngine::tilePx);  // 144
    }
  }
}

void TestMap::view_scratchAreaIsAlwaysBlockAligned()
{
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      const QRect scratch = MapEngine::scratchRect(x, y);
      QCOMPARE(scratch.x() % MapEngine::blockPx, 0);
      QCOMPARE(scratch.y() % MapEngine::blockPx, 0);
      QCOMPARE(scratch.width(), MapEngine::screenBlocksW * MapEngine::blockPx);  // 192
      QCOMPARE(scratch.height(), MapEngine::screenBlocksH * MapEngine::blockPx); // 160
    }
  }
}

void TestMap::view_everyViewFitsInsideTheBuffer()
{
  // The 3-block border is not decoration: it is exactly what makes the buffer big
  // enough to hold every view the player can produce, right out to the map's corners.
  // Walk every legal position on a real map and prove the scratch area never leaves.
  const auto buffer = MapEngine::buildOverworldMap(0); // Pallet Town
  QVERIFY(buffer.valid);

  const QRect bounds(0, 0, buffer.stride * MapEngine::blockPx, buffer.rows * MapEngine::blockPx);

  for (int y = 0; y < buffer.height * 2; y++) {
    for (int x = 0; x < buffer.width * 2; x++) {
      QVERIFY2(bounds.contains(MapEngine::scratchRect(x, y)),
               qPrintable(QString("the view leaves the buffer at %1,%2").arg(x).arg(y)));
    }
  }
}

// ── The buffer + the renderer ────────────────────────────────────────────────

void TestMap::buffer_isTheMapRingedByItsBorderBlock()
{
  const int mapInd = 0; // Pallet Town, 10x9
  const auto buffer = MapEngine::buildOverworldMap(mapInd);
  QVERIFY(buffer.valid);

  auto* entry = MapsDB::inst()->getIndAt(QString::number(mapInd));
  QCOMPARE(buffer.width, entry->getWidth());
  QCOMPARE(buffer.height, entry->getHeight());
  QCOMPARE(buffer.stride, entry->getWidth() + 2 * MapEngine::mapBorder);
  QCOMPARE(buffer.rows, entry->getHeight() + 2 * MapEngine::mapBorder);
  QCOMPARE(buffer.blocks.size(), buffer.stride * buffer.rows);
  QCOMPARE(buffer.border, entry->getBorder());

  const QByteArray raw = BlocksDB::inst()->mapBlocks(mapInd);

  for (int y = 0; y < buffer.rows; y++) {
    for (int x = 0; x < buffer.stride; x++) {
      const int block = static_cast<quint8>(buffer.blocks[y * buffer.stride + x]);

      const bool insideMap = x >= MapEngine::mapBorder && x < MapEngine::mapBorder + buffer.width
                          && y >= MapEngine::mapBorder && y < MapEngine::mapBorder + buffer.height;

      if (insideMap) {
        const int mx = x - MapEngine::mapBorder;
        const int my = y - MapEngine::mapBorder;
        QCOMPARE(block, static_cast<int>(static_cast<quint8>(raw[my * buffer.width + mx])));
      }
      else {
        // The ring is the map's border block, all the way round.
        QCOMPARE(block, buffer.border);
      }
    }
  }
}

void TestMap::buffer_glitchMapHasNone()
{
  // A glitch id has no blocks in ROM. Inventing a map for it would be worse than
  // drawing nothing, so the buffer must simply come back invalid.
  const auto buffer = MapEngine::buildOverworldMap(250);
  QVERIFY(!buffer.valid);
  QVERIFY(MapEngine::render(buffer, 0, 0).isNull());
}

void TestMap::render_isOneScreenPixelPerGameBoyPixel()
{
  const int mapInd = 0; // Pallet Town, 10x9, OVERWORLD
  const auto buffer = MapEngine::buildOverworldMap(mapInd);
  QVERIFY(buffer.valid);

  const QImage img = MapEngine::render(buffer, tilesetOf(mapInd), 0);
  QVERIFY(!img.isNull());

  // (10 + 6) x (9 + 6) blocks, 32 px each.
  QCOMPARE(img.width(), (10 + 6) * MapEngine::blockPx);
  QCOMPARE(img.height(), (9 + 6) * MapEngine::blockPx);

  // It must actually be drawn -- a blank sheet would pass every size check above.
  bool anyInk = false;
  for (int y = 0; y < img.height() && !anyInk; y += 4)
    for (int x = 0; x < img.width() && !anyInk; x += 4)
      if (img.pixelColor(x, y) != QColor(Qt::white))
        anyInk = true;

  QVERIFY2(anyInk, "the rendered map is blank");
}

void TestMap::provider_servesTheMapAndFallsBackCleanly()
{
  MapProvider provider;
  QSize size;

  const QPixmap map = provider.requestPixmap("0/0/0", &size, QSize());
  QCOMPARE(map.width(), (10 + 6) * MapEngine::blockPx);
  QCOMPARE(size.width(), (10 + 6) * MapEngine::blockPx);

  // A malformed id, and a map with no data, must both degrade quietly rather than
  // crash or draw garbage.
  QVERIFY(!provider.requestPixmap("nonsense", &size, QSize()).isNull());
  QVERIFY(!provider.requestPixmap("250/0/0", &size, QSize()).isNull());
}

// ── The QML face ─────────────────────────────────────────────────────────────

void TestMap::model_publishesTheLoadedMap()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(bytes.size(), kSaveSize);

  SaveFile sf;
  loadInto(sf, bytes);

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset);

  QVERIFY(model.valid());
  QCOMPARE(model.mapInd(), 0);                 // Pallet Town
  QCOMPARE(model.blocksWide(), 10);
  QCOMPARE(model.blocksHigh(), 9);
  QCOMPARE(model.imageWidth(), (10 + 6) * MapEngine::blockPx);
  QCOMPARE(model.imageHeight(), (9 + 6) * MapEngine::blockPx);
  QVERIFY(!model.mapName().isEmpty());
  QVERIFY(!model.tilesetName().isEmpty());
  QVERIFY(model.source().startsWith("image://map/0/"));

  // The map sits exactly one border ring in from the top-left of the image.
  QCOMPARE(model.mapX(), MapEngine::mapBorder * MapEngine::blockPx);
  QCOMPARE(model.mapY(), MapEngine::mapBorder * MapEngine::blockPx);
  QCOMPARE(model.mapW(), 10 * MapEngine::blockPx);
  QCOMPARE(model.mapH(), 9 * MapEngine::blockPx);

  // And the screen is inside the scratch area, which is inside the image.
  const QRect image(0, 0, model.imageWidth(), model.imageHeight());
  const QRect scratch(model.scratchX(), model.scratchY(), model.scratchW(), model.scratchH());
  const QRect screen(model.screenX(), model.screenY(), model.screenW(), model.screenH());

  QVERIFY(image.contains(scratch));
  QVERIFY(scratch.contains(screen));

  // Moving the player moves the boxes -- and says so.
  QSignalSpy spy(&model, &MapModel::changed);
  sf.dataExpanded->area->player->xCoord = model.playerX() + 2; // one whole block over
  sf.dataExpanded->area->player->xCoordChanged();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(model.scratchX(), scratch.x() + MapEngine::blockPx);
}

QTEST_MAIN(TestMap)
#include "tst_map.moc"
