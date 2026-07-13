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
 * One character in the Characters panel. Drag it onto the map to place it.
 *
 * The artwork is drawn at a whole **2×** -- 16×16 art at 24 px is a 1.5× scale, and a fractional
 * scale on pixel art is mush.
 *
 * ⚠️ The **!** badge means *this map has not loaded this character's picture*, so the console would
 * draw it as garbage. It is still perfectly draggable -- the byte allows it, so we allow it -- and
 * the badge is the whole of our objection.
 *
 * **Only the ! has a tooltip**, and it appears on the **!**. The cell itself is silent: a tooltip on
 * every cell fires constantly as you sweep the grid, and one that opens across the screen from the
 * thing it is describing is worse than none.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: cell

  /// One entry of MapModel::spriteCatalog(): `{ ind, name, group, source, inSpriteSet }`.
  required property var character

  /// The canvas -- only so a drop can talk to it. May be null while the dock is still wiring up.
  property var canvas: null

  implicitHeight: 62

  Rectangle {
    id: cellBg
    anchors.fill: parent
    radius: 6
    color: cellHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.04)

    HoverHandler {
      id: cellHover
      cursorShape: Qt.OpenHandCursor
    }

    Column {
      anchors.centerIn: parent
      spacing: 2

      // 32, not 24: a whole 2x. Anything else turns 16x16 pixel art into mush.
      Image {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 32
        height: 32
        source: cell.character.source
        smooth: false
        mipmap: false
        fillMode: Image.PreserveAspectFit
        cache: true
      }

      Label {
        width: cellBg.width - 8
        horizontalAlignment: Text.AlignHCenter
        text: cell.character.name
        font.pixelSize: 9
        elide: Text.ElideRight
        opacity: 0.85
      }
    }

    // ── The ! ────────────────────────────────────────────────────────────────────────────────
    //
    // A dot said "something", which is not information. An exclamation mark says "careful"
    // (Twilight, 2026-07-13).
    Rectangle {
      id: warn
      visible: !cell.character.inSpriteSet

      anchors.top: parent.top
      anchors.right: parent.right
      anchors.margins: 2

      width: 13
      height: 13
      radius: 6.5

      color: warnHover.hovered ? "#c79100" : "#ffd54f"
      border.width: 1
      border.color: "#8a6d00"

      Label {
        anchors.centerIn: parent
        text: "!"
        font.pixelSize: 9
        font.bold: true
        color: "#3a2e00"
      }

      HoverHandler {
        id: warnHover
        cursorShape: Qt.PointingHandCursor
      }

      // The ONLY tooltip in the panel, and it sits ON the mark. Not on the cell (which would fire
      // every time you swept the grid) and not across the screen from it.
      ToolTip {
        visible: warnHover.hovered
        delay: 200

        x: warn.width + 4
        y: -2

        // ⚠️ OPAQUE, and light-on-dark. The stock tooltip is dark text on a translucent background,
        // which over a pale panel is genuinely hard to read (Twilight, 2026-07-13).
        background: Rectangle {
          color: "#212121"
          radius: 4
        }

        contentItem: Label {
          text: qsTr("This map hasn't loaded this picture — the game would draw it as garbage. You can still place it.")
          color: "#ffffff"
          font.pixelSize: 10
          wrapMode: Text.Wrap
          width: 180
        }
      }
    }
  }

  // ── Drag onto the map ────────────────────────────────────────────────────────────────────
  //
  // The drag carries the PICTURE id; the canvas turns the drop point into a tile.
  Item {
    id: dragProxy
    anchors.fill: parent

    property int spritePicture: cell.character.ind

    Drag.active: dragHandler.active
    Drag.source: dragProxy
    Drag.keys: ["pse/catalog-sprite"]
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    DragHandler {
      id: dragHandler
      enabled: brg.map.npcRoomLeft() > 0
      onActiveChanged: {
        if (!active)
          dragProxy.Drag.drop();
      }
    }
  }

  // What actually follows the cursor. Drawing the cell itself would tear a hole in the panel.
  Image {
    parent: cell.Window.window ? cell.Window.window.contentItem : cell
    visible: dragHandler.active
    z: 999

    width: 32
    height: 32
    source: cell.character.source
    smooth: false
    opacity: 0.85

    x: dragHandler.centroid.scenePosition.x - width / 2
    y: dragHandler.centroid.scenePosition.y - height / 2
  }
}
