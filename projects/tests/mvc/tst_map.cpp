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
#include <QMap>
#include <QPair>
#include <QSet>
#include <QSize>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentryconnect.h>
#include <pse-db/eventsdb.h>
#include <pse-db/entries/eventdbentry.h>

#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldmissables.h>

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

/// The glitch / half-baked ids, and the map each one is really a copy of.
///
/// These are the ids that carry no size and/or no tileset of their own. They are not
/// empty maps -- they are unfinished duplicates, and `maps.json` already says which
/// map of (its `incomplete` field), in exact agreement with the ROM's header-pointer
/// table. So they render: we follow the link and draw the map they copy, which is what
/// a Game Boy loading that id puts on screen. Nothing is invented.
const QVector<QPair<int, QString>> kCopyMaps = {
  { 11,  "Saffron City" },
  { 69,  "Trashed House" },            // sized, but no tileset of its own
  { 75,  "Path Entrance Route 6" },    // ditto
  { 105, "Lances Room" }, { 106, "Lances Room" }, { 107, "Lances Room" },
  { 109, "Lances Room" }, { 110, "Lances Room" }, { 111, "Lances Room" },
  { 112, "Lances Room" }, { 114, "Lances Room" }, { 115, "Lances Room" },
  { 116, "Lances Room" }, { 117, "Lances Room" },
  { 173, "Cinnabar Mart" },            // sized, but no tileset of its own
  { 204, "Rocket Hideout Elevator" }, { 205, "Rocket Hideout Elevator" },
  { 206, "Rocket Hideout Elevator" },
  { 231, "Route 16 Gate 1F" },
  { 237, "Silph Co 2F" }, { 238, "Silph Co 2F" }, { 241, "Silph Co 2F" },
  { 242, "Silph Co 2F" }, { 243, "Silph Co 2F" }, { 244, "Silph Co 2F" }
};

} // namespace

class TestMap : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  // The data
  void borderBlock_comesFromTheSave();
  void blocks_everyRealMapHasBlockData();
  void blocks_everyMapIsExactlyItsDeclaredSize();
  void everyMapIdRenders();
  void copyMaps_drawTheMapTheyAreACopyOf();
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
  void connections_bleedTheNeighbouringMapsIntoTheRing();
  void connections_offsetIsRecoveredFromTheStoredPair();
  void buffer_isTheMapRingedByItsBorderBlock();
  void palettes_matchTheConsoleForEveryContrastValue();
  void palettes_actuallyRepaintTheMap();
  void player_isDrawnWhereTheConsolePutsHim();
  void player_facingComesFromTheSavesBitFlags();
  void player_spriteIsCutOutAndPalettedLikeAnObject();
  void buffer_glitchMapHasNone();
  void render_isOneScreenPixelPerGameBoyPixel();
  void provider_servesTheMapAndFallsBackCleanly();

  // The QML face
  void model_publishesTheLoadedMap();

  void hotspots_boxOnlyTheThingsTheSaveKeepsAFlagFor();
  void hotspots_comeFromTheRomSoAHiddenObjectStillHasABox();
  void hotspots_hiddenPickupsLandAtTheirRomCoords();
  void hotspots_hiddenCoinsShowTheirAmount();
  void hotspots_eventFlagsAppearWhereTheirScriptHappens();
  void hotspots_tileTraitsOnlyWhenTheirLayerIsShown();
  void phases_tellTheMapsStoryInOrder();
  void flagHistory_saysWhoTurnedItOnAndWhere();
  void hotspots_aBlockOnlyCarriesWhatOverlapsIt();
  void storageHidden_listsThePickupsUnderTheirOwnMap();
};

void TestMap::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

// ── The data ─────────────────────────────────────────────────────────────────

void TestMap::blocks_everyRealMapHasBlockData()
{
  // 226 maps own their block data outright; the other ids are copies that borrow it
  // (see everyMapIdRenders below). 24 tilesets.
  QCOMPARE(BlocksDB::inst()->mapCount(), 226);
  QCOMPARE(BlocksDB::inst()->tilesetCount(), 24);

  // Every map the editor sizes must own its blocks...
  for (auto* entry : MapsDB::inst()->getStore()) {
    if (entry->getWidth() <= 0 || entry->getHeight() <= 0)
      continue; // unsized => a copy, it borrows

    QVERIFY2(BlocksDB::inst()->hasMap(entry->getInd()),
             qPrintable(QString("map %1 (%2) has no block data")
                        .arg(entry->getInd()).arg(entry->bestName())));
  }

  // ...and ids that own none must not acquire any by accident.
  QVERIFY(!BlocksDB::inst()->hasMap(11));  // "Unused Map 0B" -- a copy of Saffron City
  QVERIFY(!BlocksDB::inst()->hasMap(248)); // past the end of the header table
  QVERIFY(!BlocksDB::inst()->hasMap(255));
}

/**
 * The edge of the world comes from the SAVE, not from the map's shipped border.
 *
 * `wMapBackgroundTile` (`AreaMap::outOfBoundsBlock`, save 0x2659) is the block the game fills the
 * 3-block ring with, and it is a byte in the save -- so a save may hold something else, and a
 * console draws whatever it holds.
 *
 * Until 2026-07-13 `buildOverworldMap` always used the *map's* border, so editing that byte in the
 * editor changed nothing on screen -- the control did nothing and said nothing. Found by Twilight,
 * by changing it and watching the map not change.
 */
void TestMap::borderBlock_comesFromTheSave()
{
  const int pallet = 0;

  const auto shipped = MapEngine::buildOverworldMap(pallet);          // -1: the map's own border
  QVERIFY(shipped.valid);

  // A different block entirely. The ring must be built out of THAT.
  const int want = (shipped.border == 1) ? 2 : 1;
  const auto edited = MapEngine::buildOverworldMap(pallet, want);
  QVERIFY(edited.valid);

  QCOMPARE(edited.border, want);
  QVERIFY2(edited.blocks != shipped.blocks,
           "changing the save's out-of-bounds block did not change the map's blocks -- the edge of "
           "the world is being drawn from the cartridge instead of from the save");

  // The ring is what changed; the map itself is untouched.
  const int mb = MapEngine::mapBorder;
  QCOMPARE(static_cast<quint8>(edited.blocks.at(0)), static_cast<quint8>(want));   // a ring corner

  for (int y = 0; y < edited.height; y++) {
    for (int x = 0; x < edited.width; x++) {
      const int at = (y + mb) * edited.stride + (x + mb);
      QCOMPARE(edited.blocks.at(at), shipped.blocks.at(at));   // the map proper: identical
    }
  }
}

void TestMap::everyMapIdRenders()
{
  // The whole point: if the game has data for an id -- even second-hand, as a copy --
  // we draw it. Every one of the 248 real ids must produce a buffer.
  for (int ind = 0; ind < 248; ind++) {
    auto* entry = MapsDB::inst()->getIndAt(QString::number(ind));
    if (entry == nullptr)
      continue;

    const auto buffer = MapEngine::buildOverworldMap(ind);
    QVERIFY2(buffer.valid,
             qPrintable(QString("map %1 (%2) renders nothing").arg(ind).arg(entry->bestName())));
    QVERIFY(MapEngine::tilesetOf(ind) >= 0);
  }

  // "Last Map" (255) really is empty -- no size, no blocks, nothing it is a copy of.
  // Drawing something for it would mean inventing a map, so it stays blank.
  QVERIFY(!MapEngine::buildOverworldMap(255).valid);
}

void TestMap::copyMaps_drawTheMapTheyAreACopyOf()
{
  for (const auto& [ind, copyOf] : kCopyMaps) {
    const auto buffer = MapEngine::buildOverworldMap(ind);

    QVERIFY2(buffer.valid, qPrintable(QString("copy map %1 renders nothing").arg(ind)));
    QVERIFY2(buffer.isCopy, qPrintable(QString("map %1 should be flagged a copy").arg(ind)));

    // It must land on the map maps.json says it copies -- and be the *same* map, not a
    // lookalike: same size, and identical blocks byte for byte.
    auto* source = MapsDB::inst()->getIndAt(copyOf);
    QVERIFY2(source != nullptr, qPrintable(QString("'%1' does not resolve").arg(copyOf)));

    QCOMPARE(buffer.sourceInd, source->getInd());
    QCOMPARE(buffer.sourceName, source->bestName());  // the display name QML shows
    QCOMPARE(buffer.width, source->getWidth());
    QCOMPARE(buffer.height, source->getHeight());
    QCOMPARE(MapEngine::buildOverworldMap(source->getInd()).blocks, buffer.blocks);

    // ...and the tileset it draws with is the original's, not a guess.
    QCOMPARE(MapEngine::tilesetOf(ind), MapEngine::tilesetOf(source->getInd()));
  }

  // A map that owns its data is never flagged a copy.
  QVERIFY(!MapEngine::buildOverworldMap(0).isCopy); // Pallet Town
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

void TestMap::blocks_everyBlockUsedExistsInItsTileset()
{
  for (int ind : mapsWithBlocks()) {
    const int ts = tilesetOf(ind);
    QVERIFY2(ts >= 0, qPrintable(QString("map %1 names no tileset").arg(ind)));

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
    QVERIFY(ts >= 0);

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
    if (border < 0)
      continue; // no border recorded

    QVERIFY(ts >= 0);
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
  // An UNCONNECTED map -- Red's House 1F, an interior with no neighbours. Its ring really
  // is nothing but the border block, all the way round, because there is nothing to bleed
  // into it. (A connected map's ring is a different story entirely: see
  // connections_bleedTheNeighbouringMapsIntoTheRing.)
  auto* redsHouse = MapsDB::inst()->getIndAt("Reds House 1F");
  QVERIFY(redsHouse != nullptr);
  QVERIFY2(redsHouse->getConnectAt(MapDBEntryConnect::ConnectDir::NORTH) == nullptr
        && redsHouse->getConnectAt(MapDBEntryConnect::ConnectDir::SOUTH) == nullptr
        && redsHouse->getConnectAt(MapDBEntryConnect::ConnectDir::EAST) == nullptr
        && redsHouse->getConnectAt(MapDBEntryConnect::ConnectDir::WEST) == nullptr,
           "Red's House 1F was supposed to have no connections");

  const int mapInd = redsHouse->getInd();
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

void TestMap::connections_bleedTheNeighbouringMapsIntoTheRing()
{
  // Pallet Town connects north to Route 1 and south to Route 21. Its border ring is
  // therefore NOT a wall of trees: the top three rows are Route 1's bottom three, and the
  // bottom three are Route 21's top three. (The console agrees -- tst_emu_parity.)
  const auto pallet = MapEngine::buildOverworldMap(0);
  QVERIFY(pallet.valid);

  auto* route1 = MapsDB::inst()->getIndAt("Route 1");
  auto* route21 = MapsDB::inst()->getIndAt("Route 21");
  QVERIFY(route1 != nullptr);
  QVERIFY(route21 != nullptr);

  const QByteArray r1 = BlocksDB::inst()->mapBlocks(route1->getInd());
  const QByteArray r21 = BlocksDB::inst()->mapBlocks(route21->getInd());
  QVERIFY(!r1.isEmpty());
  QVERIFY(!r21.isEmpty());

  const int border = MapEngine::mapBorder;

  // Both connections have offset 0, and both routes are 10 wide -- the same as Pallet --
  // so the strips line up column-for-column with no shift, which makes this readable.
  for (int row = 0; row < border; row++) {
    for (int col = 0; col < pallet.width; col++) {
      // North: Route 1's LAST three rows.
      const int north = static_cast<quint8>(
        r1[(route1->getHeight() - 3 + row) * route1->getWidth() + col]);
      QCOMPARE(static_cast<quint8>(pallet.blocks[row * pallet.stride + border + col]), north);

      // South: Route 21's FIRST three rows.
      const int south = static_cast<quint8>(r21[row * route21->getWidth() + col]);
      const int at = (border + pallet.height + row) * pallet.stride + border + col;
      QCOMPARE(static_cast<quint8>(pallet.blocks[at]), south);
    }
  }

  // And the ring is NOT simply the border block any more -- which is the whole point.
  bool anyNotBorder = false;
  for (int col = 0; col < pallet.width && !anyNotBorder; col++)
    if (static_cast<quint8>(pallet.blocks[border + col]) != pallet.border)
      anyNotBorder = true;

  QVERIFY2(anyNotBorder, "the north ring is still just the border block");
}

void TestMap::connections_offsetIsRecoveredFromTheStoredPair()
{
  // maps.json keeps the POST-clamp pair, not the raw offset the header was written with.
  // Recovering it is what makes the macro computable at all, and it is verified against the
  // real cartridge for all 78 connections (scripts/emu/verify_connections.py).
  struct Case { const char* map; int dir; int offset; };

  const QVector<Case> cases = {
    { "Route 4",       MapDBEntryConnect::ConnectDir::SOUTH, -25 },
    { "Route 11",      MapDBEntryConnect::ConnectDir::EAST,  -27 },
    { "Route 2",       MapDBEntryConnect::ConnectDir::NORTH,  -5 },
    { "Viridian City", MapDBEntryConnect::ConnectDir::NORTH,   5 },
    { "Pallet Town",   MapDBEntryConnect::ConnectDir::NORTH,   0 },
  };

  for (const auto& c : cases) {
    auto* map = MapsDB::inst()->getIndAt(c.map);
    QVERIFY2(map != nullptr, c.map);

    const auto* connect = map->getConnectAt(c.dir);
    QVERIFY2(connect != nullptr, c.map);

    QCOMPARE(MapEngine::connectionOffset(connect), c.offset);
  }
}

void TestMap::palettes_matchTheConsoleForEveryContrastValue()
{
  // These ten rBGP bytes are not from the disassembly -- they are what the REAL Game Boy's
  // palette register actually held, for each contrast value, read out of the console
  // (scripts/emu/verify_palettes.py). The four aligned reads are the contrast levels; the
  // six misaligned ones are the glitch palettes, and they are just as real.
  struct Case { int contrast; int bgp; bool glitch; };

  const QVector<Case> cases = {
    { 0, 0xE4, false },  // FadePal4 -- normal (the identity palette)
    { 1, 0xE4, true  },  // straddles 4/5: the BG survives, the SPRITES are wrecked
    { 2, 0xE4, true  },
    { 3, 0xF9, false },  // FadePal3 -- dark
    { 4, 0xF8, true  },
    { 5, 0xFE, true  },
    { 6, 0xFE, false },  // FadePal2 -- the "needs FLASH" cave palette
    { 7, 0xFF, true  },
    { 8, 0xFF, true  },
    { 9, 0xFF, false },  // FadePal1 -- black
  };

  for (const auto& c : cases) {
    QCOMPARE(MapEngine::backgroundPalette(c.contrast), c.bgp);
    QCOMPARE(MapEngine::isGlitchPalette(c.contrast), c.glitch);
    QVERIFY(!MapEngine::contrastName(c.contrast).isEmpty());
  }

  // Exactly four levels and exactly six glitch palettes -- which is what Twilight said,
  // before any of this was looked at.
  int levels = 0, glitches = 0;
  for (int i = 0; i <= MapEngine::contrastMax; i++)
    MapEngine::isGlitchPalette(i) ? glitches++ : levels++;

  QCOMPARE(levels, 4);
  QCOMPARE(glitches, 6);

  // Past the table the game would read whatever ROM precedes it. We don't ship the ROM, so
  // we say "unknown" rather than invent a palette.
  QCOMPARE(MapEngine::backgroundPalette(MapEngine::contrastMax + 1), -1);
  QCOMPARE(MapEngine::backgroundPalette(-1), -1);
}

void TestMap::palettes_actuallyRepaintTheMap()
{
  const auto buffer = MapEngine::buildOverworldMap(0); // Pallet Town
  QVERIFY(buffer.valid);

  const int tileset = MapEngine::tilesetOf(0);

  const QImage normal = MapEngine::render(buffer, tileset, 0, 0);
  const QImage dark   = MapEngine::render(buffer, tileset, 0, 3);
  const QImage glitch = MapEngine::render(buffer, tileset, 0, 4);
  const QImage black  = MapEngine::render(buffer, tileset, 0, 9);

  QVERIFY(!normal.isNull());
  QCOMPARE(dark.size(), normal.size());

  // A different palette must actually produce a different picture...
  QVERIFY2(dark != normal, "contrast 3 rendered identically to normal");
  QVERIFY2(glitch != normal, "the glitch palette rendered identically to normal");
  QVERIFY2(glitch != dark, "the glitch palette rendered identically to contrast 3");

  // ...and FadePal1 (0xFF: every colour -> shade 3) really is a black screen.
  for (int y = 0; y < black.height(); y += 8)
    for (int x = 0; x < black.width(); x += 8)
      QCOMPARE(black.pixelColor(x, y), QColor(Qt::black));
}

void TestMap::player_isDrawnWhereTheConsolePutsHim()
{
  // Four pixels above his tile row -- "which makes sprites appear to be in the centre of a
  // tile" (ram/wram.asm), and measured off the console's own OAM, not assumed.
  const QRect at = MapEngine::playerRect(5, 6);   // BaseSAV: Pallet Town

  QCOMPARE(at.width(), 16);
  QCOMPARE(at.height(), 16);
  QCOMPARE(at.x(), MapEngine::mapBorder * MapEngine::blockPx + 5 * 16);
  QCOMPARE(at.y(), MapEngine::mapBorder * MapEngine::blockPx + 6 * 16 - MapEngine::spriteLift);

  // And relative to the visible screen he lands on the fixed spot the game pins him to.
  const QRect screen = MapEngine::screenRect(5, 6);
  QCOMPARE(at.x() - screen.x(), 64);
  QCOMPARE(at.y() - screen.y(), 60);
}

void TestMap::player_facingComesFromTheSavesBitFlags()
{
  // The save's playerCurDir is BIT FLAGS (PLAYER_DIR_*), not the sprite facing values
  // (SPRITE_FACING_*). Two encodings of the same idea, and mixing them up would silently
  // point him the wrong way.
  QCOMPARE(MapEngine::facingFromPlayerDir(1), (int)MapEngine::FacingRight); // $C
  QCOMPARE(MapEngine::facingFromPlayerDir(2), (int)MapEngine::FacingLeft);  // $8
  QCOMPARE(MapEngine::facingFromPlayerDir(4), (int)MapEngine::FacingDown);  // $0
  QCOMPARE(MapEngine::facingFromPlayerDir(8), (int)MapEngine::FacingUp);    // $4
  QCOMPARE(MapEngine::facingFromPlayerDir(0), (int)MapEngine::FacingDown);  // the default
}

void TestMap::player_spriteIsCutOutAndPalettedLikeAnObject()
{
  const QImage down = MapEngine::playerSprite(MapEngine::FacingDown, 0);
  QVERIFY(!down.isNull());
  QCOMPARE(down.size(), QSize(16, 16));

  // Colour 0 is transparent for an OBJECT -- always. Without that he'd be a white box.
  QVERIFY2(down.pixelColor(0, 0).alpha() == 0, "the sprite's corner should be transparent");

  bool anyOpaque = false;
  for (int y = 0; y < 16 && !anyOpaque; y++)
    for (int x = 0; x < 16 && !anyOpaque; x++)
      if (down.pixelColor(x, y).alpha() == 255)
        anyOpaque = true;
  QVERIFY2(anyOpaque, "the sprite is entirely transparent");

  // Facing right is facing LEFT, mirrored -- there is no right-facing art in the game.
  const QImage left = MapEngine::playerSprite(MapEngine::FacingLeft, 0);
  const QImage right = MapEngine::playerSprite(MapEngine::FacingRight, 0);
  QCOMPARE(right, left.mirrored(true, false));

  // Up and down are genuinely different drawings.
  QVERIFY(MapEngine::playerSprite(MapEngine::FacingUp, 0) != down);

  // *** The payoff. *** Contrast 1 leaves the MAP untouched (rBGP is still 0xE4) but shifts
  // the OBJECT palette -- so the player, and only the player, changes. That is exactly what
  // the console does, and it is why these two glitch values looked harmless until now.
  QCOMPARE(MapEngine::backgroundPalette(1), MapEngine::backgroundPalette(0));
  QVERIFY(MapEngine::spritePalette(1) != MapEngine::spritePalette(0));
  QVERIFY2(MapEngine::playerSprite(MapEngine::FacingDown, 1) != down,
           "contrast 1 should wreck the player even though the map looks fine");
}

void TestMap::buffer_glitchMapHasNone()
{
  // An id past the end of the header table has nothing in ROM and is a copy of nothing.
  // Inventing a map for it would be worse than drawing nothing.
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

  // A glitch id draws the map it is a copy of -- same picture as Saffron City.
  const QPixmap copy = provider.requestPixmap("11/0/0", &size, QSize());
  QCOMPARE(copy.size(), provider.requestPixmap("10/0/0", &size, QSize()).size());

  // A malformed id, and an id with nothing behind it at all, must both degrade quietly
  // rather than crash or draw garbage.
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
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

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

// ── Flag hotspots (map-screen Phase 16) ─────────────────────────────────────────────────────────
//
// Oak's Lab is the brief's own example -- "in oaks lab the pokeballs should have boxes around them
// because there all tied to flags" -- so it is what the test pins. Its numbers come from the
// cartridge via maps.json, not from us: 11 objects, 8 of them flag-governed.

namespace {
/// Every spot of @p kind, gathered back out of the blocks that own them.
///
/// ⚠️ Deliberately does NOT de-duplicate. A spot lands on every block its true extent touches, so a
/// coord range legitimately appears once per block it crosses -- that is the aggregation rule, and
/// a helper that quietly collapsed it would hide the one behaviour worth checking.
QVariantList filterSpots(const QVariantList& blocks, const QString& kind)
{
  QVariantList out;
  for (const QVariant& b : blocks)
    for (const QVariant& s : b.toMap()["spots"].toList())
      if (s.toMap()["kind"].toString() == kind)
        out.append(s);
  return out;
}
} // namespace

void TestMap::hotspots_boxOnlyTheThingsTheSaveKeepsAFlagFor()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 40;   // Oak's Lab

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  // The filter-flag spots, gathered back out of the blocks that own them. A 16x16 half-block is
  // 16-aligned and the border ring is a whole number of blocks, so each of these lands in EXACTLY
  // one block -- which this collection would notice if it ever stopped being true.
  const QVariantList blocks = model.blockHotspots();
  QVariantList boxes = filterSpots(blocks, QStringLiteral("filterFlag"));

  // 11 objects on the map; the Girl and the two Oak Aides carry no missable and so get NO spot.
  // Boxing them would be clutter, and clutter is a bug.
  QCOMPARE(boxes.size(), 8);
  QCOMPARE(MapsDB::inst()->getIndAt(QStringLiteral("40"))->getSpritesSize(), 11);

  // The three starter Poké Balls: the exact thing she pointed at.
  QMap<int, QPair<int, int>> ballAt;   // missable -> (x, y)
  for (const QVariant& v : boxes) {
    const QVariantMap m = v.toMap();
    const int ind = m["ind"].toInt();
    if (ind >= 43 && ind <= 45)
      ballAt.insert(ind, qMakePair(m["x"].toInt(), m["y"].toInt()));
  }
  QCOMPARE(ballAt.size(), 3);
  QCOMPARE(ballAt.value(43), qMakePair(6, 3));
  QCOMPARE(ballAt.value(44), qMakePair(7, 3));
  QCOMPARE(ballAt.value(45), qMakePair(8, 3));

  // Every spot carries the missable's real name and HIGHLIGHTS its own half-block. The geometry is
  // the same arithmetic the signs and the player use -- and x/y are on the WALK GRID (16x16), though
  // maps.json's width/height are in BLOCKS (32x32). Getting that wrong put a Route 22 sign off the
  // map once.
  for (const QVariant& v : boxes) {
    const QVariantMap m = v.toMap();
    QVERIFY(m["ind"].toInt() >= 0);
    QVERIFY(!m["name"].toString().isEmpty());
    QCOMPARE(m["unit"].toString(), QStringLiteral("halfBlock"));
    QCOMPARE(m["extX"].toInt(),
             MapEngine::mapBorder * MapEngine::blockPx + m["x"].toInt() * 16);
    QCOMPARE(m["extY"].toInt(),
             MapEngine::mapBorder * MapEngine::blockPx + m["y"].toInt() * 16);
    QCOMPARE(m["extW"].toInt(), 16);
    QCOMPARE(m["extH"].toInt(), 16);
  }

  // The HIT TARGET is the block, and it is uniform 32x32 -- deliberately a different thing from the
  // highlight above. It claims to be nothing but the cursor cell, which is why it isn't a lie.
  for (const QVariant& b : blocks) {
    const QVariantMap m = b.toMap();
    QCOMPARE(m["rectW"].toInt(), MapEngine::blockPx);
    QCOMPARE(m["rectH"].toInt(), MapEngine::blockPx);
    QCOMPARE(m["rectX"].toInt(), m["blockX"].toInt() * MapEngine::blockPx);
    QCOMPARE(m["rectY"].toInt(), m["blockY"].toInt() * MapEngine::blockPx);
    QVERIFY(!m["spots"].toList().isEmpty());   // a block with nothing on it must not exist
  }

  // Oak stands in TWO places on this map (missables 46 and 49) -- the other thing she named. Both
  // get a spot, at different tiles.
  int oaks = 0;
  QSet<QPair<int, int>> oakTiles;
  for (const QVariant& v : boxes) {
    const QVariantMap m = v.toMap();
    if (m["sprite"].toString().contains(QStringLiteral("Oak"))) {
      oaks++;
      oakTiles.insert(qMakePair(m["x"].toInt(), m["y"].toInt()));
    }
  }
  QCOMPARE(oaks, 2);
  QCOMPARE(oakTiles.size(), 2);
}

void TestMap::hotspots_comeFromTheRomSoAHiddenObjectStillHasABox()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 40;   // Oak's Lab

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  const int before = filterSpots(model.blockHotspots(), QStringLiteral("filterFlag")).size();

  // THE KEYSTONE. Hide every one of them -- the whole point of the feature is that the boxes come
  // from the cartridge's cast, not from the save's sprite slots, so hiding an object must NOT take
  // its box away. (WorldMissables: bit set = HIDDEN.)
  for (int i = 42; i <= 49; i++)
    sf.dataExpanded->world->missables->missablesSet(i, true);

  QCOMPARE(filterSpots(model.blockHotspots(), QStringLiteral("filterFlag")).size(), before);
  QCOMPARE(before, 8);

  // And the save agrees they are hidden -- which is what dashes the box in the canvas.
  for (int i = 42; i <= 49; i++)
    QVERIFY(sf.dataExpanded->world->missables->missablesAt(i));
}

/// Hidden pickups land on the map, at the ROM's own coordinates, with their real item on them.
/// Viridian Forest is the first two rows of `HiddenItemCoords`, so it pins the bit<->place identity
/// at the exact place it is easiest to get transposed.
void TestMap::hotspots_hiddenPickupsLandAtTheirRomCoords()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 51;   // Viridian Forest

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  const QVariantList items = filterSpots(model.blockHotspots(), QStringLiteral("hiddenItem"));
  QCOMPARE(items.size(), 2);   // `hidden_item VIRIDIAN_FOREST, 1, 18` and `, 16, 42`

  QMap<int, QVariantMap> byInd;
  for (const QVariant& v : items)
    byInd.insert(v.toMap()["ind"].toInt(), v.toMap());

  // Bit 0 == row 0 of the ROM's table == the Potion at (1, 18). Index identity, end to end:
  // the save's bit, the ROM's row, the tile, and the item all have to agree.
  QVERIFY(byInd.contains(0));
  QCOMPARE(byInd[0]["x"].toInt(), 1);
  QCOMPARE(byInd[0]["y"].toInt(), 18);
  QCOMPARE(byInd[0]["item"].toString(), QStringLiteral("POTION"));
  QCOMPARE(byInd[0]["name"].toString(), QStringLiteral("Potion")); // items.json's own spelling
  QCOMPARE(byInd[0]["section"].toString(), QStringLiteral("hidden"));
  QCOMPARE(byInd[0]["unit"].toString(), QStringLiteral("halfBlock"));

  QVERIFY(byInd.contains(1));
  QCOMPARE(byInd[1]["x"].toInt(), 16);
  QCOMPARE(byInd[1]["y"].toInt(), 42);
  QCOMPARE(byInd[1]["item"].toString(), QStringLiteral("ANTIDOTE"));

  // A hidden item is NOT an event flag and NOT a filter flag -- it is its own storage kind, with
  // its own save array. Nothing here should have been filed as either.
  QVERIFY(filterSpots(model.blockHotspots(), QStringLiteral("hiddenCoin")).isEmpty());
}

/// The coins say how many they are -- leadership: "for the coins itd be helpful to find out how
/// much your picking up like showing the amount".
void TestMap::hotspots_hiddenCoinsShowTheirAmount()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 135;   // Game Corner (0x87) -- every hidden coin in the game

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  const QVariantList coins = filterSpots(model.blockHotspots(), QStringLiteral("hiddenCoin"));
  QCOMPARE(coins.size(), 12);

  int total = 0;
  for (const QVariant& v : coins) {
    const QVariantMap m = v.toMap();
    QVERIFY2(m["coins"].toInt() > 0, "a coin pile with no coins in it is a parse failure");
    QVERIFY(m["name"].toString().contains(QString::number(m["coins"].toInt())));
    total += m["coins"].toInt();
  }
  QCOMPARE(total, 260);   // straight out of the ROM's `COIN + n` arguments
}

/// ⭐ THE ONE THAT MATTERS: an event flag reaches the map only through a located script, and the
/// chain is what makes it complete. Pallet Town's north row (`cp 1` on wYCoord) starts the Oak
/// cutscene, and the flags it eventually writes live in the routines it hands off to.
void TestMap::hotspots_eventFlagsAppearWhereTheirScriptHappens()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 0;   // Pallet Town

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  const QVariantList blocks = model.blockHotspots();
  const QVariantList events = filterSpots(blocks, QStringLiteral("eventFlag"));
  QVERIFY2(!events.isEmpty(), "Pallet Town's north row writes event flags; they must reach the map");

  // EVENT_OAK_APPEARED_IN_PALLET is written by the trigger itself; EVENT_DAISY_WALKING only by a
  // routine further down the chain. Both belong to this place -- standing there is what causes
  // them -- and the chained one must say so rather than pretending it fires on the tile.
  bool sawChained = false;
  for (const QVariant& v : events) {
    const QVariantMap m = v.toMap();
    QCOMPARE(m["section"].toString(), QStringLiteral("event"));
    QVERIFY(!m["name"].toString().isEmpty());
    QVERIFY(m["ind"].toInt() >= 0);
    if (m["viaChain"].toBool())
      sawChained = true;
  }
  QVERIFY2(sawChained, "the chain union is the whole point -- without it only 13 spots write events");

  // THE AGGREGATION RULE. The trigger is a whole ROW: its highlight spans the map's full width at
  // one half-block row, and it therefore appears on EVERY block that row crosses -- one tab each.
  // That is not duplication; it is the rule working.
  const QVariantList scripts = filterSpots(blocks, QStringLiteral("script"));
  QVERIFY(!scripts.isEmpty());
  const QVariantMap row = scripts.first().toMap();
  QCOMPARE(row["shape"].toString(), QStringLiteral("scriptRow"));
  QCOMPARE(row["extH"].toInt(), 16);   // one half-block tall: the row, truthfully
  QCOMPARE(row["extW"].toInt(),
           MapsDB::inst()->getIndAt(QStringLiteral("0"))->getWidth() * MapEngine::blockPx);

  QSet<int> blocksTouched;
  for (const QVariant& b : blocks)
    for (const QVariant& s : b.toMap()["spots"].toList())
      if (s.toMap()["kind"].toString() == QStringLiteral("script"))
        blocksTouched.insert(b.toMap()["blockX"].toInt());
  QVERIFY2(blocksTouched.size() > 1, "a row-wide trigger must be reachable from every block it crosses");
}

/// Tile traits are the 8x8 family, and they are gated on their layer being shown. Walls are on
/// nearly every block in the game: ungated, they would drown the storage tabs the strip exists for,
/// and "clutter is a bug".
void TestMap::hotspots_tileTraitsOnlyWhenTheirLayerIsShown()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 0;   // Pallet Town

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  // Nothing asked for -> no tile family at all.
  QVERIFY(filterSpots(model.blockHotspots(0), QStringLiteral("tileTrait")).isEmpty());

  // Ask for the doors -> the doors, and each one declares itself part of the 8x8 family.
  const QVariantList doors =
      filterSpots(model.blockHotspots(MapEngine::LayerDoors), QStringLiteral("tileTrait"));
  QVERIFY2(!doors.isEmpty(), "Pallet Town has doors");
  for (const QVariant& v : doors) {
    const QVariantMap m = v.toMap();
    QCOMPARE(m["unit"].toString(), QStringLiteral("tile"));  // 8x8 -- the genuinely tile-sized one
    // A door TRAIT is a tileset fact -> it opens the Tileset panel.
    QCOMPARE(m["section"].toString(), QStringLiteral("tiles"));
  }

  // ⚠️ PALLET TOWN'S GRASS PROMISES NOTHING, and that is the point.
  //
  // It has grass tiles and **no wild encounters** (`wGrassRate` 0 -- it is a town). So its grass
  // keeps the Tileset destination: *"The invalid areas do not need to display as having wild
  // pokemon."* This assertion used to demand "wild" here and failed, correctly -- the test was
  // wrong, not the rule.
  for (const QVariant& v : filterSpots(model.blockHotspots(MapEngine::LayerGrass),
                                       QStringLiteral("tileTrait")))
    QCOMPARE(v.toMap()["section"].toString(), QStringLiteral("tiles"));

  // ⭐ ...and where there ARE encounters, grass opens the WILD POKÉMON panel -- because that is what
  // grass IS, and that table is editable. Route 12 is the case, straight from the cartridge:
  // `Route12WildMons` is `def_grass_wildmons 15` (Oddish/Pidgey/Venonat) and, notably, has **no
  // water block at all** -- so its river needs Surf and holds nothing, and its water must NOT claim
  // otherwise. Both halves of the rule, on one map.
  //
  // ⚠️ It needs the FORGED SAVE, not `curMap = 23` on this one. The wild tables live in the SAVE's
  // Area block, so poking a map id gives you Route 12's map wearing Pallet Town's encounter tables
  // -- the chimera (reference/forged-saves.md), and `grassEnabled()` would answer for the wrong
  // town. This fixture was walked to by the real console, so its tables are Route 12's own.
  const QByteArray r12bytes = readSaveBytes(QStringLiteral("saves/forged-maps/Route12.sav"));
  SaveFile r12sf;
  loadInto(r12sf, r12bytes);
  QCOMPARE(int(r12sf.dataExpanded->area->map->curMap), 23);

  // ⚠️ The FULL constructor. `AreaPokemon` is the 10th argument and defaults to nullptr, and
  // `grassRate()` is `pokemon ? pokemon->grassRate : 0` -- so the short 4-arg form used elsewhere in
  // this file answers **0 encounters for every map in the game**, silently. That is not a test
  // detail: any caller that builds a MapModel without the encounter block gets a canvas where no
  // grass or water ever offers wild Pokémon.
  MapModel r12(r12sf.dataExpanded->area->map,
               r12sf.dataExpanded->area->player,
               r12sf.dataExpanded->area->tileset,
               r12sf.dataExpanded->area->general,
               nullptr, nullptr, nullptr, nullptr, nullptr,
               r12sf.dataExpanded->area->pokemon);

  QVERIFY2(r12.grassEnabled(), "Route 12's grass rate is 15 -- it has encounters");
  QVERIFY2(!r12.waterEnabled(), "Route 12 has NO water encounters (pret: no water block at all)");

  const QVariantList r12grass =
      filterSpots(r12.blockHotspots(MapEngine::LayerGrass), QStringLiteral("tileTrait"));
  QVERIFY(!r12grass.isEmpty());
  for (const QVariant& v : r12grass)
    QCOMPARE(v.toMap()["section"].toString(), QStringLiteral("wild"));

  // The water on that same map is real water -- and it must not promise Pokémon the console never
  // put there. THE LIE THIS PREVENTS is the whole reason the rule exists.
  for (const QVariant& v : filterSpots(r12.blockHotspots(MapEngine::LayerWater),
                                       QStringLiteral("tileTrait")))
    QCOMPARE(v.toMap()["section"].toString(), QStringLiteral("tiles"));

  // The rule, over every kind at once: a spot with no destination is a hole in the map.
  const QVariantList all = model.blockHotspots(MapEngine::LayerGrass | MapEngine::LayerWater
                                               | MapEngine::LayerDoors | MapEngine::LayerCounters);
  for (const QVariant& b : all)
    for (const QVariant& s : b.toMap()["spots"].toList())
      QVERIFY2(!s.toMap()["section"].toString().isEmpty(),
               qPrintable(QString("spot '%1' (%2) has no destination -- it would be inert")
                            .arg(s.toMap()["name"].toString(), s.toMap()["kind"].toString())));

  // ⚠️ ONE TAB PER TRAIT PER BLOCK. A block is 4x4 tiles, so a grassy block has SIXTEEN grass
  // tiles -- and sixteen identical "Grass" tabs, all going to the same place, disambiguate nothing
  // and stacked into an unbroken grey bar down the whole map. (The screenshot review caught it; the
  // code looked perfectly reasonable.) The truthful 8x8 highlight is not lost: MapEngine::overlay()
  // already paints every tile at its real size. The tab is the handle, the overlay is the highlight.
  const QVariantList grass =
      filterSpots(model.blockHotspots(MapEngine::LayerGrass), QStringLiteral("tileTrait"));
  QVERIFY2(!grass.isEmpty(), "Pallet Town has grass");
  QSet<QPair<int, int>> blocks;
  for (const QVariant& v : grass) {
    const QVariantMap m = v.toMap();
    const QPair<int, int> b{ m["extX"].toInt() / MapEngine::blockPx,
                             m["extY"].toInt() / MapEngine::blockPx };
    QVERIFY2(!blocks.contains(b), "a trait must appear ONCE per block, not once per tile");
    blocks.insert(b);
  }
  QCOMPARE(grass.size(), blocks.size());
}

/// ⭐ The map's story, phase by phase -- "(First phase flags) (second phase flags) ... which
/// scripts and locations did what to them turning them on off etc".
///
/// Pallet Town is the whole shape in one map: phase 0 turns Oak's flag ON, phase 1 SHOWS him,
/// phase 5 turns Daisy's flag on and swaps her sitting sprite for her walking one.
void TestMap::phases_tellTheMapsStoryInOrder()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  const QVariantList phases = model.mapPhases(QVariantList{ 0 });   // Pallet Town
  QVERIFY(!phases.isEmpty());

  QMap<int, QVariantMap> byStep;
  for (const QVariant& v : phases)
    byStep.insert(v.toMap()["step"].toInt(), v.toMap());

  // Phase 0 -- the trigger's own phase -- turns Oak's flag ON. Nothing else.
  QVERIFY(byStep.contains(0));
  QCOMPARE(byStep[0]["sets"].toList().size(), 1);
  QCOMPARE(byStep[0]["sets"].toList().first().toMap()["name"].toString(),
           QStringLiteral("Oak Appeared In Pallet"));
  QVERIFY(byStep[0]["resets"].toList().isEmpty());

  // Phase 1 SHOWS Oak -- a filter flag, and the verb is "show", never the inverted bit.
  QVERIFY(byStep.contains(1));
  QCOMPARE(byStep[1]["shows"].toList().size(), 1);
  QCOMPARE(byStep[1]["shows"].toList().first().toMap()["name"].toString(),
           QStringLiteral("Prof. Oak"));

  // Phase 5 swaps Daisy: hides the sitting one, shows the walking one. Both in ONE phase -- which
  // a set-of-names could not express, and which is why the action is kept per write.
  QVERIFY(byStep.contains(5));
  QCOMPARE(byStep[5]["hides"].toList().size(), 1);
  QCOMPARE(byStep[5]["hides"].toList().first().toMap()["name"].toString(),
           QStringLiteral("Daisy Sitting"));
  QCOMPARE(byStep[5]["shows"].toList().size(), 1);
  QCOMPARE(byStep[5]["shows"].toList().first().toMap()["name"].toString(),
           QStringLiteral("Daisy Walking"));

  // ⚠️ REGRESSION PIN: Daisy's SHOW is `predef_jump ShowObject`, not `predef ShowObject` -- the
  // tail-call form. A regex demanding `predef ` drops it silently, and Daisy is then recorded as
  // hidden and never shown again. 12 of the game's 83 Show/Hide sites use predef_jump.
  QVERIFY2(!byStep[5]["shows"].toList().isEmpty(),
           "predef_jump ShowObject must be recognised; see TOGGLE_RE");

  // Every phase names itself, so the story is readable rather than numbered.
  for (const QVariant& v : phases) {
    QVERIFY(!v.toMap()["label"].toString().isEmpty());
    QVERIFY(v.toMap()["step"].toInt() >= 0);
  }
}

/// One flag's whole story: who turns it on, in which phase, and where that can be triggered.
void TestMap::flagHistory_saysWhoTurnedItOnAndWhere()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  // Find "Oak Appeared In Pallet" by name rather than hard-coding a bit index.
  int oakFlag = -1;
  for (int i = 0; i < EventsDB::inst()->getStoreSize(); i++) {
    EventDBEntry* e = EventsDB::inst()->getStoreAt(i);
    if (e != nullptr && e->getName() == QStringLiteral("Oak Appeared In Pallet")) {
      oakFlag = i;
      break;
    }
  }
  QVERIFY(oakFlag >= 0);

  const QVariantList story = model.flagHistory(oakFlag, true);
  QVERIFY2(!story.isEmpty(), "this flag is set by Pallet Town's phase 0; its story cannot be empty");

  bool sawPallet = false;
  for (const QVariant& v : story) {
    const QVariantMap m = v.toMap();
    QVERIFY(m["action"].toString() == QStringLiteral("set")
            || m["action"].toString() == QStringLiteral("reset"));
    if (m["mapName"].toString() != QStringLiteral("Pallet Town"))
      continue;
    sawPallet = true;
    QCOMPARE(m["action"].toString(), QStringLiteral("set"));
    QCOMPARE(m["step"].toInt(), 0);
    // ...and it has a PLACE, because phase 0 is the located north-row trigger.
    const QVariantList at = m["locations"].toList();
    QVERIFY2(!at.isEmpty(), "phase 0 is the north-row trigger; it has a location");
    QCOMPARE(at.first().toMap()["kind"].toString(), QStringLiteral("scriptRow"));
    QCOMPARE(at.first().toMap()["y"].toInt(), 1);
  }
  QVERIFY(sawPallet);

  // A filter flag's story uses the game's VERB. Prof. Oak on Pallet Town is missable 0, and phase
  // 1 shows him -- "show", not "cleared the bit".
  const QVariantList oakObj = model.flagHistory(0, false);
  QVERIFY(!oakObj.isEmpty());
  bool sawShow = false;
  for (const QVariant& v : oakObj) {
    const QString a = v.toMap()["action"].toString();
    QVERIFY(a == QStringLiteral("show") || a == QStringLiteral("hide"));
    if (a == QStringLiteral("show"))
      sawShow = true;
  }
  QVERIFY(sawShow);
}

/// A block carries ONLY what overlaps it -- and the count stays sane.
///
/// Leadership asked the sharp question: *"a block should only have tabs for the things it overlaps,
/// scripts and stuff should have clear locations so why does a block have 40 tabs?"* -- prompted by
/// me miscounting out loud. It does not: Pallet Town's busiest block carries a handful. The row of
/// strips across the top is TEN blocks with FOUR tabs each, not one block with forty, and the row
/// spans them because the cartridge's trigger really is the whole row (`cp 1` on wYCoord, no X
/// test at all). This pins the real number so the question can never be answered by guesswork again.
void TestMap::hotspots_aBlockOnlyCarriesWhatOverlapsIt()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  sf.dataExpanded->area->map->curMap = 0;   // Pallet Town

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  // Storage only (no Tiles overlays): the busiest block is the north row's -- 1 script + 3 events.
  int worst = 0;
  for (const QVariant& b : model.blockHotspots(0))
    worst = qMax(worst, b.toMap()["spots"].toList().size());
  QCOMPARE(worst, 4);

  // Every spot on a block must actually intersect that block -- the aggregation rule, enforced.
  // A spot filed on a block it doesn't touch would be a tab that points somewhere else entirely.
  for (const QVariant& b : model.blockHotspots(MapEngine::LayerGrass | MapEngine::LayerWater)) {
    const QVariantMap m = b.toMap();
    const int bx = m["blockX"].toInt() * MapEngine::blockPx;
    const int by = m["blockY"].toInt() * MapEngine::blockPx;
    for (const QVariant& s : m["spots"].toList()) {
      const QVariantMap v = s.toMap();
      const int x0 = v["extX"].toInt(), x1 = x0 + v["extW"].toInt() - 1;
      const int y0 = v["extY"].toInt(), y1 = y0 + v["extH"].toInt() - 1;
      QVERIFY2(x1 >= bx && x0 <= bx + MapEngine::blockPx - 1
               && y1 >= by && y0 <= by + MapEngine::blockPx - 1,
               qPrintable(QString("spot '%1' is filed on a block it does not overlap")
                            .arg(v["name"].toString())));
    }
  }
}

/// The Map Storage panel's hidden-pickup rows: filed under the map they're actually on
/// ("all the spots there in means there listed under that map"), each naming what's buried.
void TestMap::storageHidden_listsThePickupsUnderTheirOwnMap()
{
  const QByteArray bytes = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  SaveFile sf;
  loadInto(sf, bytes);

  MapModel model(sf.dataExpanded->area->map,
                 sf.dataExpanded->area->player,
                 sf.dataExpanded->area->tileset,
                 sf.dataExpanded->area->general);

  // Route 12 -- the forged fixture's map (assets/saves/forged-maps/Route12.sav). It buries one
  // item, and the panel must list it under Route 12 and nowhere else.
  const QVariantList r12 = model.storageHidden(QVariantList{ 23 });
  QCOMPARE(r12.size(), 1);
  QCOMPARE(r12.first().toMap()["mapName"].toString(), QStringLiteral("Route 12"));
  QCOMPARE(r12.first().toMap()["isCoin"].toBool(), false);
  QVERIFY(!r12.first().toMap()["name"].toString().isEmpty());
  // The coords are the point: this is the one storage kind whose bit IS a place, exactly.
  QCOMPARE(r12.first().toMap()["x"].toInt(), 2);
  QCOMPARE(r12.first().toMap()["y"].toInt(), 63);

  // Viridian Forest buries two, and they are ITEMS, never coins.
  const QVariantList vf = model.storageHidden(QVariantList{ 51 });
  QCOMPARE(vf.size(), 2);
  for (const QVariant& v : vf)
    QCOMPARE(v.toMap()["isCoin"].toBool(), false);

  // The Game Corner buries all twelve coin piles, and no items.
  const QVariantList gc = model.storageHidden(QVariantList{ 135 });
  QCOMPARE(gc.size(), 12);
  for (const QVariant& v : gc) {
    QCOMPARE(v.toMap()["isCoin"].toBool(), true);
    QVERIFY(v.toMap()["coins"].toInt() > 0);
  }

  // A map with nothing buried says so with an empty list -- not a row saying "none".
  QVERIFY(model.storageHidden(QVariantList{ 0 }).isEmpty());   // Pallet Town
}

QTEST_MAIN(TestMap)
#include "tst_map.moc"
