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
 * ZOOM -- and it lives in exactly ONE place (Twilight, 2026-07-13: *"I just don't want multiple
 * places where zoom is."*).
 *
 * A little **▾** on the zoom tool. Click it and a panel slides down with:
 *
 *   * a **slider** — drag it left and right; the map follows continuously (the pixel-art shader
 *     means a fractional zoom is a real one now, so there is nothing to snap to);
 *   * **Go to…** — the whole map, the Game Boy's camera, the player, and then the *randoms*: a
 *     random person, object, door, warp, block, or connecting route. They are how you go and LOOK
 *     at a map you have never seen, instead of hunting across 78 blocks of Route 17 for something
 *     worth finding.
 *
 * An entry this map has none of is **greyed out and says so** — never a live button that silently
 * does nothing.
 *
 * The on-canvas zoom buttons are GONE. There is one zoom, and it is here.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: zoomMenu
  objectName: "zoomMenu"   // the DEBUG harness opens it through this

  /// The canvas we drive.
  ///
  /// ⚠️ Not `required`, and every read of it below is NULL-GUARDED. This menu now lives in the left
  /// rail, which is built inside the dock *before* the MapCanvas exists — so `canvas` is briefly null
  /// at construction, and an unguarded binding (`Math.log(canvas.minZoom)`) throws during load. It
  /// arrives a moment later via the dock's `panelContext`. `tst_qml_screens` is the backstop.
  property var canvas: null

  property bool openState: false

  implicitWidth: chevron.implicitWidth
  implicitHeight: 26

  readonly property var destinations: [
    { kind: "map",        label: qsTr("The whole map"),        glyph: "▣" },
    { kind: "camera",     label: qsTr("The Game Boy's screen"), glyph: "▢" },
    { kind: "player",     label: qsTr("The player"),           glyph: "☺" },
    { kind: "sprite",     label: qsTr("A random person"),      glyph: "☻" },
    { kind: "object",     label: qsTr("A random object"),      glyph: "◉" },
    { kind: "door",       label: qsTr("A random door"),        glyph: "⌂" },
    { kind: "warp",       label: qsTr("A random warp"),        glyph: "⇄" },
    { kind: "xy",         label: qsTr("A random block"),       glyph: "⊞" },
    { kind: "connection", label: qsTr("A connecting route"),   glyph: "⇥" }
  ]

  property var available: ({})

  function refresh() {
    zoomMenu.available = brg.map.zoomTargetsAvailable();
  }

  Connections {
    target: brg.map
    function onChanged() { zoomMenu.refresh(); }
  }

  Component.onCompleted: zoomMenu.refresh()

  // The ▾. Small, quiet, and right up against the zoom tool so it reads as belonging to it.
  Rectangle {
    id: chevron
    objectName: "zoomMenuChevron"

    implicitWidth: 14
    height: 26
    anchors.verticalCenter: parent.verticalCenter

    radius: 4
    color: zoomMenu.openState ? Qt.rgba(0, 0, 0, 0.10)
         : chevHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
         : "transparent"

    Label {
      anchors.centerIn: parent
      text: "▾"
      font.pixelSize: 9
      opacity: 0.75
    }

    HoverHandler { id: chevHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: zoomMenu.openState = !zoomMenu.openState }

    ToolTip.visible: brg.settings.infoBtnPressed && (chevHover.hovered && !zoomMenu.openState)
    ToolTip.delay: 500
    ToolTip.text: qsTr("Zoom — the slider, and somewhere to go")
  }

  // ── The panel ─────────────────────────────────────────────────────────────────────────────
  Popup {
    id: panel

    visible: zoomMenu.openState
    onClosed: zoomMenu.openState = false

    x: -8
    y: zoomMenu.height + 4

    // Tight on purpose. Nine destinations plus a slider has to fit inside the app's default 480px
    // window without running off the bottom of it -- a menu you have to resize the window to read
    // is not a menu.
    width: 222
    padding: 8

    background: Rectangle {
      color: "#ffffff"
      radius: 8
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      width: parent.width
      spacing: 5

      // ── The slider ───────────────────────────────────────────────────────────────────────
      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
          text: qsTr("Zoom")
          font.pixelSize: 11
          font.bold: true
          opacity: 0.7
          Layout.fillWidth: true
        }

        Label {
          // Two decimals would be noise; the zoom is a feel, not a measurement. But a whole
          // multiple is worth KNOWING about, because that is where the art is pixel-exact.
          readonly property real zoomVal: zoomMenu.canvas ? zoomMenu.canvas.zoom : 1
          text: Math.abs(zoomVal - Math.round(zoomVal)) < 0.02
                  ? qsTr("%1× (exact)").arg(Math.round(zoomVal))
                  : (zoomVal.toFixed(1) + "×")
          font.pixelSize: 11
          opacity: 0.6
        }
      }

      Slider {
        id: zoomSlider
        objectName: "zoomSlider"

        Layout.fillWidth: true
        Layout.preferredHeight: 24

        // LOG scale. On a linear one half the travel is spent between 6× and 12×, which nobody
        // uses, and the range people actually live in (0.5×–3×) is crammed into a fingernail. On a
        // log scale every equal nudge is an equal *ratio*, which is what zoom actually feels like.
        from: zoomMenu.canvas ? Math.log(zoomMenu.canvas.minZoom) : 0
        to: zoomMenu.canvas ? Math.log(zoomMenu.canvas.maxZoom) : 1
        value: zoomMenu.canvas ? Math.log(zoomMenu.canvas.zoom) : 0

        onMoved: if (zoomMenu.canvas) zoomMenu.canvas.zoomToCentre(Math.exp(value))
      }

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      Label {
        text: qsTr("Go to…")
        font.pixelSize: 11
        font.bold: true
        opacity: 0.7
        Layout.fillWidth: true
      }

      // ── Somewhere to go ──────────────────────────────────────────────────────────────────
      Repeater {
        model: zoomMenu.destinations

        delegate: Rectangle {
          id: dest
          required property var modelData

          readonly property bool here: zoomMenu.available[dest.modelData.kind] !== false

          Layout.fillWidth: true
          implicitHeight: 23
          radius: 5

          color: !dest.here ? "transparent"
               : destHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
               : "transparent"

          opacity: dest.here ? 1.0 : 0.38

          RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            spacing: 7

            Label {
              text: dest.modelData.glyph
              font.pixelSize: 12
              opacity: 0.7
              Layout.preferredWidth: 14
            }

            Label {
              text: dest.modelData.label
              font.pixelSize: 11
              Layout.fillWidth: true
              elide: Text.ElideRight
            }

            // An entry this map has none of SAYS so. A live-looking button that does nothing when
            // you click it is worse than one that admits it is empty.
            Label {
              visible: !dest.here
              text: qsTr("none here")
              font.pixelSize: 9
              opacity: 0.8
            }
          }

          HoverHandler {
            id: destHover
            enabled: dest.here
            cursorShape: Qt.PointingHandCursor
          }

          TapHandler {
            enabled: dest.here
            onTapped: {
              if (zoomMenu.canvas)
                zoomMenu.canvas.goTo(dest.modelData.kind);
              zoomMenu.openState = false;
            }
          }
        }
      }
    }
  }
}
