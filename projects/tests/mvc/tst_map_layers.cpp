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
    r->map = new MapModel(area->map, area->player, area->tileset, area->general);
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

  // Meaning -- all nine semantic overlays
  const QVector<MapEngine::Layer> meaning = {
    MapEngine::LayerWalls, MapEngine::LayerGrass, MapEngine::LayerWater, MapEngine::LayerWarps,
    MapEngine::LayerDoors, MapEngine::LayerLedges, MapEngine::LayerCounters,
    MapEngine::LayerCutTrees, MapEngine::LayerElevation,
  };
  for (const MapEngine::Layer l : meaning)
    QVERIFY2(rowOf(r->layers, QStringLiteral("overlay%1").arg(static_cast<int>(l))) >= 0,
             qPrintable(QStringLiteral("meaning layer %1 is missing").arg(static_cast<int>(l))));
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

  // The DEFAULTS (2026-07-13, Twilight): the player and the screen box are on; the DRAW AREA is off
  // -- it is the engine's scratch, useful when you want it and clutter when you don't.
  QVERIFY2(r->layers->showPlayer(), "the player should be on by default");
  QVERIFY2(r->layers->showScreenBox(), "the screen box should be on by default");
  QVERIFY2(!r->layers->showDrawArea(), "the draw area should be OFF by default");
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
  QVERIFY2(visible(r->layers, row), "the connections should be on by default -- the ring is "
                                    "meaningless until you can see whose edges are in it");

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

  // A deliberately lopsided setup, so "restore" has something real to restore. (The draw area is
  // already off by default -- make sure of it rather than assuming either way.)
  if (visible(r->layers, draw))
    r->layers->toggle(draw);

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

QTEST_MAIN(TestMapLayers)
#include "tst_map_layers.moc"
