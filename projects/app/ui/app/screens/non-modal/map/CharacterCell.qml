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

      width: 14
      height: 14
      radius: 7

      color: warnHover.hovered ? "#c79100" : "#ffd54f"
      border.width: 1
      border.color: "#8a6d00"

      // ⚠️ `anchors.centerIn` centres the ITEM's box, and a Label's box carries the font's ascent
      // and descent -- so a "!" (which has neither) ends up shoved down and left. Fill the circle
      // and centre the GLYPH inside it instead. (It was visibly bottom-left; Twilight caught it.)
      Text {
        anchors.fill: parent
        text: "!"
        font.pixelSize: 10
        font.bold: true
        color: "#3a2e00"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
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
  // ⚠️ THE DRAG SOURCE HAS TO ACTUALLY MOVE. Qt's DropArea decides what you are over by GEOMETRIC
  // OVERLAP with the dragged item. The first version left the Drag source anchored inside this
  // panel and floated a separate "ghost" image under the cursor -- so the source was never over the
  // map, the DropArea never fired, and dragging a character out did nothing at all. (It is why
  // deleting did not work either: same bug, other direction.)
  //
  // So the ghost IS the source. It is reparented to the window, it follows the cursor, and it is
  // what the DropArea sees.
  Image {
    id: ghost

    parent: cell.Window.window ? cell.Window.window.contentItem : cell
    visible: dragHandler.active
    z: 9999

    width: 32
    height: 32
    source: cell.character.source
    smooth: false
    mipmap: false
    fillMode: Image.PreserveAspectFit
    opacity: 0.85

    /// What the canvas's DropArea reads off us.
    property int spritePicture: cell.character.ind

    x: dragHandler.centroid.scenePosition.x - width / 2
    y: dragHandler.centroid.scenePosition.y - height / 2

    Drag.active: dragHandler.active
    Drag.source: ghost
    Drag.keys: ["pse/catalog-sprite"]
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
  }

  DragHandler {
    id: dragHandler
    target: null                                  // we position the ghost ourselves
    enabled: brg.map.npcRoomLeft() > 0

    onActiveChanged: {
      if (!active)
        ghost.Drag.drop();
    }
  }
}
