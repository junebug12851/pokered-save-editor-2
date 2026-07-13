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

    // ── Contrast ─────────────────────────────────────────────────────────────────────────────
    ContrastPicker { id: contrastPicker }

    Rectangle {
      visible: brg.mapClock.animates
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
    }

    // ── The animation: ONE button that is either ▶ or ⏸ ──────────────────────────────────────
    //
    // The console rewrites the water tile every 20 frames (21 with flowers) -- about three times a
    // second -- so the map MOVES. This is the switch for it, and it is a thing you DO, which is why
    // it is up here and the frame counter is down in the status bar with the other facts.
    MapRailButton {
      objectName: "animToggle"
      visible: brg.mapClock.animates
      size: 26
      glyph: brg.mapClock.playing ? "⏸" : "▶"
      tip: brg.mapClock.playing
           ? qsTr("Pause the animation")
           : qsTr("Animate the map — the water and the flowers, at the console's own pace")
      onClicked: brg.mapClock.playing = !brg.mapClock.playing
    }

    Item { Layout.fillWidth: true }
  }
}
