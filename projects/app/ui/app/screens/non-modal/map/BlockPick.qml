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
  BlockPick.qml -- pick a BLOCK by looking at it.

  TilePick's sibling, one layer up. A block is 4x4 tiles (32x32 px) and it is what a map is
  actually MADE of -- so the map's out-of-bounds block (`wMapBackgroundTile`, the thing that
  fills the 3-block border ring) is a block, not a number, and you pick it by seeing it.

  The one that matters: change this and the ring around the whole map changes with it, live.
  You are not editing a byte, you are choosing what the edge of the world looks like.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  /// The block id this control edits.
  property int block: 0

  /// Its graphics come from the current tileset's blockset.
  property int tilesetInd: brg.map.tilesetInd

  /// Emitted when the user chooses one. The parent writes the save; this never does.
  signal picked(int block)

  implicitHeight: row.implicitHeight
  implicitWidth: row.implicitWidth

  // One block, drawn from its 16 tiles. There is no "block" image provider -- a block is only
  // ever an arrangement of tiles -- so we lay the tiles out, which is exactly what the game does.
  component BlockView: Rectangle {
    id: bv
    property int blockId: 0
    property real px: 48          // rendered size of the whole 32x32 block

    implicitWidth: px + 2
    implicitHeight: px + 2

    color: "#ffffff"
    border.width: 1
    border.color: brg.settings.dividerColor
    radius: 2

    readonly property var tiles: brg.map.blockTileIds(bv.blockId)
    readonly property real tilePx: px / 4

    Item {
      anchors.centerIn: parent
      width: bv.px
      height: bv.px

      Repeater {
        model: 16

        Image {
          required property int index

          x: (index % 4) * bv.tilePx
          y: Math.floor(index / 4) * bv.tilePx
          width: bv.tilePx
          height: bv.tilePx

          source: (bv.tiles && bv.tiles.length === 16)
                  ? ("image://tileset/" + brg.map.tilesetName + "/"
                     + brg.map.tileAnimStrFor(brg.map.tileAnim) + "/nofont/0/"
                     + bv.tiles[index] + "/"
                     + Math.max(1, Math.trunc(bv.tilePx)) + "/"
                     + Math.max(1, Math.trunc(bv.tilePx)))
                  : ""

          smooth: false
          mipmap: false
          fillMode: Image.Stretch
          cache: true
        }
      }
    }
  }

  RowLayout {
    id: row
    anchors.fill: parent
    spacing: 8

    BlockView {
      id: current
      blockId: root.block
      px: 48

      Layout.alignment: Qt.AlignVCenter

      border.color: mouse.containsMouse
                    ? brg.settings.primaryColor
                    : brg.settings.dividerColor

      MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: grid.open()
      }
    }

    ColumnLayout {
      Layout.fillWidth: true
      spacing: 1

      Text {
        Layout.fillWidth: true
        text: qsTr("Block %1").arg("$" + root.block.toString(16).toUpperCase().padStart(2, "0"))
        font.pixelSize: 12
        color: brg.settings.textColorDark
        elide: Text.ElideRight
      }

      Text {
        Layout.fillWidth: true
        text: qsTr("Click to change")
        font.pixelSize: 10
        color: brg.settings.textColorMid
        elide: Text.ElideRight
      }
    }
  }

  // ── The picker ──────────────────────────────────────────────────────────────
  Popup {
    id: grid

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay

    width: 470
    height: 470
    padding: 12
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
      color: "#ffffff"
      radius: 6
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 8

      Text {
        text: qsTr("Pick a block")
        font.pixelSize: 14
        font.bold: true
        color: brg.settings.textColorDark
      }

      Text {
        Layout.fillWidth: true
        text: qsTr("Every block %1 is built from.").arg(brg.map.tilesetName)
        font.pixelSize: 11
        color: brg.settings.textColorMid
        elide: Text.ElideRight
      }

      ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        GridView {
          id: view

          // Only the blocks this tileset actually HAS. Unlike a tile id, a block id past the
          // end of the blockset isn't "some other graphic" -- it is nothing at all, and the
          // game can't draw it either. (Map.qml skips those blocks for the same reason.)
          model: brg.map.blockCount

          cellWidth: 54
          cellHeight: 54

          delegate: Item {
            id: cell
            required property int index

            width: view.cellWidth
            height: view.cellHeight

            readonly property bool isCurrent: cell.index === root.block

            Rectangle {
              anchors.centerIn: parent
              width: 50
              height: 50
              radius: 3

              color: cell.isCurrent
                     ? Qt.rgba(0.85, 0.11, 0.38, 0.14)
                     : (cellMouse.containsMouse ? "#f0f0f0" : "transparent")

              border.width: cell.isCurrent ? 2 : 1
              border.color: cell.isCurrent
                            ? brg.settings.primaryColor
                            : (cellMouse.containsMouse
                               ? brg.settings.textColorMid
                               : "transparent")

              BlockView {
                anchors.centerIn: parent
                blockId: cell.index
                px: 40
                border.width: 0
                color: "transparent"
              }
            }

            MouseArea {
              id: cellMouse
              anchors.fill: parent
              hoverEnabled: true
              cursorShape: Qt.PointingHandCursor

              onClicked: {
                root.picked(cell.index);
                grid.close();
              }
            }
          }
        }
      }
    }
  }
}
