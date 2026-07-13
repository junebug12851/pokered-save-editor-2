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
  TilePick.qml -- pick a TILE by looking at it.

  This is the component that keeps the promise in principles.md -> "Every Byte, None of Them
  Raw". The grass tile and the three counter tiles are save bytes, and a byte is not a number
  box: it is a *tile*, so you pick it by seeing the tiles.

  The current tile is shown, magnified, with what it DOES written next to it. Click it and the
  whole tileset opens as a grid of the real 8x8 graphics; hovering one says what that tile is.

  Both halves of the rule are here:
    * every value is REACHABLE -- the grid is all 256 ids, including the ones past the
      tileset's own 0x00-0x5F graphics, because a glitch hunter is a legitimate user; and
    * "None" ($FF) is a first-class choice, not a magic number you have to know to type.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  /// The tile id this control edits. 0xFF (255) means "none".
  property int tile: 0xFF

  /// The tileset its graphics come from -- the name, not the index (the provider wants a name).
  property string tilesetName: brg.map.tilesetName

  /// Which tiles animate: "indoor" / "cave" / "outdoor". The tiles are drawn the way the save
  /// says they'd move, so a water tile picked in a cave ripples in the picker too.
  property string tileAnimStr: {
    switch (brg.map.tileAnim) {
      case 1:  return "cave";
      case 2:  return "outdoor";
      default: return "indoor";
    }
  }

  /// Shown when the tile is 0xFF, e.g. "No grass on this tileset".
  property string noneLabel: qsTr("None")

  /// May this be un-set from the row itself? True wherever "nothing" is a value the byte can hold --
  /// which, for a tile, it always is ($FF).
  property bool clearable: true

  /// Emitted when the user chooses one. The parent writes the save; this never does.
  signal picked(int tile)

  readonly property int noTile: 0xFF

  implicitHeight: row.implicitHeight
  implicitWidth: row.implicitWidth

  // A tile, drawn big. `sizeMult` scales the whole 128x128 sheet, and `tileInd` crops one
  // 8x8 out of it -- the same id format the keyboard's TileGlyph uses.
  component Swatch: Rectangle {
    id: sw
    property int tileId: root.noTile
    property real mult: 4

    implicitWidth: 8 * mult + 2
    implicitHeight: 8 * mult + 2

    color: "#ffffff"
    border.width: 1
    border.color: brg.settings.dividerColor
    radius: 2

    Image {
      anchors.centerIn: parent
      width: 8 * sw.mult
      height: 8 * sw.mult

      // Nothing to draw for "none" -- and an empty square says that better than a glyph would.
      visible: sw.tileId !== root.noTile

      // image://tileset/<tileset>/<type>/<font>/<frame>/<tileInd>/<w>/<h>  (see TilesetProvider)
      source: sw.tileId === root.noTile
              ? ""
              : ("image://tileset/" + root.tilesetName + "/" + root.tileAnimStr
                 + "/nofont/0/" + sw.tileId + "/"
                 + Math.trunc(8 * sw.mult) + "/" + Math.trunc(8 * sw.mult))

      smooth: false   // 8x8 pixel art -- never interpolate it
      mipmap: false
      fillMode: Image.Stretch
      cache: true
    }

    // The "none" square reads as an empty slot, not as a broken image.
    Text {
      anchors.centerIn: parent
      visible: sw.tileId === root.noTile
      text: "—"
      font.pixelSize: Math.max(9, sw.mult * 2.5)
      color: brg.settings.dividerColor
    }
  }

  RowLayout {
    id: row
    anchors.fill: parent
    spacing: 8

    // The button IS the tile. Clicking the thing you want to change is the whole idea.
    Swatch {
      id: current
      tileId: root.tile
      mult: 4

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

    // What it IS, in words. The number is deliberately secondary -- it's the last thing on the
    // line, small and grey, for the people who do want it.
    ColumnLayout {
      Layout.fillWidth: true
      spacing: 1

      Text {
        Layout.fillWidth: true
        text: root.tile === root.noTile
              ? root.noneLabel
              : brg.map.tileInfo(root.tile).label
        font.pixelSize: 12
        color: brg.settings.textColorDark
        elide: Text.ElideRight
      }

      Text {
        Layout.fillWidth: true
        visible: root.tile !== root.noTile
        text: qsTr("Tile %1").arg("$" + root.tile.toString(16).toUpperCase().padStart(2, "0"))
        font.pixelSize: 10
        color: brg.settings.textColorMid
        elide: Text.ElideRight
      }
    }

    // ── Un-set it, right here ────────────────────────────────────────────────────────────────
    //
    // "None" ($FF) has always been a choice inside the picker, but you had to OPEN the picker to
    // reach it -- which is a strange thing to have to do to remove something (Twilight, 2026-07-13:
    // "allow unselecting things ... like how counters has an empty value"). Now the row clears itself.
    //
    // It only appears when there IS something to clear, so it never sits there looking like an
    // action you have not taken.
    MapRailButton {
      visible: root.clearable && root.tile !== root.noTile
      size: 22
      glyph: "×"
      tip: qsTr("Clear this — set it to none")
      Layout.alignment: Qt.AlignVCenter

      onClicked: root.picked(root.noTile)
    }
  }

  // ── The picker ──────────────────────────────────────────────────────────────
  Popup {
    id: grid

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay

    width: 420
    height: 460
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
        text: qsTr("Pick a tile")
        font.pixelSize: 14
        font.bold: true
        color: brg.settings.textColorDark
      }

      // Hovering a tile tells you what it does, right here, before you commit to it.
      Text {
        id: hint
        Layout.fillWidth: true
        text: qsTr("Hover a tile to see what it does.")
        font.pixelSize: 11
        color: brg.settings.textColorMid
        elide: Text.ElideRight
      }

      // "None" is a real choice with a real button, not a number you have to know.
      Button {
        Layout.fillWidth: true
        flat: true
        text: root.noneLabel
        font.pixelSize: 11

        highlighted: root.tile === root.noTile
        onClicked: {
          root.picked(root.noTile);
          grid.close();
        }
      }

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      // Every id the byte can hold. 0x00-0x5F is the tileset's own graphics; past that is the
      // text-box and font region, which no real map uses -- but the byte can still hold it, and
      // "every value reachable" means exactly that. They simply draw as what they draw.
      ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        GridView {
          id: view
          model: 256
          cellWidth: 38
          cellHeight: 38

          delegate: Item {
            id: cell
            required property int index

            width: view.cellWidth
            height: view.cellHeight

            readonly property var info: brg.map.tileInfo(cell.index)
            readonly property bool isCurrent: cell.index === root.tile

            Rectangle {
              anchors.centerIn: parent
              width: 36
              height: 36
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

              Image {
                anchors.centerIn: parent
                width: 24
                height: 24

                source: "image://tileset/" + root.tilesetName + "/" + root.tileAnimStr
                        + "/nofont/0/" + cell.index + "/24/24"

                smooth: false
                mipmap: false
                fillMode: Image.Stretch
                cache: true
              }
            }

            MouseArea {
              id: cellMouse
              anchors.fill: parent
              hoverEnabled: true
              cursorShape: Qt.PointingHandCursor

              onEntered: hint.text = qsTr("$%1 — %2")
                           .arg(cell.index.toString(16).toUpperCase().padStart(2, "0"))
                           .arg(cell.info.label)
              onExited: hint.text = qsTr("Hover a tile to see what it does.")

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
