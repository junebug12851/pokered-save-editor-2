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
 * @file tst_map_layers.cpp
 * @brief The map's LAYER TREE (MapLayersModel) -- the groups, the tri-state eye, solo, and the one
 *        promise that matters most: **a layer never writes the save**.
 *
 * A layer is a way of LOOKING at the map, not a fact about it. The byte-fidelity rule
 * (context/principles.md -> "Save File Integrity Is Sacred") says the editor writes only the bytes
 * it was told to -- so switching a layer on has to leave all 32,768 of them exactly where they were.
 * `everyToggle_writesNotOneByte` proves that by flattening the whole save before and after and
 * comparing it byte for byte. It is the cheapest strong statement we can make, and it is the one a
 * user actually cares about.
 */
#include <QtTest>

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
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>
#include <mvc/mapmodel.h>
#include <mvc/maplayersmodel.h>

using namespace pse_test;

class TestMapLayers : public QObject
{
  Q_OBJECT

  QByteArray m_orig;

  /// Everything the model needs, over the real fixture save.
  struct Rig {
    SaveFile sf;
    MapModel* map = nullptr;
    MapLayersModel* layers = nullptr;
  };

  Rig* makeRig()
  {
    auto* r = new Rig;
    loadInto(r->sf, m_orig);

    auto* area = r->sf.dataExpanded->area;

    // ⚠️ The rig has to be the REAL thing. It used to build a MapModel with no sprites and no warps,
    // and the moment the Doors layer landed (2026-07-14) `groupEye_isTriState` went red -- not
    // because the tri-state broke, but because a layer for a thing the model had never been given
    // could never "apply", so "turn the whole group on" could never turn it on, and the group could
    // never reach "all".
    //
    // The bug was in the RIG, and it was the honest kind: a stripped-down fixture that quietly stops
    // resembling the app. Hand it what the app hands it.
    r->map = new MapModel(area->map, area->player, area->tileset, area->general,
                          area->preloadedSprites, area->sprites, area->warps,
                          r->sf.dataExpanded->world->general, area->signs);
    r->layers = new MapLayersModel(r->map);
    return r;
  }

  /// Find a row by its stable key.
  int rowOf(MapLayersModel* m, const QString& key)
  {
    for (int i = 0; i < m->rowCount(); i++)
      if (m->data(m->index(i, 0), MapLayersModel::KeyRole).toString() == key)
        return i;
    return -1;
  }

  bool visible(MapLayersModel* m, int row)
  {
    return m->data(m->index(row, 0), MapLayersModel::VisibleRole).toBool();
  }

  int groupState(MapLayersModel* m, int row)
  {
    return m->data(m->index(row, 0), MapLayersModel::GroupStateRole).toInt();
  }

private slots:
  void initTestCase();

  void theThreeGroups_areAllThere();
  void gameViewLayers_existAndToggle();
  void connectionsAreALayer();
  void groupEye_isTriState();
  void solo_isALookNotAnEdit();
  void aLayerWithNothingToShow_saysSo();
  void foldingAGroup_hidesItsChildrenNotItsState();
  void everyToggle_writesNotOneByte();

  void colourFilter_isTheGamesOwnSgbPalettePerMap();
  void colourFilter_writesNotOneByte();
};

void TestMapLayers::initTestCase()
{
  DB::inst();
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// Guides / Meaning / Game View -- and every child in them.
void TestMapLayers::theThreeGroups_areAllThere()
{
  QScopedPointer<Rig> r(makeRig());

  QVERIFY(rowOf(r->layers, "guides") >= 0);
  QVERIFY(rowOf(r->layers, "meaning") >= 0);
  QVERIFY(rowOf(r->layers, "gameview") >= 0);

  // Guides
  QVERIFY(rowOf(r->layers, "blockGrid") >= 0);
  QVERIFY(rowOf(r->layers, "tileGrid") >= 0);
  QVERIFY(rowOf(r->layers, "mapBounds") >= 0);
  QVERIFY2(rowOf(r->layers, QStringLiteral("overlay%1").arg(MapEngine::LayerBorder)) >= 0,
           "the border ring should be a layer in Guides");

  // Tiles (key still "meaning") -- the nine tile-meaning overlays. The WARP-TILE trait (which tile
  // graphics are warp-capable on this tileset) belongs here: it's a tileset fact, distinct from the
  // save's warp LIST (the draggable objects in Game View). A DOOR is likewise a passable tile type.
  const QVector<MapEngine::Layer> tiles = {
    MapEngine::LayerWalls, MapEngine::LayerGrass, MapEngine::LayerWater, MapEngine::LayerWarps,
    MapEngine::LayerDoors, MapEngine::LayerLedges, MapEngine::LayerCounters,
    MapEngine::LayerCutTrees, MapEngine::LayerElevation,
  };
  for (const MapEngine::Layer l : tiles)
    QVERIFY2(rowOf(r->layers, QStringLiteral("overlay%1").arg(static_cast<int>(l))) >= 0,
             qPrintable(QStringLiteral("tile-meaning layer %1 is missing").arg(static_cast<int>(l))));
}

/**
 * The whole point of the ask: the PLAYER, the RED screen box and the ACCENT draw area used to be
 * hard-coded rectangles with no switch at all. They are layers now, and they turn off.
 */
void TestMapLayers::gameViewLayers_existAndToggle()
{
  QScopedPointer<Rig> r(makeRig());

  const QStringList keys = { QStringLiteral("player"), QStringLiteral("screenBox"),
                             QStringLiteral("drawArea") };

  for (const QString& key : keys) {
    const int row = rowOf(r->layers, key);
    QVERIFY2(row >= 0, qPrintable(QStringLiteral("%1 is not a layer").arg(key)));

    // Each one toggles, whichever state it starts in -- that is what makes it a layer rather than a
    // rectangle painted on the map.
    const bool was = visible(r->layers, row);

    r->layers->toggle(row);
    QVERIFY2(visible(r->layers, row) != was,
             qPrintable(QStringLiteral("%1 did not toggle -- it is not really a layer").arg(key)));

    r->layers->toggle(row);
    QCOMPARE(visible(r->layers, row), was);
  }

  // The DEFAULTS (Twilight, 2026-07-15, amended 2026-07-17): the player, the people, the WARPS and
  // the signs are on; the screen box and the draw area are off.
  //
  // ⚠️ The SCREEN BOX is OFF as of 2026-07-17 ("disable camera view box by default ... the outline
  // around the player that would be exactly the gameboy screen view"). It used to be ON, and this
  // line used to assert exactly that -- so if you are reading this because the assert flipped, it
  // flipped ON PURPOSE.
  //
  // The DRAW AREA is a separate, older decision (Twilight, 3a22f84) and is unrelated to the above --
  // it has been off since it existed. Two boxes are off; only one of them was ever turned off.
  QVERIFY2(r->layers->showPlayer(), "the player should be on by default");
  QVERIFY2(r->layers->showNpcs(), "the people should be on by default");
  QVERIFY2(r->layers->showWarps(), "the warps (Game View object layer) should be on by default");
  QVERIFY2(r->layers->showSigns(), "the signs should be on by default");
  QVERIFY2(!r->layers->showScreenBox(), "the screen box should be OFF by default");
  QVERIFY2(!r->layers->showDrawArea(), "the draw area should be OFF by default");

  // And the whole Tiles group is OFF by default -- no tile-meaning overlay stands unasked.
  QCOMPARE(r->map->layers(), 0);
}

/// The neighbours' strips in the ring are a layer -- and on a map with no neighbours it says so.
void TestMapLayers::connectionsAreALayer()
{
  QScopedPointer<Rig> r(makeRig());

  const int row = rowOf(r->layers, "connections");
  QVERIFY2(row >= 0, "the connections are not a layer");

  // The fixture save is in Pallet Town, which connects to Route 1 (north) and Route 21 (south).
  const bool applies = r->layers->data(r->layers->index(row, 0),
                                       MapLayersModel::AppliesRole).toBool();
  QVERIFY2(applies, "Pallet Town has two connections -- the layer should apply");

  // ON by default (Twilight, 2026-07-15): you want to see how the map joins the world -- the
  // connections (and the connecting maps that render off the edges) are the point, not clutter.
  QVERIFY2(visible(r->layers, row), "the connections should be ON by default");
  QVERIFY(r->layers->showConnections());

  QVERIFY(!r->map->connectionList().isEmpty());

  r->layers->toggle(row);
  QVERIFY(!r->layers->showConnections());
}

/// One click on a group's eye always changes something: any child on -> all off; none on -> all on.
void TestMapLayers::groupEye_isTriState()
{
  QScopedPointer<Rig> r(makeRig());

  const int group = rowOf(r->layers, "gameview");
  QVERIFY(group >= 0);

  // Game View starts on SOME: the player and the screen box on, the draw area off (the default
  // changed 2026-07-13). Which makes it the perfect group to test the tri-state on.
  QCOMPARE(groupState(r->layers, group), 1);       // some

  r->layers->toggle(group);                        // any on -> everything off
  QCOMPARE(groupState(r->layers, group), 0);
  QVERIFY(!r->layers->showPlayer());
  QVERIFY(!r->layers->showScreenBox());
  QVERIFY(!r->layers->showDrawArea());

  r->layers->toggle(group);                        // none on -> everything on
  QCOMPARE(groupState(r->layers, group), 2);
  QVERIFY(r->layers->showDrawArea());

  // "Some" again, from the other side -- the group has to report it honestly (a folded group's eye
  // depends on this being right).
  r->layers->toggle(rowOf(r->layers, "player"));
  QCOMPARE(groupState(r->layers, group), 1);
}

/// Solo shows one layer alone in its group -- and puts back exactly what was on before.
void TestMapLayers::solo_isALookNotAnEdit()
{
  QScopedPointer<Rig> r(makeRig());

  const int player = rowOf(r->layers, "player");
  const int screen = rowOf(r->layers, "screenBox");
  const int draw   = rowOf(r->layers, "drawArea");

  // A deliberately lopsided setup, so "restore" has something real to restore. ARRANGE it, don't
  // inherit it: this test is about solo/un-solo, and what the defaults happen to be is
  // gameViewLayers_defaultToTheOnesYouWant's business, not ours. (It used to just assert the screen
  // box was on -- true until Twilight turned it off by default on 2026-07-17, at which point this
  // failed for a reason that had nothing to do with solo. A test that leans on a default it doesn't
  // own breaks when someone changes their mind about the UI, which they are entitled to do.)
  if (visible(r->layers, draw))
    r->layers->toggle(draw);
  if (!visible(r->layers, screen))
    r->layers->toggle(screen);
  if (!visible(r->layers, player))
    r->layers->toggle(player);

  QVERIFY(visible(r->layers, player));
  QVERIFY(visible(r->layers, screen));
  QVERIFY(!visible(r->layers, draw));

  r->layers->solo(screen);
  QVERIFY2(visible(r->layers, screen), "the soloed layer must be on");
  QVERIFY2(!visible(r->layers, player), "solo must switch off the rest of its group");
  QVERIFY2(!visible(r->layers, draw), "solo must switch off the rest of its group");

  r->layers->solo(screen);                         // un-solo
  QVERIFY2(visible(r->layers, player), "un-solo must put back what was on before");
  QVERIFY2(visible(r->layers, screen), "un-solo must put back what was on before");
  QVERIFY2(!visible(r->layers, draw), "un-solo must put back what was OFF before, too");
}

/// A layer with nothing to show says so, rather than switching on an empty overlay.
void TestMapLayers::aLayerWithNothingToShow_saysSo()
{
  QScopedPointer<Rig> r(makeRig());

  // The fixture save is in Pallet Town, which has no ledges and no cut trees.
  const int ledges = rowOf(r->layers, QStringLiteral("overlay%1")
                                        .arg(static_cast<int>(MapEngine::LayerLedges)));
  QVERIFY(ledges >= 0);

  const bool applies = r->layers->data(r->layers->index(ledges, 0),
                                       MapLayersModel::AppliesRole).toBool();
  QVERIFY2(!applies, "Pallet Town has no ledges -- the row should say so");

  // ...and clicking it does nothing, rather than lighting an empty overlay.
  r->layers->toggle(ledges);
  QVERIFY2(!visible(r->layers, ledges),
           "a layer with nothing to show must not switch on -- that is a lie the UI would be telling");
}

/// Folding a group hides its rows. It does NOT change what is drawn -- a folded group is tidy, not off.
void TestMapLayers::foldingAGroup_hidesItsChildrenNotItsState()
{
  QScopedPointer<Rig> r(makeRig());

  const int before = r->layers->rowCount();
  const int group = rowOf(r->layers, "gameview");

  QVERIFY(r->layers->showPlayer());

  r->layers->toggleExpanded(group);
  QVERIFY2(r->layers->rowCount() < before, "folding a group should hide its children");
  QVERIFY2(r->layers->showPlayer(), "folding a group must not switch its layers off");

  // The folded group's eye still tells the truth about what it holds -- "some", here, because the
  // draw area is off by default and the other two are on.
  QCOMPARE(groupState(r->layers, rowOf(r->layers, "gameview")), 1);

  r->layers->toggleExpanded(rowOf(r->layers, "gameview"));
  QCOMPARE(r->layers->rowCount(), before);
}

/**
 * THE ONE THAT MATTERS: toggle every layer there is, twice, plus every group, plus a solo, plus
 * clearAll -- and demand the save comes out **byte for byte identical**.
 *
 * A layer is how you LOOK at the map. If looking at it can change it, the editor is not trustworthy,
 * and no amount of UI polish fixes that.
 */
void TestMapLayers::everyToggle_writesNotOneByte()
{
  QScopedPointer<Rig> r(makeRig());

  // Flatten the save as it is now -- this is what a Save would write to disk.
  r->sf.dataExpanded->save(&r->sf);
  const QByteArray before = snapshot(r->sf);

  for (int i = 0; i < r->layers->rowCount(); i++) {
    r->layers->toggle(i);
    r->layers->toggle(i);
  }

  for (int i = 0; i < r->layers->rowCount(); i++)
    if (!r->layers->data(r->layers->index(i, 0), MapLayersModel::IsGroupRole).toBool())
      r->layers->solo(i);

  r->layers->clearAll();
  r->layers->setOverlayOpacity(0.5);

  r->sf.dataExpanded->save(&r->sf);
  const QByteArray after = snapshot(r->sf);

  QCOMPARE(after.size(), before.size());

  for (int i = 0; i < before.size(); i++) {
    if (before.at(i) != after.at(i)) {
      QFAIL(qPrintable(QStringLiteral(
        "a LAYER changed the save at 0x%1: %2 -> %3. Layers are a way of looking at the map, "
        "not a way of editing it.")
        .arg(i, 4, 16, QLatin1Char('0'))
        .arg(static_cast<quint8>(before.at(i)))
        .arg(static_cast<quint8>(after.at(i)))));
    }
  }
}

/// The Super Game Boy mode paints each map in the game's OWN palette -- Pallet green, a route's
/// green, a cave's brown -- straight from `data/sgb/sgb_palettes.asm`. Not a generic filter.
void TestMapLayers::colourFilter_isTheGamesOwnSgbPalettePerMap()
{
  MapEngine::setColourMode(MapEngine::SuperGameBoy);

  auto colour2 = [](int mapInd, int tilesetInd) {
    QRgb p[4];
    MapEngine::outputPaletteFor(mapInd, tilesetInd, p);
    return p[1];   // colour 1 is the one that carries a palette's character; 0 and 3 are near white/black
  };

  // Pallet Town (map 0, Overworld tileset 0) is PAL_PALLET: 25,28,27 -> a pale green-white.
  const QRgb pallet = colour2(0, 0);
  QVERIFY2(qGreen(pallet) > qBlue(pallet) && qGreen(pallet) > 180,
           "Pallet Town's SGB colour 1 should be its pale green");

  // Vermilion (city map 5) is PAL_VERMILION: 30,18,0 -> a strong orange. It must DIFFER from Pallet:
  // the whole point is that each map gets its own.
  const QRgb verm = colour2(5, 0);
  QVERIFY2(verm != pallet, "each city must get its OWN palette -- Vermilion != Pallet");
  QVERIFY2(qRed(verm) > qBlue(verm) && qRed(verm) > 200, "Vermilion's SGB colour 1 should be orange");

  // A cave (tileset 17 = CAVERN) is PAL_CAVE whatever its map id -- the tileset wins.
  const QRgb cave = colour2(0x40, 17);
  QVERIFY2(qRed(cave) > qBlue(cave), "a cave should read warm/brown (PAL_CAVE), by tileset");

  // Back to the default so we leave no state behind for the next test.
  MapEngine::setColourMode(MapEngine::Grey);
}

/// ⚠️ THE PROMISE. The colour filter is a VIEW setting -- every mode, and a custom colour, must leave
/// the save byte-for-byte identical. Same guarantee the layers make, for the same reason.
void TestMapLayers::colourFilter_writesNotOneByte()
{
  QScopedPointer<Rig> r(makeRig());

  r->sf.dataExpanded->save(&r->sf);
  const QByteArray before = snapshot(r->sf);

  for (int mode : { MapEngine::GameBoy, MapEngine::SuperGameBoy, MapEngine::Custom, MapEngine::Grey })
    r->map->setColourMode(mode);

  r->map->setCustomColour(0, QColor(Qt::red));
  r->map->setCustomColour(3, QColor(Qt::blue));

  r->sf.dataExpanded->save(&r->sf);
  const QByteArray after = snapshot(r->sf);

  QCOMPARE(after.size(), before.size());

  for (int i = 0; i < before.size(); i++) {
    if (before.at(i) != after.at(i)) {
      QFAIL(qPrintable(QStringLiteral(
        "the COLOUR FILTER changed the save at 0x%1: %2 -> %3. It is a way of looking at the map, "
        "not a way of editing it.")
        .arg(i, 4, 16, QLatin1Char('0'))
        .arg(static_cast<quint8>(before.at(i)))
        .arg(static_cast<quint8>(after.at(i)))));
    }
  }

  MapEngine::setColourMode(MapEngine::Grey);
}

QTEST_MAIN(TestMapLayers)
#include "tst_map_layers.moc"
