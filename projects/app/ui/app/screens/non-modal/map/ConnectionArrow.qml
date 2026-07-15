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
 * A ghostly white "add a connecting route here" arrow on ONE edge of the map.
 *
 * Shown only where that edge has NO connection (an invitation, never chrome -- Twilight: *"lightweight
 * and simple… it needs to look kind of ghostly"*). Clicking it opens a small map picker; choosing a
 * neighbour calls `brg.map.addConnection(dir, ind)`, and the arrow vanishes as the connection appears.
 * That is the click-to-add half of "both add-gestures"; the drag-a-map-onto-it half rides the same
 * `addConnection` call.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: arrow

  required property var canvas
  required property int dir            // MapDBEntryConnect::ConnectDir: N 0, S 1, E 2, W 3

  readonly property var edge: { arrow.canvas.revision; return arrow.canvas.connEdgeFor(arrow.dir); }
  readonly property bool absent: arrow.edge !== null && arrow.edge.exists === false

  visible: absent && brg.mapLayers.showConnections && brg.map.valid

  readonly property real z0: arrow.canvas.zoom
  readonly property real cell: 34 * z0

  // The map rect in buffer px (bound through revision so it tracks size/map changes), and the arrow
  // parked just outside the named edge, in the border ring.
  readonly property real mx: { arrow.canvas.revision; return brg.map.mapX * z0; }
  readonly property real my: { arrow.canvas.revision; return brg.map.mapY * z0; }
  readonly property real mw: { arrow.canvas.revision; return brg.map.mapW * z0; }
  readonly property real mh: { arrow.canvas.revision; return brg.map.mapH * z0; }

  width: cell
  height: cell
  z: 6

  x: arrow.dir === 3 ? mx - cell - 6 * z0                       // West
   : arrow.dir === 2 ? mx + mw + 6 * z0                         // East
   : mx + mw / 2 - cell / 2                                     // North / South
  y: arrow.dir === 0 ? my - cell - 6 * z0                       // North
   : arrow.dir === 1 ? my + mh + 6 * z0                         // South
   : my + mh / 2 - cell / 2                                     // East / West

  Rectangle {
    id: pill
    anchors.fill: parent
    radius: 6
    color: hov.containsMouse ? "#f0ffffff" : "#66ffffff"       // ghostly by default, solid on hover
    border.width: 1
    border.color: hov.containsMouse ? "#d55e00" : "#b0ffffff"

    Text {
      anchors.centerIn: parent
      text: arrow.dir === 0 ? "▲" : arrow.dir === 1 ? "▼" : arrow.dir === 2 ? "▶" : "◀"
      font.pixelSize: Math.max(11, Math.round(15 * arrow.z0))
      color: hov.containsMouse ? "#d55e00" : "#5a5a5a"
    }

    // A small + so it reads as "add", not as a scroll button.
    Text {
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.margins: 2
      text: "+"
      font.bold: true
      font.pixelSize: Math.max(8, Math.round(10 * arrow.z0))
      color: hov.containsMouse ? "#d55e00" : "#7a7a7a"
    }
  }

  MouseArea {
    id: hov
    anchors.fill: parent
    hoverEnabled: true
    enabled: !arrow.canvas.panning && arrow.canvas.tool !== "zoom"
    cursorShape: Qt.PointingHandCursor
    onClicked: (m) => { m.accepted = true; pop.open(); }
  }

  ToolTip {
    visible: hov.containsMouse && !pop.opened
    text: qsTr("Add a connecting route on the %1 edge")
          .arg(arrow.edge ? arrow.edge.dirName : "")
    delay: 400
  }

  // ── The picker ───────────────────────────────────────────────────────────────────────────
  Popup {
    id: pop
    y: arrow.height + 4
    width: 300
    padding: 10
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    // While it is open the ground must not take the dismiss-press (the popup-leak bug the canvas
    // already guards). We register with the canvas the same way the docks do.
    onOpenedChanged: {
      arrow.canvas.popupsOpen += (pop.opened ? 1 : -1);
      if (arrow.canvas.popupsOpen < 0) arrow.canvas.popupsOpen = 0;
    }

    background: Rectangle {
      color: "#ffffff"; radius: 6
      border.width: 1; border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 8

      Text {
        text: qsTr("Connect the %1 edge to…").arg(arrow.edge ? arrow.edge.dirName : "")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      ComboBox {
        id: combo
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.connectionMapList(arrow.dir)
        textRole: "name"
        valueRole: "value"
        currentIndex: -1
        displayText: currentIndex < 0 ? qsTr("Pick a neighbouring map…") : currentText

        onActivated: {
          if (currentValue === undefined || currentValue < 0)
            return;
          if (brg.map.addConnection(arrow.dir, currentValue)) {
            if (!brg.mapLayers.showConnections)
              brg.mapLayers.setKeyVisible("connections", true);
            arrow.canvas.selectedConnection = arrow.dir;
            arrow.canvas.status = qsTr("Connected the %1 edge to %2.")
                                    .arg(arrow.edge ? arrow.edge.dirName : "").arg(currentText);
          }
          pop.close();
        }

        // Grouped: Default first, then the maps that fit this edge, then the rest. Each row shows the
        // map's SIZE (grey, right), and the default wears a ★.
        delegate: ItemDelegate {
          required property var modelData
          required property int index
          width: combo.width
          height: (modelData.group !== "" ? 20 : 0) + 26
          highlighted: combo.highlightedIndex === index

          contentItem: ColumnLayout {
            spacing: 0
            Text {
              visible: modelData.group !== ""
              Layout.fillWidth: true
              text: modelData.group
              font.pixelSize: 10; font.bold: true
              color: brg.settings.textColorMid
            }
            RowLayout {
              Layout.fillWidth: true
              spacing: 6
              Text {
                visible: modelData.isDefault === true
                text: "★"
                font.pixelSize: 11
                color: "#e69f00"
              }
              Text {
                Layout.fillWidth: true
                text: modelData.name
                font.pixelSize: 12
                font.bold: modelData.isDefault === true
                color: brg.settings.textColorDark
                elide: Text.ElideRight
              }
              Text {
                text: modelData.size
                font.pixelSize: 10; font.family: "monospace"
                color: brg.settings.textColorMid
              }
            }
          }
        }
      }

      Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("It starts corner-aligned; drag the strip along the edge to slide it, or edit the "
                   + "details to set the exact offset.")
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }
  }
}
