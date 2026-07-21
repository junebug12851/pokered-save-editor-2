/*
  * Copyright 2026 Fairy Fox
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
  MapSimButton.qml -- one control in the top bar's SIMULATION group: a play/pause SPLIT BUTTON.

  Project leadership, 2026-07-14: *"have the dropdown arrow INSIDE the button, like a normal dropdown tool
  button"* (the ▾ used to be a separate square to the right, which sprawled the group out).

  So it is one rounded frame with two zones:

      ┌──────────────┬───┐
      │  ▶/⏸  symbol  │ ⌄ │      left zone plays/pauses · right zone drops the menu
      └──────────────┴───┘

  A button with no menu (Walk) is just the left zone -- no divider, no ⌄. Running = filled orange.

  The button does not own the popup -- the caller does, and positions it under the ⌄.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: sim

  /// The subject icon, as a glyph (♪, ✿). Leave empty to use @ref iconSource.
  property string glyph: ""

  /// The subject icon, as an SVG (walk footprints) -- the DARK one, for the button at rest.
  ///
  /// ⚠️ Two pre-coloured SVGs swapped by state, NOT a shader recolour: `MultiEffect`/any GPU recolour
  /// draws NOTHING on Qt's software backend, which the screenshooter / screenshot review / CI all use,
  /// so the icon comes up blank there. Plain `Image` renders everywhere. (ui-patterns.md.)
  property string iconSource: ""

  /// The WHITE SVG, for the playing (orange) state. Falls back to @ref iconSource if empty.
  property string iconSourcePlaying: ""

  property bool playing: false
  property bool playEnabled: true
  property string playTip: ""

  property bool hasMenu: false
  property bool menuOpen: false
  property string menuTip: ""

  signal toggled()
  signal menuToggled()

  implicitWidth: frame.implicitWidth
  implicitHeight: 26

  Rectangle {
    id: frame
    anchors.fill: parent
    radius: 6

    implicitWidth: contentRow.implicitWidth

    // The whole frame carries the play state -- filled orange running, outlined white at rest, both
    // spelled out rather than inherited from the palette (which resolves its text LIGHT and bit us).
    color: sim.playing ? "#d55e00" : "#ffffff"
    border.width: 1
    border.color: sim.playing ? "#d55e00" : brg.settings.dividerColor
    opacity: sim.playEnabled ? 1.0 : 0.4

    Behavior on color { ColorAnimation { duration: 90 } }

    readonly property color ink: sim.playing ? "#ffffff" : brg.settings.textColorDark

    Row {
      id: contentRow
      anchors.fill: parent

      // ── The play / pause zone: ▶/⏸ + the subject icon ─────────────────────────────────────
      Item {
        id: playZone
        width: playRow.implicitWidth + 14
        height: parent.height

        // Hover wash, inset so it stays within the rounded frame.
        Rectangle {
          anchors.fill: parent
          anchors.margins: 2
          radius: 4
          visible: sim.playEnabled && playHover.hovered && !sim.playing
          color: "#00000014"
        }

        Row {
          id: playRow
          anchors.centerIn: parent
          spacing: 3

          Text {
            anchors.verticalCenter: parent.verticalCenter
            text: sim.playing ? "⏸" : "▶"
            font.pixelSize: 10
            color: frame.ink
          }

          Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: sim.glyph !== ""
            text: sim.glyph
            font.pixelSize: 13
            color: frame.ink
          }

          Image {
            anchors.verticalCenter: parent.verticalCenter
            visible: sim.iconSource !== ""
            width: 15
            height: 15
            source: (sim.playing && sim.iconSourcePlaying !== "") ? sim.iconSourcePlaying
                                                                  : sim.iconSource
            sourceSize: Qt.size(30, 30)
            fillMode: Image.PreserveAspectFit
            smooth: true
          }
        }

        HoverHandler { id: playHover; enabled: sim.playEnabled; cursorShape: Qt.PointingHandCursor }
        TapHandler { enabled: sim.playEnabled; onTapped: sim.toggled() }

        MapToolTip { shown: playHover.hovered && sim.playTip !== ""; text: sim.playTip }
      }

      // ── The divider + ▾, INSIDE the frame (only when there's a menu) ───────────────────────
      Rectangle {
        visible: sim.hasMenu
        width: 1
        height: parent.height
        color: sim.playing ? Qt.rgba(1, 1, 1, 0.35) : brg.settings.dividerColor
      }

      Item {
        id: menuZone
        visible: sim.hasMenu
        width: 18
        height: parent.height

        Rectangle {
          anchors.fill: parent
          anchors.margins: 2
          radius: 4
          visible: (menuHover.hovered || sim.menuOpen) && !sim.playing
          color: "#00000014"
        }

        Text {
          anchors.centerIn: parent
          text: "⌄"
          font.pixelSize: 10
          color: frame.ink
          opacity: 0.8
        }

        HoverHandler { id: menuHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: sim.menuToggled() }

        MapToolTip { shown: menuHover.hovered && sim.menuTip !== "" && !sim.menuOpen; text: sim.menuTip }
      }
    }
  }
}
