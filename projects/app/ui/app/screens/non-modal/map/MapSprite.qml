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
 * ONE selectable, draggable sprite on the map. Used for the NPCs **and the player** -- he is slot 0
 * and there was never a reason he should be the one thing you could not pick up.
 *
 * ⚠️ **A MouseArea, not PointerHandlers.** The first version used a TapHandler + a DragHandler, and
 * three separate bugs came out of it:
 *
 *   * **you could not CLICK a sprite** -- only click-and-drag would select one, because the
 *     DragHandler took the grab before the TapHandler ever completed;
 *   * **the ✎ button opened the panel in map-details mode** -- the canvas's own TapHandler fired
 *     *as well* and cleared the selection out from under it;
 *   * **the mouse got stuck to a sprite** with no button held.
 *
 * A MouseArea takes an exclusive grab on press and *consumes* the event, so nothing underneath fires,
 * the press/release pairing is unambiguous, and there is nothing left to get stuck on.
 *
 * ⚠️ **The drag GHOST is the drag source.** Qt's DropArea decides what you are over by geometric
 * overlap with the dragged item. The sprite itself is clamped to the map, so it can never overlap the
 * Characters panel -- which is exactly why dragging a sprite out to delete it did nothing. The ghost
 * is reparented to the window, follows the cursor anywhere, and is what the DropArea sees.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: sprite

  /// The canvas, for the zoom and the selection.
  required property var canvas

  /// Sprite slot. 0 is the player.
  required property int slot

  /// Map tile coords (the +4 bias already taken off), the artwork, and the two flags.
  required property int tileX
  required property int tileY
  required property string art
  required property bool inSet

  readonly property bool isPlayer: sprite.slot === 0

  signal editRequested()

  // ── Drag state ────────────────────────────────────────────────────────────────────────────
  //
  // While dragging, the sprite previews at the tile under the cursor; on release we commit. Esc
  // puts it back and writes NOTHING.
  property int dragX: -1
  property int dragY: -1
  readonly property bool dragging: sprite.dragX >= 0

  readonly property int liveX: sprite.dragging ? sprite.dragX : sprite.tileX
  readonly property int liveY: sprite.dragging ? sprite.dragY : sprite.tileY

  readonly property bool selected: sprite.canvas.selectedNpc === sprite.slot

  // The 4-pixel lift -- where the console's own OAM puts a sprite.
  x: (sprite.canvas.mapBorderPx + sprite.liveX * 16) * sprite.canvas.zoom
  y: (sprite.canvas.mapBorderPx + sprite.liveY * 16 - 4) * sprite.canvas.zoom
  width: 16 * sprite.canvas.zoom
  height: 16 * sprite.canvas.zoom

  z: sprite.dragging ? 30 : (sprite.selected ? 25 : 0)

  PixelImage {
    anchors.fill: parent
    source: sprite.art
    opacity: sprite.dragging ? 0.6 : 1.0
  }

  // The one thing you cannot see by looking at a sprite: whether this map has actually LOADED its
  // picture. If it hasn't, the console draws garbage there. (The player is always loaded.)
  Rectangle {
    visible: !sprite.inSet && !sprite.selected && !sprite.isPlayer
    anchors.fill: parent
    color: "transparent"
    border.width: 1
    border.color: "#ffd54f"
    opacity: 0.9
  }

  // A selection you can lose under a layer is not a selection: it draws above everything.
  Rectangle {
    visible: sprite.selected
    z: 20
    anchors.fill: parent
    anchors.margins: -2
    color: "transparent"
    border.width: 2
    border.color: "#cc79a7"

    Rectangle {
      anchors.fill: parent
      anchors.margins: 2
      color: "transparent"
      border.width: 1
      border.color: "#ccffffff"
    }
  }

  // ── The buttons ──────────────────────────────────────────────────────────────────────────
  //
  // On the selected sprite, ABOVE it -- a delete button drawn over the character you are trying to
  // look at is worse than no button. The player has no ✕: the game requires him.
  Row {
    visible: sprite.selected && !sprite.dragging
    z: 40
    anchors.bottom: parent.top
    anchors.bottomMargin: 3
    anchors.horizontalCenter: parent.horizontalCenter
    spacing: 3

    Rectangle {
      width: 20; height: 20; radius: 10
      color: editArea.containsMouse ? "#56b4e9" : "#212121"
      border.width: 1
      border.color: "#ffffff"

      Text {
        anchors.centerIn: parent
        text: "✎"
        font.pixelSize: 11
        color: "white"
      }

      MouseArea {
        id: editArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        // ⚠️ Consumes the click. The canvas used to get it too and clear the selection, so the panel
        // opened on "the map" instead of on the sprite you had just asked to edit.
        onClicked: (m) => { m.accepted = true; sprite.editRequested(); }
      }
    }

    Rectangle {
      visible: !sprite.isPlayer
      width: 20; height: 20; radius: 10
      color: delArea.containsMouse ? "#d55e00" : "#212121"
      border.width: 1
      border.color: "#ffffff"

      Text {
        anchors.centerIn: parent
        text: "✕"
        font.pixelSize: 11
        color: "white"
      }

      MouseArea {
        id: delArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: (m) => {
          m.accepted = true;
          brg.map.removeNpc(sprite.slot);
          sprite.canvas.selectedNpc = -1;
          sprite.canvas.status = qsTr("Removed. The sprites after it slid up a slot.");
        }
      }
    }
  }

  // ── The ghost: what the DropArea actually sees ───────────────────────────────────────────
  Image {
    id: ghost

    parent: sprite.Window.window ? sprite.Window.window.contentItem : sprite
    visible: sprite.dragging
    z: 9999

    width: 32
    height: 32
    source: sprite.art
    smooth: false
    mipmap: false
    fillMode: Image.PreserveAspectFit
    opacity: 0.9

    /// What the Characters panel's DropArea reads off us. The player cannot be deleted, so he does
    /// not offer a slot to delete.
    property int spriteSlot: sprite.isPlayer ? -1 : sprite.slot

    Drag.active: sprite.dragging && !sprite.isPlayer
    Drag.source: ghost
    Drag.keys: ["pse/map-sprite"]
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
  }

  // ── Input ────────────────────────────────────────────────────────────────────────────────
  MouseArea {
    id: area
    anchors.fill: parent

    enabled: !sprite.canvas.panning && sprite.canvas.tool !== "zoom"
    hoverEnabled: true
    cursorShape: sprite.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

    // Do not let the Flickable steal the drag and pan the map out from under us.
    preventStealing: true

    property point press
    property bool moved: false

    onPressed: (m) => {
      area.press = Qt.point(m.x, m.y);
      area.moved = false;
      sprite.canvas.selectedNpc = sprite.slot;   // a CLICK selects. It always should have.
      m.accepted = true;
    }

    onPositionChanged: (m) => {
      if (!area.pressed)
        return;

      const step = 16 * sprite.canvas.zoom;

      const dx = Math.round((m.x - area.press.x) / step);
      const dy = Math.round((m.y - area.press.y) / step);

      // A tiny wobble on a click is not a drag.
      if (!area.moved
          && Math.abs(m.x - area.press.x) < 4 && Math.abs(m.y - area.press.y) < 4)
        return;

      area.moved = true;

      sprite.dragX = Math.max(0, Math.min(brg.map.blocksWide * 2 - 1, sprite.tileX + dx));
      sprite.dragY = Math.max(0, Math.min(brg.map.blocksHigh * 2 - 1, sprite.tileY + dy));

      // Keep the ghost under the cursor, in SCENE coordinates.
      const scene = sprite.mapToItem(null, m.x, m.y);
      ghost.x = scene.x - ghost.width / 2;
      ghost.y = scene.y - ghost.height / 2;
    }

    onReleased: {
      if (!sprite.dragging) {
        area.moved = false;
        return;      // it was a click, and the click already selected
      }

      const nx = sprite.dragX;
      const ny = sprite.dragY;

      sprite.dragX = -1;
      sprite.dragY = -1;
      area.moved = false;

      // Dropped on the Characters panel? Then it was a DELETE, and the panel has already done it.
      if (!sprite.isPlayer && ghost.Drag.drop() === Qt.MoveAction)
        return;

      if (nx === sprite.tileX && ny === sprite.tileY)
        return;   // put back where it started: write nothing

      if (sprite.isPlayer)
        brg.map.movePlayer(nx, ny);
      else
        brg.map.moveNpc(sprite.slot, nx, ny);
    }

    // The pointer left in a way we did not see (a grab was stolen, the window lost focus). Do NOT
    // leave a half-drag hanging off the cursor -- that is the "stuck to the mouse" bug.
    onCanceled: {
      sprite.dragX = -1;
      sprite.dragY = -1;
      area.moved = false;
    }
  }

  // Esc, mid-drag: put them back, write NOTHING.
  Connections {
    target: sprite.canvas
    function onCancelDragChanged() {
      sprite.dragX = -1;
      sprite.dragY = -1;
    }
  }
}
