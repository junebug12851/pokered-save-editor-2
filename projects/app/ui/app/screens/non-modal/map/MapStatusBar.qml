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
  MapStatusBar.qml -- the DETAILS. Where the cursor is, what is under it, how big the map is, what
  the animation is doing, how far in you are zoomed.

  Project leadership, 2026-07-13: "details like the map size and many other details need to be in a status bar
  below, not in the toolbar at top". Right -- the toolbar is for things you DO; the status bar is for
  things that are TRUE. So the size chip and the unfinished-copy note came down here, and the
  "Click a block to inspect it" bar above the map went away entirely.

  It is also what replaced the old footer legend, and the old click-a-block-opens-a-panel behaviour:
  the block under your cursor is named here, with what its tiles DO, without a wall of information
  arriving uninvited on the side of the screen. Everything comes from one C++ call
  (`brg.map.describeAt`) on each mouse-move, and it renders nothing.
*/
import QtQuick
import QtQuick.Layouts

Rectangle {
  id: bar

  /// What the canvas last reported under the cursor (brg.map.describeAt()), or null when it is off
  /// the map.
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

  component Sep: Rectangle {
    implicitWidth: 1
    implicitHeight: 12
    color: brg.settings.dividerColor
    Layout.alignment: Qt.AlignVCenter
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 6
    spacing: 10

    // Where the cursor is. BOTH of the game's coordinate systems, because it uses both and they
    // differ by the 3-block ring -- and an editor that shows you only one of them is hiding the other.
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

    Sep { visible: bar.at && bar.at.valid }

    // What is under it, in words -- the whole reason the meaning layer exists: a wall and a floor
    // are just two pictures until something says which is which.
    Text {
      visible: bar.at && bar.at.valid
      Layout.maximumWidth: 300
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
    }

    Item { Layout.fillWidth: true }

    // ── The facts about the map itself ───────────────────────────────────────────────────────
    Text {
      text: qsTr("%1 × %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh)
      font.pixelSize: 11
      color: brg.settings.textColorMid
    }

    Sep {}

    Text {
      text: qsTr("map %1").arg(brg.map.mapInd)
      font.pixelSize: 11
      font.family: "monospace"
      color: brg.settings.textColorMid
    }

    // A glitch / half-baked id draws ANOTHER map's data -- which is exactly what a Game Boy does with
    // it. Said plainly, in the place facts live, and not in red: it is information, not an error.
    Text {
      visible: brg.map.isCopy
      text: qsTr("· unfinished copy of %1").arg(brg.map.copyOfName)
      font.pixelSize: 11
      color: brg.settings.textColorMid
      font.italic: true
      elide: Text.ElideRight
      Layout.maximumWidth: 220
    }

    Sep { visible: brg.mapClock.animates }

    // The animation, as a fact. (The ▶/⏸ itself is one button in the toolbar -- it is a thing you
    // DO.) The cadence is worth showing: 20 frames a step for water, 21 when the flowers move too.
    Text {
      visible: brg.mapClock.animates
      text: qsTr("anim %1f · %2/8").arg(brg.mapClock.cadence).arg(brg.mapClock.frame % 8)
      font.pixelSize: 11
      font.family: "monospace"
      color: brg.settings.textColorMid
    }

    Sep {}

    // ⚠️ The zoom READOUT, and nothing else. The −/+/fit buttons that used to live here are GONE:
    // zoom now has exactly ONE home, the ▾ on the toolbar's zoom tool (project leadership, 2026-07-13: "I just
    // don't want multiple places where zoom is").
    //
    // A number is a FACT, and facts are what this bar is for. A button is a THING YOU DO, and those
    // live up top.
    Text {
      text: !bar.canvas ? ""
          : Math.abs(bar.canvas.zoom - Math.round(bar.canvas.zoom)) < 0.02
              ? qsTr("%1×").arg(Math.round(bar.canvas.zoom))
              : (bar.canvas.zoom.toFixed(2) + "×")
      font.pixelSize: 11
      font.family: "monospace"
      color: brg.settings.textColorMid
      Layout.minimumWidth: 38
      horizontalAlignment: Text.AlignRight
    }
  }
}
