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
  BlockPanel.qml -- the BLOCK layer of the map: what you clicked, and what the edge of the
  world is made of.

  A map is a grid of BLOCKS (4x4 tiles, 32x32 px). It is never a grid of tiles. That is the
  distinction this whole panel exists to make honest -- and it is the one the old UI got wrong,
  filing the out-of-bounds BLOCK in among the tileset's TILES.

  Two things live here:

    * The inspector. Click any block on the map and it appears here, drawn big, with its 16
      tiles each saying what they DO -- wall, grass, door, ledge (and which way you jump),
      counter. Hover a tile here and it lights up on the map.

    * The out-of-bounds block (`wMapBackgroundTile`, save 0x2659). The block that fills the
      3-block ring around every map. Editable, as a picker of real blocks -- change it and the
      edge of the world changes in front of you.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // The screen can exist with no save open (the QML smoke test loads every screen cold), so
  // every one of these may be null and every binding below has to survive that.
  readonly property var areaMap: (brg.file.data.dataExpanded
                                  && brg.file.data.dataExpanded.area)
                                 ? brg.file.data.dataExpanded.area.map
                                 : null
  readonly property bool hasMap: areaMap !== null && areaMap !== undefined
                                 && areaMap.outOfBoundsBlock !== undefined

  /// Which tile of the selected block the mouse is over (-1 = none). Map.qml mirrors this.
  property int hoveredTile: -1

  color: "#fafafa"

  Rectangle {
    anchors.left: parent.left
    width: 1
    height: parent.height
    color: "#22000000"
  }

  // A ScrollView, like the Tileset panel -- because this content is genuinely taller than a
  // 480px window and a bare ColumnLayout does not clip, it OVERFLOWS: the empty-state text
  // walked straight over the footer legend. (Caught by the screenshot review, which is why
  // that review is mandatory.)
  ScrollView {
    anchors.fill: parent
    anchors.margins: 12
    clip: true

  ColumnLayout {
    width: panel.width - 24
    spacing: 10

    Text {
      text: qsTr("Blocks")
      font.pixelSize: 14
      font.bold: true
      color: brg.settings.textColorDark
    }

    Text {
      Layout.fillWidth: true
      text: qsTr("A map is a grid of blocks — 4×4 tiles each. Never of single tiles.")
      font.pixelSize: 11
      color: brg.settings.textColorMid
      wrapMode: Text.WordWrap
    }

    // ── The inspector ─────────────────────────────────────────────────────────
    //
    // FIRST in the panel, deliberately. It is the answer to the click, and an answer you have
    // to scroll down to find is not an answer -- clicking a block and having the explanation
    // sit below the fold is exactly the kind of thing the screenshot review is for.
    //
    // A real height, not fillHeight: inside a ScrollView the column has no bounded height to
    // fill, so fillHeight would resolve to nothing and the content would spill out the bottom.
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: brg.map.hasSelection ? 250 : 90
      radius: 4
      color: "#ffffff"
      border.width: 1
      border.color: brg.settings.dividerColor

      // Nothing selected: say how to select something. An empty panel explains nothing.
      ColumnLayout {
        anchors.centerIn: parent
        width: parent.width - 24
        visible: !brg.map.hasSelection
        spacing: 4

        Text {
          Layout.fillWidth: true
          text: qsTr("Click any block on the map")
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
          horizontalAlignment: Text.AlignHCenter
        }

        Text {
          Layout.fillWidth: true
          text: qsTr("and its 16 tiles show up here, each one saying what it does.")
          font.pixelSize: 11
          color: brg.settings.textColorMid
          wrapMode: Text.WordWrap
          horizontalAlignment: Text.AlignHCenter
        }
      }

      ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        visible: brg.map.hasSelection

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Text {
            Layout.fillWidth: true
            text: brg.map.selectedIsBorder
                  ? qsTr("Block $%1 — in the border ring")
                      .arg(Math.max(0, brg.map.selectedBlock).toString(16)
                             .toUpperCase().padStart(2, "0"))
                  : qsTr("Block $%1 — at %2, %3")
                      .arg(Math.max(0, brg.map.selectedBlock).toString(16)
                             .toUpperCase().padStart(2, "0"))
                      .arg(brg.map.selectedMapX).arg(brg.map.selectedMapY)
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
            elide: Text.ElideRight
          }

          Button {
            flat: true
            font.pixelSize: 11
            text: "✕"
            implicitWidth: 26
            implicitHeight: 22
            onClicked: brg.map.clearSelection()
          }
        }

        // The block, big, with every tile hoverable. This IS the block -- same 16 tiles, same
        // 4x4 arrangement the game reads out of the blockset.
        Item {
          Layout.alignment: Qt.AlignHCenter
          implicitWidth: 132
          implicitHeight: 132

          readonly property var tiles: brg.map.hasSelection
                                       ? brg.map.selectedTiles()
                                       : []

          Rectangle {
            anchors.fill: parent
            color: "#ffffff"
            border.width: 1
            border.color: brg.settings.dividerColor
          }

          Repeater {
            model: parent.tiles

            Item {
              required property var modelData
              required property int index

              x: 2 + modelData.x * 32
              y: 2 + modelData.y * 32
              width: 32
              height: 32

              Image {
                anchors.fill: parent
                source: "image://tileset/" + brg.map.tilesetName + "/"
                        + brg.map.tileAnimStrFor(brg.map.tileAnim) + "/nofont/0/"
                        + modelData.tile + "/32/32"
                smooth: false
                mipmap: false
                fillMode: Image.Stretch
                cache: true
              }

              // A wall gets a quiet hatch even without the overlay on, because "can I walk
              // here" is the first question anyone has about a tile.
              Rectangle {
                anchors.fill: parent
                visible: modelData.wall
                color: Qt.rgba(0.21, 0.28, 0.31, 0.22)
              }

              Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.width: tileMouse.containsMouse ? 2 : 0
                border.color: brg.settings.primaryColor
              }

              MouseArea {
                id: tileMouse
                anchors.fill: parent
                hoverEnabled: true

                // Hovering here lights the same tile up on the MAP. Two views of one thing.
                onEntered: panel.hoveredTile = modelData.index
                onExited: if (panel.hoveredTile === modelData.index) panel.hoveredTile = -1
              }
            }
          }
        }

        // What the hovered tile is. Falls back to a summary of the block when nothing is
        // hovered, so the space is never dead.
        Rectangle {
          Layout.fillWidth: true
          implicitHeight: 44
          radius: 3
          color: "#f5f5f5"

          Text {
            anchors.fill: parent
            anchors.margins: 6

            text: {
              const tiles = brg.map.hasSelection ? brg.map.selectedTiles() : [];
              if (panel.hoveredTile >= 0 && panel.hoveredTile < tiles.length) {
                const t = tiles[panel.hoveredTile];
                return qsTr("$%1 — %2")
                         .arg(t.tile.toString(16).toUpperCase().padStart(2, "0"))
                         .arg(t.label);
              }

              const walls = tiles.filter(t => t.wall).length;
              if (walls === 0)      return qsTr("Every tile in this block can be walked on.");
              if (walls === tiles.length) return qsTr("Solid — none of it can be walked on.");
              return qsTr("%1 of its 16 tiles are wall. Hover one to see what it is.")
                       .arg(walls);
            }

            font.pixelSize: 11
            color: brg.settings.textColorDark
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
          }
        }

        Item { Layout.fillHeight: true }
      }
    }

    // ── This map ──────────────────────────────────────────────────────────────
    Rectangle {
      Layout.fillWidth: true
      implicitHeight: sizeRow.implicitHeight + 16
      radius: 4
      color: "#ffffff"
      border.width: 1
      border.color: brg.settings.dividerColor

      ColumnLayout {
        id: sizeRow
        anchors.fill: parent
        anchors.margins: 8
        spacing: 2

        Text {
          text: qsTr("%1 × %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh)
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
        }

        Text {
          Layout.fillWidth: true
          text: qsTr("Built from %1's %2 blocks.")
                  .arg(brg.map.tilesetName).arg(brg.map.blockCount)
          font.pixelSize: 11
          color: brg.settings.textColorMid
          wrapMode: Text.WordWrap
        }
      }
    }

    // ── The edge of the world ─────────────────────────────────────────────────
    Rectangle {
      Layout.fillWidth: true
      implicitHeight: borderCol.implicitHeight + 16
      radius: 4
      color: "#ffffff"
      border.width: 1
      border.color: brg.settings.dividerColor

      ColumnLayout {
        id: borderCol
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        Text {
          text: qsTr("Out-of-bounds block")
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
        }

        Text {
          Layout.fillWidth: true
          text: qsTr("The block filling the 3-block ring around the map. It's what you see "
                   + "past the edge — and what connected maps bleed their edges into.")
          font.pixelSize: 11
          color: brg.settings.textColorMid
          wrapMode: Text.WordWrap
        }

        BlockPick {
          Layout.fillWidth: true
          visible: panel.hasMap

          block: panel.hasMap ? panel.areaMap.outOfBoundsBlock : 0

          // Writes exactly one save byte (0x2659) and nothing else, and the ring redraws.
          onPicked: (b) => {
            if (!panel.hasMap) return;
            panel.areaMap.outOfBoundsBlock = b;
          }
        }

        Button {
          flat: true
          font.pixelSize: 11
          text: brg.map.layerOn(128) ? qsTr("Hide the ring") : qsTr("Show me the ring")
          onClicked: brg.map.toggleLayer(128)   // MapEngine::LayerBorder
        }
      }
    }

    Item { Layout.preferredHeight: 4 }
  }
  }
}
