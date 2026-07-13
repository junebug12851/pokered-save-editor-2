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
    * THE MAP NEVER DISAPPEARS. Too narrow to seat a panel beside the map? The panel FLOATS over the
      canvas edge instead of squeezing it -- with a shadow, and swallowing its own clicks, wheel and
      hover so nothing falls through to the map behind.
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

  /// How wide the panel column is when open (drag its inner edge; remembered for the session).
  property int panelWidth: 170
  readonly property int railWidth: 40
  readonly property int minPanelWidth: 140
  readonly property int maxPanelWidth: 320

  /// How much room the canvas can spare. Below the panel's minimum it floats instead of pushing.
  property int roomForPanel: 0
  readonly property bool overlayMode: dock.open !== "" && roomForPanel < dock.minPanelWidth

  /// What the dock actually takes out of the row (just the rail, when the panel floats).
  readonly property int inlineWidth: railWidth + ((open !== "" && !overlayMode) ? panelWidth : 0)

  function toggle(id) { dock.open = (dock.open === id) ? "" : id; }
  function show(id)   { if (dock.open !== id) dock.open = id; }

  implicitWidth: inlineWidth

  // ── The panel column ────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: column

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
            Layout.fillWidth: true
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
