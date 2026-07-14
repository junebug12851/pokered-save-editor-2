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

    // ── The marks ────────────────────────────────────────────────────────────────────────────
    //
    // A dot said "something", which is not information. An exclamation mark says "careful"
    // (Twilight, 2026-07-13). They are the ONLY tooltips in the panel, and each sits ON its own
    // mark -- not on the cell (which would fire every time you swept the grid) and not across the
    // screen from it.
    //
    // ⚠️ **!** and **?** are different marks and they mean different things:
    //
    //   * **!**  this map hasn't loaded this picture -- the console would draw garbage;
    //   * **?**  something you'd want to know, but nothing is wrong.
    //
    // The Player has a **?**, and that is the whole point of the distinction. He used to carry a
    // **!** -- because he is not in any sprite SET -- which made the panel say "this map hasn't
    // loaded the player's picture", and Twilight (rightly) called that weird: the player's picture
    // is loaded on every map in the game. It is loaded, it draws perfectly, and dropping one out
    // gives you a second Red. That is a **?**, not a **!**.
    // ⚠️ The REASON comes from the model, because the reason DEPENDS ON THE MAP: outdoors it is "not
    // in the cartridge's sprite set for this map"; indoors it is "this building is out of video
    // memory" -- indoor maps have no sprite set at all. @see MapModel::vramPictures.
    MapWarnIcon {
      visible: !cell.character.inSpriteSet

      anchors.top: parent.top
      anchors.right: parent.right
      anchors.margins: 2

      text: cell.character.why || ""
      tipWidth: 200
    }

    MapInfoIcon {
      visible: cell.character.inSpriteSet && (cell.character.note || "") !== ""

      anchors.top: parent.top
      anchors.right: parent.right
      anchors.margins: 2

      text: cell.character.note || ""
      tipWidth: 190
    }
  }

  // ── Drag onto the map ────────────────────────────────────────────────────────────────────
  //
  // ⚠️ NO `Drag` / `DropArea`. It broke twice, invisibly, and it is gone from this screen.
  //
  // A MouseArea (which takes an exclusive grab and cannot be stolen by the ScrollView we sit in),
  // a ghost that follows the cursor, and on release **one function call** with a global coordinate:
  // `canvas.dropCharacter(x, y, picture)`. The canvas does a containment test and answers. Nothing
  // hidden, nothing implicit, and it can be reasoned about by reading it.
  MouseArea {
    id: drag
    anchors.fill: parent

    enabled: brg.map.npcRoomLeft() > 0
    preventStealing: true          // the ScrollView does NOT get to steal this
    cursorShape: drag.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor

    property bool dragging: false
    property point origin

    onPressed: (m) => {
      drag.origin = Qt.point(m.x, m.y);
      drag.dragging = false;
    }

    onPositionChanged: (m) => {
      if (!drag.pressed)
        return;

      // A wobble is not a drag.
      if (!drag.dragging
          && Math.abs(m.x - drag.origin.x) < 5 && Math.abs(m.y - drag.origin.y) < 5)
        return;

      drag.dragging = true;

      // The ghost lives in the WINDOW's coordinates (its parent is the window's content item), so
      // the cursor has to be mapped there -- not to the screen.
      const w = cell.mapToItem(ghost.parent, m.x, m.y);
      ghost.x = w.x - ghost.width / 2;
      ghost.y = w.y - ghost.height / 2;
    }

    onReleased: (m) => {
      if (!drag.dragging)
        return;

      drag.dragging = false;

      if (!cell.canvas)
        return;

      const g = cell.mapToGlobal(m.x, m.y);
      cell.canvas.dropCharacter(g.x, g.y, cell.character.ind);
    }

    onCanceled: drag.dragging = false
  }

  // What follows the cursor. Global coordinates, so it goes over anything -- including the map.
  Image {
    id: ghost

    parent: cell.Window.window ? cell.Window.window.contentItem : cell
    visible: drag.dragging
    z: 9999

    width: 32
    height: 32
    source: cell.character.source
    smooth: false
    mipmap: false
    fillMode: Image.PreserveAspectFit
    opacity: 0.85
  }
}
