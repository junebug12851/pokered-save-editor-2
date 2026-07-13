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
  MapIdentityBar.qml -- the top bar: THE TOOLS, then WHAT IS LOADED, then THE PALETTE.

  Twilight's call (2026-07-13): the tools moved up here from a left-hand rail, and the whole left
  edge went back to the map. What used to be three read-only chips (map name, tileset, size) is now
  ONE control you can actually drive:

    [ ↖ ✥ ⌕ ] │ [ Pallet Town · Overworld ⌄ ] [ 100% ⌄ ] [ 10x9 ] [ ⚠ unfinished copy ]
      tools        the map picker              the palette   size    what's wrong, if anything

  * The MAP PICKER (MapPicker.qml) drops the three things the save keeps separately -- the map id,
    the tileset the graphics come from (with Indoor/Cave/Outdoor, which is not a place but which
    tiles MOVE), and the blockset the map is BUILT from. Two pointers, two controls, because the
    save has two pointers and a console obeys both.
  * The PALETTE (ContrastPicker.qml) reads as a percentage and drops a segmented slider. The six
    glitch values are behind a switch -- shown as their own, differently coloured segments, because
    they are not levels; they are a read that lands between two.

  Nothing here is a menu bar and nothing here is a toolbar with separators. It is chips.
*/
import QtQuick
import QtQuick.Layouts

Rectangle {
  id: bar

  /// The active tool. The rail moved into this bar, so the bar owns it now.
  property string tool: "select"

  /// The two drop-downs, drivable by name for the DEBUG harness / the screenshot review.
  property alias mapPickerOpen: mapPicker.openState
  property alias contrastPickerOpen: contrastPicker.openState
  property alias contrastShowGlitch: contrastPicker.showGlitch

  readonly property var tools: [
    { id: "select", glyph: "↖", key: "V", tip: qsTr("Select & Move — click to select, drag to move") },
    { id: "pan",    glyph: "✥", key: "H", tip: qsTr("Pan — drag the map (or hold Space with any tool)") },
    { id: "zoom",   glyph: "⌕", key: "Z", tip: qsTr("Zoom — click to zoom in, Alt-click to zoom out") }
  ]

  implicitHeight: 36
  color: "#f7f7f7"

  Rectangle {
    anchors.bottom: parent.bottom
    width: parent.width
    height: 1
    color: brg.settings.dividerColor
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 8
    anchors.rightMargin: 10
    spacing: 8

    // ── The tools ─────────────────────────────────────────────────────────────────────────────
    RowLayout {
      spacing: 2

      Repeater {
        model: bar.tools

        MapRailButton {
          required property var modelData

          objectName: "toolBtn_" + modelData.id   // the DEBUG harness picks tools through these
          size: 26
          glyph: modelData.glyph
          tip: modelData.tip
          shortcut: modelData.key
          active: bar.tool === modelData.id

          onClicked: bar.tool = modelData.id
        }
      }
    }

    Rectangle {
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
      Layout.rightMargin: 2
    }

    // ── What is loaded, and what it is drawn out of ───────────────────────────────────────────
    MapPicker { id: mapPicker }

    // ── The palette ──────────────────────────────────────────────────────────────────────────
    ContrastPicker { id: contrastPicker }

    // A plain fact, not a control: the map's size. (Changing it lives in the map picker's "Fix",
    // and in the Blocks panel -- it is a property of the map, not of this bar.)
    Rectangle {
      implicitWidth: sizeText.implicitWidth + 14
      implicitHeight: 20
      radius: 10
      color: "transparent"
      border.width: 1
      border.color: brg.settings.dividerColor

      Text {
        id: sizeText
        anchors.centerIn: parent
        text: qsTr("%1 × %2").arg(brg.map.blocksWide).arg(brg.map.blocksHigh)
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }

    // A glitch / half-baked map id draws ANOTHER map's data -- which is exactly what a Game Boy does
    // with it. Say so, rather than let it pass as a map of its own.
    Rectangle {
      visible: brg.map.isCopy
      implicitWidth: warnText.implicitWidth + 16
      implicitHeight: 20
      radius: 10
      color: Qt.rgba(brg.settings.errorColor.r, brg.settings.errorColor.g,
                     brg.settings.errorColor.b, 0.10)
      border.width: 1
      border.color: brg.settings.errorColor

      Text {
        id: warnText
        anchors.centerIn: parent
        text: qsTr("⚠ unfinished copy of %1").arg(brg.map.copyOfName)
        font.pixelSize: 11
        color: brg.settings.errorColor
      }
    }

    Item { Layout.fillWidth: true }
  }
}
