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
  MapStatusBar.qml -- where the cursor is, what is under it, and how far in we are zoomed.

  Aseprite's status bar, in this app's clothes. It replaces the old footer, which was a colour
  legend (three swatches naming three rectangles) plus the zoom buttons. The legend is gone because
  the LAYERS PANEL is the legend now -- every layer row carries the exact ink the renderer uses, so
  it cannot drift out of sync with what is drawn. A legend that lives next to the thing it explains
  beats a legend in a footer.

  What replaces it is the thing a map editor actually needs on every mouse-move: the coordinates,
  in BOTH systems the game uses (the map's own, and the buffer's -- the ring included), the block id
  under the cursor, and what that tile DOES in words. Everything comes from brg.map.describeAt();
  QML computes none of it.
*/
import QtQuick
import QtQuick.Layouts

Rectangle {
  id: bar

  /// What the canvas last reported under the cursor (brg.map.describeAt()), or null when the
  /// cursor is off the map.
  property var at: null

  /// The canvas, for the zoom readout + buttons.
  property var canvas: null

  implicitHeight: 26
  color: "#f7f7f7"

  Rectangle {
    anchors.top: parent.top
    width: parent.width
    height: 1
    color: brg.settings.dividerColor
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 8
    spacing: 14

    // Where the cursor is. Two coordinate systems, because the game uses both and they differ by
    // the 3-block ring -- and a save editor that shows you only one of them is hiding the other.
    Text {
      text: {
        if (!bar.at || !bar.at.valid)
          return qsTr("—");

        if (bar.at.border)
          return qsTr("border ring · block %1, %2").arg(bar.at.blockX).arg(bar.at.blockY);

        return qsTr("tile %1, %2   ·   block %3, %4")
                 .arg(bar.at.mapTileX).arg(bar.at.mapTileY)
                 .arg(bar.at.mapBlockX).arg(bar.at.mapBlockY);
      }
      font.pixelSize: 11
      font.family: "monospace"   // coordinates that don't jitter as the digits change
      color: brg.settings.textColorDark
    }

    Rectangle {
      visible: bar.at && bar.at.valid
      implicitWidth: 1
      implicitHeight: 12
      color: brg.settings.dividerColor
    }

    // What is under it, in words. The whole reason the meaning layer exists: a wall and a floor are
    // just two pictures until something says which is which.
    Text {
      visible: bar.at && bar.at.valid
      text: {
        if (!bar.at || !bar.at.valid)
          return "";

        const b = "$" + Number(bar.at.block).toString(16).toUpperCase().padStart(2, "0");
        return bar.at.label !== ""
             ? qsTr("block %1 · %2").arg(b).arg(bar.at.label)
             : qsTr("block %1").arg(b);
      }
      font.pixelSize: 11
      color: brg.settings.textColorMid
      elide: Text.ElideRight
      Layout.fillWidth: true
      Layout.maximumWidth: implicitWidth
    }

    Item { Layout.fillWidth: true }

    // ── The transport ────────────────────────────────────────────────────────────────────────
    //
    // The map MOVES: the console rewrites the water tile every 20 frames (21 with flowers), which
    // is about three times a second. That is not decoration -- it is what a Game Boy puts on the
    // screen, and a still map is a map telling you something false.
    //
    // ⏸ exists partly because the screenshots and the visual-regression tests need a frozen frame 0
    // anyway (MapClock refuses to run headless), so exposing it costs one button. The step button
    // is for walking the water round its 8-step cycle by hand.
    //
    // A map with nothing to animate says so by being absent: no transport, no lie.
    RowLayout {
      spacing: 2
      visible: brg.mapClock.animates

      MapRailButton {
        size: 22
        glyph: brg.mapClock.playing ? "⏸" : "▶"
        tip: brg.mapClock.playing
             ? qsTr("Pause the animation")
             : qsTr("Animate the map — the water and the flowers, at the console's own pace")
        onClicked: brg.mapClock.playing = !brg.mapClock.playing
      }

      MapRailButton {
        size: 22
        glyph: "⏭"
        tip: qsTr("One animation step (there are 8 in a full cycle)")
        onClicked: brg.mapClock.step()
      }

      // The cadence, stated: 20 frames for water, 21 when the flowers move too. It is the sort of
      // number that makes someone trust what they are looking at.
      Text {
        text: qsTr("%1f · %2/8").arg(brg.mapClock.cadence).arg(brg.mapClock.frame % 8)
        font.pixelSize: 10
        font.family: "monospace"
        color: brg.settings.textColorMid
        Layout.leftMargin: 2
      }
    }

    Rectangle {
      visible: brg.mapClock.animates
      implicitWidth: 1
      implicitHeight: 12
      color: brg.settings.dividerColor
      Layout.leftMargin: 6
      Layout.rightMargin: 2
    }

    Text {
      text: bar.canvas ? qsTr("%1×").arg(bar.canvas.zoom) : ""
      font.pixelSize: 11
      color: brg.settings.textColorMid
      Layout.minimumWidth: 24
      horizontalAlignment: Text.AlignRight
    }

    MapRailButton {
      size: 22
      glyph: "−"
      tip: qsTr("Zoom out")
      enabledBtn: bar.canvas && bar.canvas.zoom > bar.canvas.minZoom
      onClicked: bar.canvas.userZoom = bar.canvas.zoom - 1
    }

    MapRailButton {
      size: 22
      glyph: "+"
      tip: qsTr("Zoom in")
      enabledBtn: bar.canvas && bar.canvas.zoom < bar.canvas.maxZoom
      onClicked: bar.canvas.userZoom = bar.canvas.zoom + 1
    }

    MapRailButton {
      size: 22
      glyph: "⤢"
      tip: qsTr("Fit the whole map in the window")
      enabledBtn: bar.canvas && bar.canvas.userZoom > 0
      onClicked: bar.canvas.userZoom = 0
    }
  }
}
