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
#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QString>
#include <QVector>

class MapModel;

/**
 * @brief The map's LAYERS -- grouped, toggleable, and the map screen's legend.
 *
 * Exposed as `brg.mapLayers`. Three groups today (a fourth, Objects, arrives with the objects
 * themselves -- notes/plans/map-screen.md):
 *
 *  - **Guides**    -- the block grid, the tile grid, the map bounds, the border ring.
 *  - **Meaning**   -- walls, grass, water, warps, doors, ledges, counters, cut trees, elevation
 *                     edges: the nine semantic overlays MapEngine renders.
 *  - **Game View** -- the player, the **red screen box** (the 20x18 tiles the Game Boy is actually
 *                     showing) and the **accent draw area** (the 6x5 blocks LoadCurrentMapView
 *                     redraws). These three were hard-coded rectangles you could not switch off.
 *                     They are layers now, exactly as Twilight asked.
 *
 * It is a **tree flattened into a list** (`isGroup` + `depth` roles). A QML `TreeView` is not worth
 * its weight for a fixed three-group tree, and a flat model is far easier to test.
 *
 * ⚠️ **This model NEVER writes the save.** A layer is a way of looking at the map, not a fact about
 * it. `tst_map_layers` asserts that (byte-diffing the whole save across every toggle).
 *
 * Two kinds of layer live behind one interface:
 *  - **overlay** layers are `MapEngine::Layer` bits, rendered into the overlay image by C++ (they
 *    have to be: Route 17 is 20,000+ tiles). They route to MapModel::layers.
 *  - **view** layers are drawn by QML over the map (the grids, the boxes, the player sprite). They
 *    live in @ref viewBits, and QML binds the individual `show*` properties.
 */
class MapLayersModel : public QAbstractListModel
{
  Q_OBJECT

  /// The view-drawn layers, as a bit set (@ref ViewLayer). Purely a view setting.
  Q_PROPERTY(int viewBits READ viewBits NOTIFY viewBitsChanged)

  // The individual bindings QML actually uses. (A bitmask is the model's business; a QML file
  // should say `visible: brg.mapLayers.showBlockGrid`, not do arithmetic.)
  Q_PROPERTY(bool showBlockGrid READ showBlockGrid NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showTileGrid READ showTileGrid NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showMapBounds READ showMapBounds NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showPlayer READ showPlayer NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showScreenBox READ showScreenBox NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showDrawArea READ showDrawArea NOTIFY viewBitsChanged)
  Q_PROPERTY(bool showConnections READ showConnections NOTIFY viewBitsChanged)

  /// How strongly the meaning overlay is drawn (0..1). The one dial the Meaning group carries:
  /// stacked annotation over four shades of grey genuinely needs it.
  Q_PROPERTY(qreal overlayOpacity READ overlayOpacity WRITE setOverlayOpacity NOTIFY overlayOpacityChanged)

public:
  /// The QML-drawn layers. (The overlay layers are `MapEngine::Layer` and live in MapModel.)
  enum ViewLayer {
    ViewNone      = 0,
    ViewBlockGrid = 1 << 0,   ///< The 32px cells a map is really made of.
    ViewTileGrid  = 1 << 1,   ///< The 8px tiles inside them. Off by default -- it is a lot of lines.
    ViewMapBounds = 1 << 2,   ///< Where the map ends and the 3-block ring begins.
    ViewPlayer    = 1 << 3,   ///< Him, where the console's own OAM puts him.
    ViewScreenBox = 1 << 4,   ///< The 20x18 tiles actually on the Game Boy's screen.
    ViewDrawArea  = 1 << 5,   ///< The 6x5 blocks LoadCurrentMapView redraws.
    /// Where each CONNECTED map bleeds into the ring -- and how big that strip is. The ring is not a
    /// wall of trees: it carries the neighbours' edges, and working out where each lands is the
    /// hardest arithmetic in the engine. This is the layer that lets you look at it.
    ViewConnections = 1 << 6,
  };
  Q_ENUM(ViewLayer)

  enum Roles {
    NameRole = Qt::UserRole + 1,
    DescriptionRole,   ///< What it is, in words. Nobody should have to already know what a counter is.
    IsGroupRole,       ///< A group header row (Guides / Meaning / Game View).
    VisibleRole,       ///< This row's own eye.
    GroupStateRole,    ///< For a group: 0 none / 1 some / 2 all of its children are visible.
    AppliesRole,       ///< False when this map has none of it -- the row says so instead of lying.
    ColorRole,         ///< The exact ink the renderer uses. The ROW is the legend.
    ExpandedRole,      ///< For a group: is it unfolded?
    KeyRole,           ///< A stable id, for the tests and the DEBUG harness.
  };

  explicit MapLayersModel(MapModel* map, QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int viewBits() const;
  bool showBlockGrid() const;
  bool showTileGrid() const;
  bool showMapBounds() const;
  bool showPlayer() const;
  bool showScreenBox() const;
  bool showDrawArea() const;
  bool showConnections() const;

  qreal overlayOpacity() const;
  void setOverlayOpacity(qreal opacity);

  /// Toggle the row's eye. A group row toggles ALL of its children (off if any are on -- one click
  /// always changes something, which is what a tri-state eye has to guarantee).
  Q_INVOKABLE void toggle(int row);

  /// Solo: this layer, alone in its group. Doing it again restores what was on before -- so it is a
  /// look, not a destructive edit of your setup. (Alt-click the eye; Photoshop's gesture.)
  Q_INVOKABLE void solo(int row);

  /// Fold a group away. Clutter is a bug; a group you are not using takes one line.
  Q_INVOKABLE void toggleExpanded(int row);

  /// Everything off, in every group. (The map is the point; this is the way back to it.)
  Q_INVOKABLE void clearAll();

  /// Is anything at all switched on? (Drives the "Clear" affordance.)
  Q_INVOKABLE bool anyOn() const;

signals:
  void viewBitsChanged();
  void overlayOpacityChanged();

private:
  /// One row of the flattened tree.
  struct Row {
    QString key;
    QString name;
    QString description;
    bool isGroup = false;
    int group = 0;          ///< Which group this row belongs to (or IS).
    int overlayBit = 0;     ///< MapEngine::Layer bit, or 0 for a view layer.
    int viewBit = 0;        ///< ViewLayer bit, or 0 for an overlay layer.
  };

  void buildAll();                ///< Every layer there is, folded or not. Built once.
  void rebuild();                 ///< Re-flatten what is SHOWN (a group folded/unfolded).
  bool rowVisible(const Row& r) const;
  bool rowApplies(const Row& r) const;
  void setRowVisible(const Row& r, bool on);
  void refreshAll();              ///< Everything's eye may have changed -- redraw every row.

  MapModel* map = nullptr;

  /// EVERY layer, folded groups included. A folded group still has to tell the truth about what it
  /// holds -- its eye says "all/some/none" whether or not its children are on screen -- so the
  /// state can never be computed from the rows that merely happen to be VISIBLE. (That bug was
  /// written, and caught by tst_map_layers::foldingAGroup_hidesItsChildrenNotItsState.)
  QVector<Row> allRows;

  QVector<Row> rows;              ///< The flattened tree, as shown.
  QVector<bool> expanded;         ///< Per group.

  // The solo snapshot: what was on before, so soloing is a LOOK and not a destructive edit of the
  // setup you had. -1 = nothing is soloed.
  QString soloKey;
  int savedView = 0;
  int savedOverlay = 0;

  // What is on when the screen opens. The DRAW AREA and the CONNECTIONS are both OFF (Twilight,
  // 2026-07-13): they are things you go looking for, not things that should be sitting on top of the
  // map before you have asked for them. The map is the point.
  int bits = ViewBlockGrid | ViewMapBounds | ViewPlayer | ViewScreenBox;
  qreal opacity = 1.0;
};
