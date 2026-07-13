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
  MapDock.qml -- the right-hand dock: an icon RAIL, and at most ONE open panel.

  This replaces the old three-toggles-three-columns arrangement, which stacked panels beside each
  other and, when they no longer fit, silently EVICTED the longest-open one. That was a workaround
  for a layout that was never designed to hold this much. (notes/plans/map-screen.md)

  The rules, and each one is a rule Twilight asked for:

    * PANELS DO NOT STACK. One panel is open at a time. Opening another closes the first --
      predictably, visibly, always in the same place. Nothing is ever evicted behind your back.
    * IT COLLAPSES TO NOTHING. Click the lit icon and the panel folds away, leaving the 44px rail.
      The map gets the width back.
    * THE MAP NEVER DISAPPEARS. If the window is too narrow to seat a panel beside the map, the
      panel becomes an OVERLAY (a scrim + a floating column over the canvas edge) instead of
      squeezing the map to nothing. The map keeps its floor at every window size.

  The icon rail is the legend: what's open is lit, and everything you could open is right there.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: dock

  /// The panels this dock offers, in rail order. `id` is the key; `glyph` is the rail icon.
  /// (Glyphs, not SVGs: these are one-character marks in the app's own flat language, and a tray
  /// of tiny system icons is exactly the old-fashioned chrome this screen is getting rid of.)
  //
  // The Layers (phase 2) and Inspector (phase 5) panels join this list in their own phases. A rail
  // icon that opens an empty panel is exactly the "rough it in and clean it up later" this project
  // does not do -- an icon appears here the day its panel is finished.
  readonly property var panels: [
    { id: "blocks",    glyph: "▣", title: qsTr("Blocks"),     tip: qsTr("Blocks — click the map to inspect one") },
    { id: "tileset",   glyph: "▦", title: qsTr("Tileset"),    tip: qsTr("Tileset — what the tiles are and what they do") },
    { id: "music",     glyph: "♪", title: qsTr("Music"),      tip: qsTr("Music — the map's track, and play it") }
  ]

  /// The open panel's id, or "" for none. ONE at a time, by design.
  property string open: ""

  /// How wide the panel column is when open (drag the edge to change it; remembered per session).
  property int panelWidth: 300
  readonly property int railWidth: 44
  readonly property int minPanelWidth: 240
  readonly property int maxPanelWidth: 420

  /// How much room the canvas can spare. Below this the panel floats OVER the canvas instead of
  /// pushing it -- the map is the screen, and it does not get squeezed to nothing.
  property int roomForPanel: 0
  readonly property bool overlayMode: dock.open !== "" && roomForPanel < dock.minPanelWidth

  /// What the dock actually takes out of the row (0 when it floats).
  readonly property int inlineWidth: railWidth + ((open !== "" && !overlayMode) ? panelWidth : 0)

  /// Which tile of the selected block the open panel is hovering, mirrored onto the map -- so the
  /// panel and the map are one thing rather than two. -1 when the panel has nothing to say.
  readonly property int hoveredTile: (body.item && body.item.hoveredTile !== undefined)
                                     ? body.item.hoveredTile : -1

  signal opened(string id)

  function toggle(id) {
    dock.open = (dock.open === id) ? "" : id;
    if (dock.open !== "")
      dock.opened(dock.open);
  }

  /// Open a panel that may already be open (a click on the map opening the Blocks panel, say).
  /// `toggle` would CLOSE it in that case, which is not what the caller means.
  function show(id) {
    if (dock.open !== id)
      toggle(id);
  }

  implicitWidth: inlineWidth

  // ── The panel column ────────────────────────────────────────────────────────────────────────
  //
  // Inline it sits beside the rail; in overlay mode it floats to the LEFT of the rail, over the
  // canvas, with a scrim behind it. Same item, two homes -- so there is one panel, not two
  // implementations of one panel that can drift apart.
  Rectangle {
    id: column

    width: dock.panelWidth
    height: parent.height

    x: dock.overlayMode ? -dock.panelWidth : 0
    z: 2
    visible: dock.open !== ""

    color: "#fafafa"

    // Its own left edge, and a shadow when it is floating (so it reads as ABOVE the map).
    Rectangle {
      anchors.left: parent.left
      width: 1
      height: parent.height
      color: "#22000000"
    }

    Rectangle {
      visible: dock.overlayMode
      anchors.right: parent.left
      width: 10
      height: parent.height
      gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#00000000" }
        GradientStop { position: 1.0; color: "#28000000" }
      }
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      // The panel's header lives HERE, not in each panel -- so every panel is titled the same way,
      // and a panel is just its content.
      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 34
        color: "#f0f0f0"

        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: 12
          anchors.rightMargin: 6
          spacing: 6

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
          }

          Item { Layout.fillWidth: true }

          // Collapse. (The rail icon does the same thing -- two ways to the same door, because
          // "click the lit icon again" is not discoverable on its own.)
          MapRailButton {
            glyph: "›"
            size: 26
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

      // The panels themselves. Only the open one is loaded -- a closed panel should cost nothing,
      // and the Music panel in particular must not be alive (and audible) when it isn't shown.
      Loader {
        id: body
        Layout.fillWidth: true
        Layout.fillHeight: true

        source: {
          switch (dock.open) {
          case "blocks":    return "BlockPanel.qml";
          case "tileset":   return "TilesetPanel.qml";
          case "music":     return "MusicPanel.qml";
          }
          return "";
        }
      }
    }

    // ── While it FLOATS, it must swallow its own input ────────────────────────────────────────
    //
    // Plain Rectangles and Texts don't accept events, so a click on the panel's background -- or a
    // wheel its list doesn't consume -- would fall straight through to the MAP behind it and pan or
    // zoom it. The HoverHandler is the one that matters most: hover passes through a plain
    // Rectangle, so without it the canvas would keep reporting coordinates for a cursor that is
    // over the panel. (Same three-handler pattern as the Bag's View All drawer -- ui-patterns.md.)
    //
    // The lowest child, so the panel's own content still gets first crack at everything.
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

    // Drag the panel's left edge to resize it. A splitter, without a 1998 splitter handle: the
    // cursor changes, and that is the whole affordance.
    MouseArea {
      anchors.left: parent.left
      anchors.leftMargin: -3
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

        const dx = pressX - mapToItem(null, mouse.x, 0).x;
        dock.panelWidth = Math.max(dock.minPanelWidth,
                                   Math.min(dock.maxPanelWidth, pressW + dx));
      }
    }
  }

  // ── The rail ────────────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: rail

    anchors.right: parent.right
    width: dock.railWidth
    height: parent.height
    z: 3

    color: "#f2f2f2"

    Rectangle {
      anchors.left: parent.left
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
