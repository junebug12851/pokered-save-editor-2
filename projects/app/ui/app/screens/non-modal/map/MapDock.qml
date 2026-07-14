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

/*
  MapDock.qml -- an icon RAIL and, at most, ONE open panel. There are two of them now.

    * the LEFT dock holds LAYERS -- the map's legend and its navigation (Twilight, 2026-07-13);
    * the RIGHT dock holds the panels that EDIT things.

  The rules, and each one is a rule Twilight asked for:

    * PANELS DO NOT STACK. One panel at a time, in the same place, every time. Nothing is ever
      evicted behind your back (the first cut had an eviction QUEUE -- a workaround for a layout
      that was never designed to hold this much).
    * IT COLLAPSES TO NOTHING. Click the lit icon and the panel folds away, leaving the rail.
    * THE MAP NEVER MOVES. A panel ALWAYS floats over the canvas -- it never pushes it.

  ⚠️ **The panel always floats.** It used to be seated *beside* the map when there was room and float
  only when there wasn't, and the cost of that was: open a panel and the canvas resized, so the map
  re-laid-out and jumped under the cursor. Twilight, 2026-07-13: *"Map does not need to resize on
  panel changes."* It doesn't. The dock takes the RAIL's width out of the row and nothing else; the
  panel is drawn over the canvas edge with a shadow, and swallows its own clicks, wheel and hover so
  nothing falls through to the map behind.

  A panel may put ONE action in the title bar (the Layers panel's Clear) by declaring a
  `headerAction` Component. @see LayersPanel.qml.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: dock

  /// Which edge this dock lives on: "left" or "right".
  property string side: "right"
  readonly property bool isLeft: side === "left"

  /// The panels this dock offers, in rail order: `{ id, glyph, title, tip }`. Glyphs, not SVGs --
  /// this app's chrome is flat marks, not a tray of tiny system icons.
  property var panels: []

  /// The open panel's id, or "" for none. ONE at a time, by design.
  property string open: ""

  /// Which QML file each panel id loads. Supplied by the screen.
  property var sources: ({})

  /// Handed to any loaded panel that declares a `canvas` property -- the Details panel edits
  /// whatever is selected on the canvas, and a Loader cannot see the ids around it.
  property var panelContext: null

  /// How wide the panel column is when open (drag its inner edge; remembered for the session).
  ///
  /// ⚠️ It has been too narrow **twice**. 170 clipped text outright; 200 still left a group row
  /// (chevron + eye + name + a per-group Clear) fighting for space, so "Components" came back cut
  /// off -- and the 16px scrollbar lane, which content must now reserve, takes another bite.
  ///
  /// 240. The map does not get squeezed for it (the panel floats over the canvas), so the only thing
  /// a wider panel costs is a little of the map you are looking *past*, and the only thing a narrow
  /// one buys is elided words. That is not a trade worth making.
  property int panelWidth: 240
  readonly property int railWidth: 40
  readonly property int minPanelWidth: 190
  readonly property int maxPanelWidth: 380

  /// ⚠️ ALWAYS. The panel is never seated in the row -- see the note at the top of this file. Opening
  /// one must not move the map.
  readonly property bool overlayMode: dock.open !== ""

  /// What the dock takes out of the row: the rail, and only ever the rail.
  readonly property int inlineWidth: railWidth

  function toggle(id) { dock.open = (dock.open === id) ? "" : id; }
  function show(id)   { if (dock.open !== id) dock.open = id; }

  // ── Where the dock ACTUALLY is, in global coordinates ──────────────────────────────────────
  //
  // ⚠️ You cannot ask the dock ITEM. It is only `railWidth` wide (40px) -- the open panel hangs
  // **outside** its bounds, floating over the canvas. So `dock.mapFromGlobal(...)` against
  // `dock.width` tests the RAIL and nothing else, and anything relying on it is quietly wrong:
  //
  //   * the map's ground tap thought a click on the Details panel was a click on the map, and
  //     cleared the selection out from under the panel you were using;
  //   * `overDeleteZone` was testing the 40px rail rather than the Characters panel, so dragging
  //     somebody out to delete them only worked if you dropped them on the icon strip.
  //
  // Ask the dock. It knows where its own parts are.

  /// Is this global point inside the open PANEL? (Not the rail.)
  function panelContainsGlobal(sx, sy) {
    if (dock.open === "")
      return false;

    const p = column.mapFromGlobal(sx, sy);
    return p.x >= 0 && p.y >= 0 && p.x < column.width && p.y < column.height;
  }

  /// Is this global point anywhere on the dock -- the rail, or the open panel?
  function containsGlobal(sx, sy) {
    const r = rail.mapFromGlobal(sx, sy);
    if (r.x >= 0 && r.y >= 0 && r.x < rail.width && r.y < rail.height)
      return true;

    return dock.panelContainsGlobal(sx, sy);
  }

  implicitWidth: inlineWidth

  // ⚠️ The panel hangs OUT of the dock, over the canvas -- and the canvas is a later sibling in the
  // row, so without this it would paint straight over the panel and the panel would be invisible.
  z: 5

  // ── The panel column ────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: column

    // The DEBUG harness reads this to answer "is the panel actually on screen?" -- which is exactly
    // the question that took an hour on 2026-07-13.
    objectName: dock.isLeft ? "mapLeftPanel" : "mapRightPanel"

    width: dock.panelWidth
    height: parent.height
    z: 2
    visible: dock.open !== ""

    // Inline: beside the rail (right dock -> left of the rail; left dock -> right of it).
    // Floating: over the canvas, on the same side as the rail.
    x: {
      if (dock.isLeft)
        return dock.overlayMode ? dock.railWidth : dock.railWidth;

      return dock.overlayMode ? -dock.panelWidth : 0;
    }

    color: "#fafafa"

    // The edge that faces the map.
    Rectangle {
      x: dock.isLeft ? parent.width - 1 : 0
      width: 1
      height: parent.height
      color: "#22000000"
    }

    // Floating: a shadow, so it reads as ABOVE the map rather than beside it.
    Rectangle {
      visible: dock.overlayMode
      x: dock.isLeft ? parent.width : -10
      width: 10
      height: parent.height
      gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: dock.isLeft ? "#28000000" : "#00000000" }
        GradientStop { position: 1.0; color: dock.isLeft ? "#00000000" : "#28000000" }
      }
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      // The panel's header lives HERE, not in each panel -- so every panel is titled the same way,
      // and a panel is nothing but its content.
      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 32
        color: "#f0f0f0"

        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: 10
          anchors.rightMargin: 4
          spacing: 4

          Text {
            text: {
              for (let i = 0; i < dock.panels.length; i++)
                if (dock.panels[i].id === dock.open)
                  return dock.panels[i].title;
              return "";
            }
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
            elide: Text.ElideRight
            Layout.maximumWidth: parent.width - 70
          }

          // ── The "?" ─────────────────────────────────────────────────────────────────────────
          //
          // A panel declares `panelInfo` and gets one. It is where the PARAGRAPHS went: these panels
          // used to open with two or three of them stacked over the controls, which is a wall you
          // have to read past every time to reach what you came for (Twilight, 2026-07-13: "remove
          // all the text below Sprite Set — it's way too much to read").
          //
          // ONE per panel. Don't litter them; the mark only means something while it is rare.
          MapInfoIcon {
            Layout.alignment: Qt.AlignVCenter
            visible: body.item && body.item.panelInfo !== undefined && body.item.panelInfo !== ""
            text: (body.item && body.item.panelInfo !== undefined) ? body.item.panelInfo : ""
          }

          Item { Layout.fillWidth: true }

          // ── The panel's ONE title-bar action ─────────────────────────────────────────────────
          //
          // A panel can put a single control up here by declaring a `headerAction` Component -- the
          // Layers panel's Clear (Twilight: "Clear button on layers should be at top, actually in the
          // pull-out panel title on the right side").
          //
          // ONE. A title bar that grows a toolbar is a toolbar, and this app does not have those.
          Loader {
            Layout.alignment: Qt.AlignVCenter
            sourceComponent: (body.item && body.item.headerAction !== undefined)
                               ? body.item.headerAction : null
          }

          // Collapse. (The rail icon does the same -- two ways to the same door, because "click the
          // lit icon again" is not discoverable on its own.)
          MapRailButton {
            glyph: dock.isLeft ? "‹" : "›"
            size: 24
            tip: qsTr("Collapse the panel")
            onClicked: dock.open = ""
          }
        }
      }

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      // Only the OPEN panel is loaded. A closed panel should cost nothing -- and the Music panel in
      // particular must not be alive (and audible) when it isn't shown.
      Loader {
        id: body
        Layout.fillWidth: true
        Layout.fillHeight: true
        source: dock.sources[dock.open] !== undefined ? dock.sources[dock.open] : ""

        // A Loader's content is its own scope -- it cannot see the ids around the dock. A panel
        // that needs the canvas (the Details panel does: it edits what is SELECTED on it) declares
        // a `canvas` property and we hand it over here.
        onLoaded: {
          if (item && item.canvas !== undefined && dock.panelContext !== null)
            item.canvas = dock.panelContext;
        }
      }
    }

    // ── While it FLOATS, it must swallow its own input ────────────────────────────────────────
    //
    // Plain Rectangles and Texts don't accept events, so a click on the panel's background -- or a
    // wheel its list doesn't consume -- would fall straight through to the MAP behind it. The
    // HoverHandler matters most: hover passes through a plain Rectangle, so without it the canvas
    // would keep reporting coordinates for a cursor that is over the panel. (Same three-handler rule
    // as the Bag's View All drawer -- ui-patterns.md.)
    MouseArea {
      anchors.fill: parent
      z: -1
      enabled: dock.overlayMode
      acceptedButtons: Qt.AllButtons
    }

    WheelHandler {
      enabled: dock.overlayMode
      onWheel: (event) => { event.accepted = true; }
    }

    HoverHandler {
      enabled: dock.overlayMode
      blocking: true
    }

    // Drag the panel's map-facing edge to resize it. A splitter without a 1998 splitter handle: the
    // cursor changes, and that is the whole affordance.
    MouseArea {
      x: dock.isLeft ? parent.width - 3 : -3
      width: 7
      height: parent.height
      cursorShape: Qt.SizeHorCursor

      property int pressX: 0
      property int pressW: 0

      onPressed: (mouse) => {
        pressX = mapToItem(null, mouse.x, 0).x;
        pressW = dock.panelWidth;
      }

      onPositionChanged: (mouse) => {
        if (!pressed)
          return;

        const raw = mapToItem(null, mouse.x, 0).x - pressX;
        const dx = dock.isLeft ? raw : -raw;   // both edges grow the panel by dragging OUTWARD

        dock.panelWidth = Math.max(dock.minPanelWidth,
                                   Math.min(dock.maxPanelWidth, pressW + dx));
      }
    }
  }

  // ── The rail ────────────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: rail

    x: dock.isLeft ? 0 : dock.width - dock.railWidth
    width: dock.railWidth
    height: parent.height
    z: 3

    color: "#f2f2f2"

    Rectangle {
      x: dock.isLeft ? parent.width - 1 : 0
      width: 1
      height: parent.height
      color: "#22000000"
    }

    ColumnLayout {
      anchors.top: parent.top
      anchors.topMargin: 6
      anchors.horizontalCenter: parent.horizontalCenter
      spacing: 4

      Repeater {
        model: dock.panels

        MapRailButton {
          required property var modelData

          objectName: "dockBtn_" + modelData.id   // the DEBUG harness drives the dock through these
          glyph: modelData.glyph
          tip: modelData.tip
          active: dock.open === modelData.id

          onClicked: dock.toggle(modelData.id)
        }
      }
    }
  }
}
