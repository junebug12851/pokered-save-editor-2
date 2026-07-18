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
 * ONE selectable, draggable DOOR on the map -- a warp point.
 *
 * The sibling of MapSprite.qml, and deliberately built on exactly the same machinery: a MouseArea
 * (never PointerHandlers -- see MapSprite's note, which cost three bugs to learn), the same
 * tile-snap-from-the-cursor drag, the same drag-off-to-delete, the same ✎/✕ buttons above the thing
 * rather than over it.
 *
 * What is different is what it IS. A sprite has artwork; a door has none — the game draws a door as
 * whatever tile it happens to sit on. So this is a **chip**: a marked tile, in the Doors layer's
 * yellow, carrying ⇄ and its index.
 *
 * ⚠️ **The chip has to say where it goes**, because that is the only thing about a door that matters
 * and the only thing you cannot see by looking at the map. A door reading "back outside" (`$FF`)
 * resolves through `wLastMap` — the "Outside is…" chip in the toolbar — LIVE. Change that and every
 * one of these re-labels at once. See notes/reference/warps.md.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: door

  /// The canvas, for the zoom and the selection.
  required property var canvas

  /// Which door this is -- its index in the save's warp list (0..31).
  required property int ind

  /// Map tile coords, and where it leads.
  required property int tileX
  required property int tileY

  /// "Viridian City", or "back outside (Pallet Town)".
  required property string destName

  /// False when the destination map has no arrival point with that index. The console does NOT
  /// bounds-check this: past the end it reads whatever cartridge bytes come next and drops the
  /// player somewhere undefined. **Shown, never refused.**
  required property bool destValid

  /// The `$FF` door -- "back outside". Drawn differently, because it behaves differently.
  required property bool isReturn

  signal editRequested()

  // ── Drag state ────────────────────────────────────────────────────────────────────────────
  //
  // While dragging, the door previews at the tile under the cursor; on release we commit. Esc puts
  // it back and writes NOTHING.
  property int dragX: -1
  property int dragY: -1

  /// Being dragged by its TAB (the canvas proxy-drag). @see MapCanvas.proxyKind
  readonly property bool proxied: door.canvas.proxyKind === "warp"
                                  && door.canvas.proxyInd === door.ind
                                  && door.canvas.proxyX >= 0

  readonly property bool dragging: door.dragX >= 0 || door.proxied

  readonly property int liveX: door.proxied ? door.canvas.proxyX
                             : door.dragX >= 0 ? door.dragX : door.tileX
  readonly property int liveY: door.proxied ? door.canvas.proxyY
                             : door.dragX >= 0 ? door.dragY : door.tileY

  readonly property bool selected: door.canvas.selectedWarp === door.ind

  /// True while the cursor is over the delete zone mid-drag. @see MapCanvas.overDeleteZone
  property bool overBin: false

  // A door sits ON its tile -- no 4-pixel lift. That lift is an OAM fact about sprites; a warp is a
  // map coordinate, and putting it anywhere but on its tile would be a lie about where it is.
  x: (door.canvas.mapBorderPx + door.liveX * 16) * door.canvas.zoom
  y: (door.canvas.mapBorderPx + door.liveY * 16) * door.canvas.zoom
  width: 16 * door.canvas.zoom
  height: 16 * door.canvas.zoom

  z: door.dragging ? 30 : (door.selected ? 25 : 1)

  // (Object stacking was removed 2026-07-15; a door always draws, overlapping or not.)

  // ── The chip ─────────────────────────────────────────────────────────────────────────────
  //
  // The Warps layer's own ink, out of the CANONICAL table (brg.map.ink) -- the same value the
  // Layers panel paints its swatch with, so the row IS the legend and the two can never drift.
  // HOVER brightens the border: the "this is draggable" answer (leadership, 2026-07-18).
  readonly property color layerInk: door.destValid ? brg.map.ink("warps") : brg.map.ink("invalid")
  Rectangle {
    id: chip
    anchors.fill: parent

    // ⚠️ NO FILL (leadership, 2026-07-18: *"Id like the warps and signs to not be filled in
    // either keeping the solid and dashed color language"*). The line carries everything:
    // SOLID = movable, and a touch thicker so it reads over four shades of grey.
    color: "transparent"
    border.width: Math.max(2, Math.round(1.5 * door.canvas.zoom))
    border.color: area.containsMouse && !door.selected
                    ? "#ffffff" : door.layerInk
    opacity: door.dragging ? 0.65 : 1.0

    // A "back outside" door is the ordinary kind, and it gets the plain fill. A door that names a
    // specific map is the unusual one, and it gets a corner tick so you can tell them apart at a
    // glance without reading anything.
    Rectangle {
      visible: !door.isReturn && door.canvas.zoom >= 1
      width: parent.width * 0.3
      height: parent.height * 0.3
      color: chip.border.color
      anchors.top: parent.top
      anchors.right: parent.right
    }

    // ⇄. It vanishes when the map is zoomed too far out to draw it legibly -- a glyph rendered at
    // four pixels is not a glyph, it is noise. In the layer's own ink now that the chip has no
    // fill to sit on (a dark glyph straight over dark artwork disappeared).
    Text {
      anchors.centerIn: parent
      visible: door.canvas.zoom >= 1.5
      text: "⇄"
      font.bold: true
      font.pixelSize: Math.max(8, Math.round(9 * door.canvas.zoom))
      color: door.layerInk
      style: Text.Outline
      styleColor: "#99212121"
    }
  }

  // 🔫 The door points at an arrival point the target map does not have. The console would copy four
  // arbitrary ROM bytes into the view pointer and the player's coordinates. It is drawn, it is
  // editable, and it is FLAGGED -- exactly like an out-of-set sprite.
  Text {
    visible: !door.destValid && door.canvas.zoom >= 1
    anchors.centerIn: parent
    text: "!"
    font.bold: true
    font.pixelSize: Math.max(9, Math.round(10 * door.canvas.zoom))
    color: brg.map.ink("invalid")
  }

  // A selection you can lose under a layer is not a selection: it draws above everything.
  // ⭐ WHITE, the one selection colour everywhere on the canvas (a sprite's silhouette turns
  // white too) -- it used to be the People-layer pink, which said "sprite" on a door.
  Rectangle {
    visible: door.selected
    z: 20
    anchors.fill: parent
    anchors.margins: -2
    color: "transparent"
    border.width: 2
    border.color: "#ffffff"

    Rectangle {
      anchors.fill: parent
      anchors.margins: 2
      color: "transparent"
      border.width: 1
      border.color: "#cc212121"
    }
  }

  // ── Where it goes, on the chip ───────────────────────────────────────────────────────────
  //
  // The one fact about a door you cannot get by looking at the map. Shown on the selected one and on
  // hover -- not on all of them at once, which on a map with a dozen doors is a wall of text over the
  // thing you are trying to look at.
  Rectangle {
    visible: (door.selected || area.containsMouse) && !door.dragging
    z: 40

    anchors.bottom: parent.top
    // ⚠️ Clear the ✎/✕ row, and clear it PROPERLY. The buttons are 20px tall on a 3px margin, so they
    // occupy 3..23px above the door -- and a 26px margin left a 3px gap that read as "touching" on
    // the screenshot review. 32px is a real gap. (Cramping is a bug, not a detail.)
    anchors.bottomMargin: door.selected ? 32 : 4
    anchors.horizontalCenter: parent.horizontalCenter

    width: label.implicitWidth + 12
    height: label.implicitHeight + 6
    radius: 3
    color: "#e6212121"

    Text {
      id: label
      anchors.centerIn: parent
      text: door.destValid
            ? qsTr("→ %1").arg(door.destName)
            : qsTr("→ %1 — no such warp there").arg(door.destName)
      font.pixelSize: 11
      color: door.destValid ? "white" : "#ffb74d"
    }
  }

  // ── The delete button ────────────────────────────────────────────────────────────────────
  //
  // ABOVE the door, never over it: a delete button drawn on top of the thing you are trying to look
  // at is worse than no button. There is no ✎ any more -- a plain CLICK opens the Details panel now
  // (see onReleased), so an edit button would be a second way to do the thing a click already does.
  Row {
    visible: door.selected && !door.dragging
    z: 45
    anchors.bottom: parent.top
    anchors.bottomMargin: 3
    anchors.horizontalCenter: parent.horizontalCenter
    spacing: 3

    Rectangle {
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
          brg.map.removeWarp(door.ind);
          door.canvas.selectedWarp = -1;
          door.canvas.status = qsTr("Warp removed. The warps after it slid up a slot.");
        }
      }
    }
  }

  // ── The ghost ────────────────────────────────────────────────────────────────────────────
  //
  // It only appears once you have dragged OFF the map, over the panel where dropping means delete.
  // On the map itself the door previews in place, which reads better.
  Rectangle {
    id: ghost

    parent: door.Window.window ? door.Window.window.contentItem : door
    visible: door.dragging && door.overBin
    z: 9999

    width: 26
    height: 26
    radius: 3
    color: Qt.alpha(door.layerInk, 0.8)
    border.width: 1
    border.color: door.layerInk

    Text {
      anchors.centerIn: parent
      text: "⇄"
      font.pixelSize: 13
      color: "#212121"
    }
  }

  // ── Input ────────────────────────────────────────────────────────────────────────────────
  MouseArea {
    id: area
    anchors.fill: parent

    enabled: !door.canvas.panning && door.canvas.tool !== "zoom" && !door.canvas.placing
    hoverEnabled: true
    cursorShape: door.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

    // ⭐ A movable thing under the pointer: the cell hands its highlight over and the tabs
    // withdraw -- the same contract the sprites keep. @see MapCanvas.hoverMovable
    onContainsMouseChanged: {
      const key = Math.floor((door.canvas.mapBorderPx + door.liveX * 16) / 32) + ","
                + Math.floor((door.canvas.mapBorderPx + door.liveY * 16) / 32);
      if (area.containsMouse) {
        door.canvas.hoverMovable = key;
        door.canvas.hoverMovableFullCell = false;
      } else if (door.canvas.hoverMovable === key) {
        door.canvas.hoverMovable = "";
      }
    }

    // Do not let the Flickable steal the drag and pan the map out from under us.
    preventStealing: true

    property point press
    property bool moved: false

    onPressed: (m) => {
      area.press = Qt.point(m.x, m.y);
      area.moved = false;

      // Selecting a door deselects any sprite, and vice versa. One selection, one Details panel.
      door.canvas.selectedWarp = door.ind;
      door.canvas.selectedNpc = -1;
      m.accepted = true;
    }

    onPositionChanged: (m) => {
      if (!area.pressed)
        return;

      // A tiny wobble on a click is not a drag.
      if (!area.moved
          && Math.abs(m.x - area.press.x) < 4 && Math.abs(m.y - area.press.y) < 4)
        return;

      area.moved = true;

      // Over the left dock? Then this is a delete-in-progress, not a move.
      const g = door.mapToGlobal(m.x, m.y);
      door.overBin = door.canvas.overDeleteZone(g.x, g.y);
      door.canvas.deleteHover = door.overBin;

      if (door.overBin) {
        const w = door.mapToItem(ghost.parent, m.x, m.y);
        ghost.x = w.x - ghost.width / 2;
        ghost.y = w.y - ghost.height / 2;

        door.dragX = door.tileX;   // keep the preview where it was; it is on its way to the bin
        door.dragY = door.tileY;
        return;
      }

      // ⚠️ TILE-SNAPPED FROM THE CURSOR, not from an accumulated delta -- the same fix, for the same
      // reason, as MapSprite. A delta drifts: the door's own coordinate moves under you while the
      // press point stays put, the two disagree by a tile, and the thing skitters.
      const p = door.canvas.tileAtGlobal(g.x, g.y);

      door.dragX = Math.max(0, Math.min(brg.map.blocksWide * 2 - 1, p.x));
      door.dragY = Math.max(0, Math.min(brg.map.blocksHigh * 2 - 1, p.y));
    }

    onReleased: (m) => {
      if (!door.dragging && !area.moved) {
        // A plain CLICK -- no drag -- opens the Details panel on this door (Twilight: "details
        // should only open on a click not click and drag"). The press already selected it.
        door.editRequested();
        return;
      }

      const nx = door.dragX;
      const ny = door.dragY;
      const bin = door.overBin;

      door.dragX = -1;
      door.dragY = -1;
      door.overBin = false;
      door.canvas.deleteHover = false;
      area.moved = false;

      if (bin) {
        brg.map.removeWarp(door.ind);
        door.canvas.selectedWarp = -1;
        door.canvas.status = qsTr("Warp removed. The warps after it slid up a slot.");
        return;
      }

      if (nx < 0 || (nx === door.tileX && ny === door.tileY))
        return;   // put back where it started: write nothing

      // Exactly two bytes. tst_warps byte-diffs the whole save across this and demands it.
      brg.map.moveWarp(door.ind, nx, ny);
    }

    // The pointer left in a way we did not see (a grab was stolen, the window lost focus). Do NOT
    // leave a half-drag hanging off the cursor -- that is the "stuck to the mouse" bug.
    onCanceled: {
      door.dragX = -1;
      door.dragY = -1;
      door.overBin = false;
      area.moved = false;
    }
  }

  // Esc, mid-drag: put it back, write NOTHING.
  Connections {
    target: door.canvas
    function onCancelDragChanged() {
      door.dragX = -1;
      door.dragY = -1;
    }
  }
}
