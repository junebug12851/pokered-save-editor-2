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
 * ONE BLOCK of the map that has persistent storage on it -- and the tabbed squares that reach
 * everything landing there.
 *
 * > *"If a map tile/block whatever has multiple persistent map storage spots for it they need to
 * >  show as tabbed little squares at the top left that can be clicked on and theyll take you to
 * >  the different spots"* -- Fairy Fox, 2026-07-17
 *
 * ── The two granularities, and keeping them apart IS the design ──────────────────────────────
 *
 * (Leadership's settled call, 2026-07-17: *"why dont we highlight truthfully the ranges and have
 * clicks and mouse overs be on the blocks to keep things simple"*.)
 *
 *  - **The HIGHLIGHT is the truth.** Each spot draws at its OWN real size: a tile trait 8x8, an
 *    object or script tile 16x16, a coord range exactly the row or column it really covers. Drawn
 *    from `extX/extY/extW/extH`, which the model measures in the spot's own unit.
 *  - **The HIT TARGET is the block.** Uniform 32x32, one cursor cell, one tab strip. It claims to
 *    be nothing except a cell -- which is why a block-sized target isn't a lie about a 16x16 thing.
 *
 * ── The tabs ─────────────────────────────────────────────────────────────────────────────────
 *
 * Square colour tabs, and the two families sit on **opposite sides**, split by UNIT:
 * **non-tile tabs on the LEFT** (filter flags, script locations, coord ranges, hidden items,
 * event flags -- all on the 16x16 walk grid) and **tile traits on the RIGHT** (Door, Warp tiles,
 * grass, water, counters -- the 8x8 family).
 *
 * A tab is **the escape hatch**: it reaches its spot **directly, whatever is stacked above it**, so
 * a buried object is never unreachable and the grid needs no gating or special-casing.
 * Deliberately ONE spot = no tabs (a plain box, as before) -- tabs are for disambiguation, and a
 * strip of one disambiguates nothing. Clutter is a bug.
 *
 * @see MapModel::blockHotspots, notes/plans/map-screen.md -> Phase 16f
 */
import QtQuick
import QtQuick.Controls

Item {
  id: hot

  /// The canvas, for the zoom.
  required property var canvas

  /// One entry of MapModel::blockHotspots: `{blockX, blockY, rectX/Y/W/H, spots:[...]}`.
  required property var block

  /// A spot was reached -- open Map Storage at @p section, on row @p ind.
  signal spotClicked(string section, int ind)

  /// The spot kinds whose LAYER is currently on. A tab exists only for a layer that is showing --
  /// leadership, 2026-07-17: *"the tab strip only needs to have tabs based on the active layers"*.
  ///
  /// This is the same doctrine that decides the click: **priority IS the Layers panel's own order**,
  /// and a layer that is off draws nothing and catches nothing, so a tab into it would be pointing
  /// at something that isn't on screen. It also keeps the strip honest about clutter -- turn a layer
  /// off and its tabs go with it.
  required property var activeKinds

  /// Only what the active layers admit. Everything below -- the tabs, the gap, the single-spot hit
  /// target -- reads THIS, never the raw list, so a hidden layer can never be reached by accident.
  readonly property var spots: hot.block.spots.filter(s => hot.activeKinds.indexOf(s.kind) >= 0)

  /// The tile-based family (8x8 tile TRAITS) and the walk-grid family, told apart by the spot's own
  /// unit. The gap between them is the whole point of the split, and the unit is what defines it --
  /// which is exactly why `blockHotspots` carries a unit per spot.
  readonly property var tileSpots: hot.spots.filter(s => s.unit === "tile")
  readonly property var gridSpots: hot.spots.filter(s => s.unit !== "tile")

  /// Tabs appear only when there is more than one thing here to tell apart.
  readonly property bool tabbed: hot.spots.length > 1

  // A block whose every spot belongs to a switched-off layer is not a block with nothing on it --
  // it is a block you asked not to see. Either way it draws nothing.
  visible: hot.spots.length > 0

  // The BLOCK is the hit target: uniform, 32x32, the cursor cell.
  x: hot.block.rectX * hot.canvas.zoom
  y: hot.block.rectY * hot.canvas.zoom
  width: hot.block.rectW * hot.canvas.zoom
  height: hot.block.rectH * hot.canvas.zoom

  // Under the real objects: this annotates them, it must never cover them.
  z: 0

  /// The layer's ink, by what the spot is attached to -- her rule: a thing tied to a flag does not
  /// look like a thing that isn't. Okabe-Ito throughout, and each one is the swatch its own Layers
  /// row paints, so the row IS the legend.
  function inkOf(kind) {
    switch (kind) {
      case "filterFlag":  return "#009e73";  // Flag boxes -- bluish green
      case "eventFlag":   return "#56b4e9";  // Event flags -- sky blue
      case "script":      return "#e69f00";  // Script triggers -- orange
      case "cardKeyDoor": return "#d55e00";  // Card Key doors -- vermillion
      case "hiddenItem":  return "#cc79a7";  // Hidden items -- reddish purple
      case "hiddenCoin":  return "#f0e442";  // Hidden coins -- yellow
      case "tileTrait":   return "#999999";  // Tile traits -- the quiet family
    }
    return "#009e73";
  }

  /// Is this spot's object currently switched OFF by the save? Only a filter flag can be, and it is
  /// read live so flipping a switch in Map Storage re-dashes one box instead of rebuilding the list.
  /// (WorldMissables: **bit set = HIDDEN**.)
  function isHidden(spot) {
    if (spot.kind !== "filterFlag")
      return false;
    hot.canvas.revision;
    return hot.canvas.wMissables ? hot.canvas.wMissables.missablesAt(spot.ind) : false;
  }

  /// Can you MOVE this thing -- drag it, delete it, put a new one down?
  ///
  /// > *"stuff draggable, deletable, insertable, etc.... should have a solid fill and box, stuff
  /// >  that allows editing things from fixed locations like scripts and stuff should have a dashed
  /// >  outline and not be filled."*
  ///
  /// Nothing this component draws is movable **today**: a script trigger sits where the cartridge
  /// tests the player's coords, and a hidden pickup's tile *is* its save bit's identity -- you edit
  /// what they do, never where they are. So this answers false for every current kind, and the
  /// layer is honestly all-dashed.
  ///
  /// It is written as a rule rather than hard-coded to `false` because the movable kinds -- warps,
  /// signs, people -- are persistent storage with locations too, and when they join `spots[]` this
  /// is the one line that has to know. @see MapWarp, MapSign, MapSprite
  function isMovable(kind) {
    return kind === "warp" || kind === "sign" || kind === "sprite";
  }

  /// ⭐ The BLOCK's outline style, decided by the block and not by each spot:
  ///
  /// > *"if there are multiple tabs fill and use solid line only if any of the tabs contain one
  /// >  thing that matches the rules above for solid fill and solid outline"*
  ///
  /// So one movable thing among many makes the whole box solid and filled -- because the box is the
  /// handle for the CELL, and if anything in that cell can be picked up then the cell can be picked
  /// up. Mixing a dashed box with a solid one inside a single cell would be saying two things about
  /// the same target.
  readonly property bool anyMovable: hot.spots.some(s => hot.isMovable(s.kind))

  // ── The HIGHLIGHTS -- each spot at its own true size, drawn ONCE ───────────────────────────
  //
  // Positioned relative to this block, but sized from the spot's real extent, so a coord range
  // legitimately draws far outside the block that owns this strip: the range IS that wide, and
  // shrinking it to fit the block would be a lie about the data.
  //
  // ⚠️ ONLY the block containing the spot's TOP-LEFT draws it. A spot is filed on every block its
  // extent touches -- that is the aggregation rule, and it is right for the TABS -- but the
  // highlight is one thing in the world, not one per cell. Without this guard Pallet Town's
  // north-row trigger drew its full-width rectangle TEN TIMES, once per block it crosses: the
  // translucent range stacked into a solid bar and the edges got ten times the ink. Caught by the
  // mandatory screenshot review, which is exactly the class of thing a glance misses.
  //
  // ⚠️ TILE TRAITS draw NO box of their own. `MapEngine::overlay()` already paints every one of
  // their tiles at its true 8x8, in the layer's own colour -- so a box here would be a second
  // answer to a question already answered, and one that has to agree with the first forever. Their
  // tab is a handle; the overlay is the highlight.
  readonly property var ownSpots: hot.spots.filter(s =>
      s.kind !== "tileTrait"
      && Math.floor(s.extX / 32) === hot.block.blockX
      && Math.floor(s.extY / 32) === hot.block.blockY)

  Repeater {
    model: hot.ownSpots

    delegate: Item {
      id: spotItem
      required property var modelData

      /// Is the save currently switching this object OFF? (Only a filter flag can be.)
      readonly property bool dashed: hot.isHidden(modelData)

      x: (modelData.extX - hot.block.rectX) * hot.canvas.zoom
      y: (modelData.extY - hot.block.rectY) * hot.canvas.zoom
      width: modelData.extW * hot.canvas.zoom
      height: modelData.extH * hot.canvas.zoom

      // ── DASHED, NEVER FILLED, and that is now a RULE, not a look ────────────────────────────
      //
      // > *"stuff draggable, deletable, insertable, etc.... should have a solid fill and box, stuff
      // >  that allows editing things from fixed locations like scripts and stuff should have a
      // >  dashed outline and not be filled."* -- Fairy Fox, 2026-07-17
      //
      // ⭐ So the outline tells you what you can DO before you touch it. Everything this component
      // draws is FIXED: a script trigger sits where the cartridge tests the player's coords, and a
      // hidden item's tile IS its save bit's identity -- you can edit what they do, never where they
      // are. Nothing here is draggable, so nothing here is filled. The solid-and-filled language
      // belongs to the objects you can pick up: warps, signs, people (MapWarp / MapSign / MapSprite).
      //
      // ⚠️ This RETIRES the old solid-vs-dashed meaning ("solid = shown, dashed = hidden"), which
      // used the same ink for a different question. A switched-off object now says so with the ⚑ and
      // a fainter line -- the outline STYLE is reserved for "can I move this?".
      //
      // The decision is the BLOCK's, not this spot's: one movable thing in the cell makes the whole
      // cell solid and filled. @see hot.anyMovable

      // The SOLID, FILLED box -- only when something in this cell can be picked up.
      Rectangle {
        anchors.fill: parent
        visible: hot.anyMovable
        // A light wash of the layer's own ink -- present enough to say "solid fill", faint enough
        // that the artwork underneath is still the thing you look at. (A heavy wash tinted every
        // Poké Ball in Oak's Lab green once, and the screenshot review caught it.)
        color: {
          const c = Qt.color(hot.inkOf(modelData.kind));
          return Qt.rgba(c.r, c.g, c.b, 0.18);
        }
        border.width: Math.max(1, Math.round(hot.canvas.zoom))
        border.color: hot.inkOf(modelData.kind)
        radius: 2
      }

      Canvas {
        id: dashes
        anchors.fill: parent
        visible: !hot.anyMovable
        // A switched-off object is fainter; a range is a broad claim and says so quietly.
        opacity: spotItem.dashed ? 0.5
                                 : (modelData.kind === "script" && modelData.shape !== "scriptTile"
                                      ? 0.55 : 1.0)
        // The dashes are painted, not bound, so a size change has to ask for a repaint by hand --
        // otherwise zooming leaves the old rectangle stretched.
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
          const ctx = getContext("2d");
          ctx.reset();
          const w = Math.max(1, Math.round(hot.canvas.zoom));
          ctx.lineWidth = w;
          ctx.strokeStyle = hot.inkOf(modelData.kind);
          ctx.setLineDash([Math.max(2, 3 * hot.canvas.zoom), Math.max(2, 3 * hot.canvas.zoom)]);
          ctx.strokeRect(w / 2, w / 2, width - w, height - w);
        }
        // The dashes must be repainted when the zoom changes them.
        //
        // ⚠️ `dashes.requestPaint()`, NOT `parent.requestPaint()`. Inside a Connections nested in a
        // REPEATER DELEGATE, `parent` does not resolve to the enclosing Canvas -- it walks out to
        // the delegate's root (MapBlockHotspot), which has no such method, and every zoom throws
        // `TypeError: Property 'requestPaint' ... is not a function`. The same line worked verbatim
        // in MapFlagBox, which was not inside a delegate. Caught by tst_qml_screens, which is the
        // only thing in the suite that instantiates QML -- a silent binding error that no C++ test
        // could ever see.
        Connections {
          target: hot.canvas
          function onZoomChanged() { dashes.requestPaint(); }
        }
      }

      // ⚑ -- ONLY on an empty box. When the object is there, its own artwork is the thing worth
      // seeing and a glyph over its face is just in the way; the outline already says "the game
      // remembers this". When the flag hides it there IS no sprite, and the mark is what tells you
      // something belongs here.
      Text {
        anchors.centerIn: parent
        visible: spotItem.dashed && hot.canvas.zoom >= 1.5
        text: "⚑"
        font.pixelSize: Math.max(8, Math.round(9 * hot.canvas.zoom))
        color: hot.inkOf(modelData.kind)
      }
    }
  }

  // ── The TAB STRIPS -- square colour tabs, and the two families sit on OPPOSITE SIDES ───────
  //
  // > *"there is a gap in the tabs to seperate the tile-based tabs and non-tile-based tabs. door
  // >  and warp tile traits would be an example of tile tabs seperated from non-tile tabs like
  // >  filter flags or script locations or coord ranges these would be the tab tags on the left"*
  // > ...and, when the first cut put both on the left with a gap between them:
  // > *"i said the tile traits go right not left i said the non tile traits go left"*
  //
  // So the separation is a SIDE, not a gap in one column: **non-tile tabs LEFT, tile traits
  // RIGHT**. Which reads well -- the two families can never be mistaken for one strip, and you
  // always know which side to look at for which kind.

  /// NON-TILE (the walk grid): filter flags, script locations, coord ranges, hidden items, events.
  Column {
    id: gridStrip
    visible: hot.tabbed && hot.gridSpots.length > 0
    spacing: 1
    // Outside the cell, so a tab never covers the thing it points at.
    x: -width - 2
    y: 0
    z: 2   // above the highlights AND the objects: a tab must always be reachable

    Repeater { model: hot.gridSpots; delegate: tabDelegate }
  }

  /// TILE-BASED (8x8 traits): Door, Warp tiles, grass, water, counters.
  Column {
    id: tileStrip
    visible: hot.tabbed && hot.tileSpots.length > 0
    spacing: 1
    x: hot.width + 2
    y: 0
    z: 2

    Repeater { model: hot.tileSpots; delegate: tabDelegate }
  }

  Component {
    id: tabDelegate

    Rectangle {
      id: tab
      required property var modelData

      width: Math.max(4, Math.round(5 * hot.canvas.zoom))
      height: width
      radius: 1
      color: hot.inkOf(modelData.kind)
      border.width: 1
      border.color: Qt.darker(color, 1.6)
      // A tab for a switched-off object is hollow -- the same language the box itself speaks.
      opacity: hot.isHidden(modelData) ? 0.45 : 1.0

      // A MouseArea, never a PointerHandler -- a TapHandler fires THROUGH a floating panel, which
      // cost a whole review round once. See notes/reference/qt-patterns.md (top of file).
      MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        // A tile trait is a TILESET fact with no Map Storage row to open; its tab is a label and a
        // hover target, not a link. Saying so by not pretending to be clickable.
        enabled: modelData.section !== ""
        onClicked: hot.spotClicked(modelData.section, modelData.ind)

        ToolTip.visible: containsMouse
        ToolTip.delay: 300
        ToolTip.text: {
          let t = modelData.name;
          if (modelData.kind === "eventFlag") {
            t += modelData.action === "reset" ? qsTr("\nTurned OFF here.") : qsTr("\nTurned ON here.");
            if (modelData.viaChain)
              t += qsTr("\n(Later in the sequence this spot starts.)");
          } else if (modelData.kind === "filterFlag") {
            t += hot.isHidden(modelData) ? qsTr("\nYour save is hiding this.")
                                         : qsTr("\nYour save is showing this.");
          }
          if (modelData.desc && modelData.desc.length > 0)
            t += "\n\n" + modelData.desc;
          if (modelData.section !== "")
            t += qsTr("\n\nClick to open it in Map Storage.");
          return t;
        }
      }
    }
  }

  // ── The block's own hit target ─────────────────────────────────────────────────────────────
  //
  // Only when there is exactly ONE spot here: then the whole cell opens it, which is the plain-box
  // behaviour from before. With several, the tabs are how you say WHICH -- a cell that guessed for
  // you would be picking one of them behind your back.
  MouseArea {
    id: hit
    anchors.fill: parent
    hoverEnabled: true
    enabled: !hot.tabbed && hot.spots.length === 1 && hot.spots[0].section !== ""
    cursorShape: Qt.PointingHandCursor
    onClicked: hot.spotClicked(hot.spots[0].section, hot.spots[0].ind)

    ToolTip.visible: hit.containsMouse && hit.enabled
    ToolTip.delay: 400
    ToolTip.text: {
      if (hot.spots.length !== 1)
        return "";
      const s = hot.spots[0];
      let t = s.name;
      if (s.kind === "filterFlag")
        t += hot.isHidden(s) ? qsTr("\nYour save is hiding this.") : qsTr("\nYour save is showing this.");
      if (s.desc && s.desc.length > 0)
        t += "\n\n" + s.desc;
      t += qsTr("\n\nClick to open it in Map Storage.");
      return t;
    }
  }
}
