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
 * ONE selectable, draggable SIGN on the map -- a placard.
 *
 * The door's quieter sibling, built on exactly the MapWarp / MapSprite machinery: a MouseArea (never
 * PointerHandlers), the same tile-snap-from-the-cursor drag, the same drag-off-to-delete, the same
 * ✎/✕ buttons above the thing rather than over it.
 *
 * A sign has no artwork of its own — the game draws it as whatever tile it sits on — so this is a
 * **chip**: a marked tile in the Signs layer's orange, carrying ▤ and its index, and showing the
 * one thing about a sign you cannot see by looking at the map: **what it says**. That is resolved
 * from this map's text table. See notes/reference/signs.md.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: sign

  /// The canvas, for the zoom and the selection.
  required property var canvas

  /// Which sign this is -- its index in the save's sign list (0..15).
  required property int ind

  /// Map tile coords.
  required property int tileX
  required property int tileY

  /// The sign's real words, one line ("PALLET TOWN / Shades of your…"), or "(scripted text)".
  required property string preview

  /// False when the text id points past this map's text table -- the game would read whatever text
  /// comes next in the cartridge. Shown, never refused.
  required property bool textValid

  signal editRequested()

  // ── Drag state ────────────────────────────────────────────────────────────────────────────
  property int dragX: -1
  property int dragY: -1
  readonly property bool dragging: sign.dragX >= 0

  readonly property int liveX: sign.dragging ? sign.dragX : sign.tileX
  readonly property int liveY: sign.dragging ? sign.dragY : sign.tileY

  readonly property bool selected: sign.canvas.selectedSign === sign.ind

  /// True while the cursor is over the delete zone mid-drag. @see MapCanvas.overDeleteZone
  property bool overBin: false

  // A sign sits ON its tile -- no 4-pixel lift (that is an OAM fact about sprites).
  x: (sign.canvas.mapBorderPx + sign.liveX * 16) * sign.canvas.zoom
  y: (sign.canvas.mapBorderPx + sign.liveY * 16) * sign.canvas.zoom
  width: 16 * sign.canvas.zoom
  height: 16 * sign.canvas.zoom

  z: sign.dragging ? 30 : (sign.selected ? 25 : 1)

  // (Object stacking was removed 2026-07-15; a sign always draws, overlapping or not.)

  // ── The chip ─────────────────────────────────────────────────────────────────────────────
  //
  // The Signs layer's own orange (#e69f00, Okabe-Ito) -- the same ink the Layers panel paints its
  // swatch with, so the row IS the legend and the two can never drift apart.
  Rectangle {
    id: chip
    anchors.fill: parent

    color: sign.textValid ? "#66e69f00" : "#66d55e00"   // vermillion when it points at no real text
    border.width: Math.max(1, Math.round(sign.canvas.zoom))
    border.color: sign.textValid ? "#e69f00" : "#d55e00"
    opacity: sign.dragging ? 0.65 : 1.0

    // ▤. It vanishes when the map is zoomed too far out to draw it legibly.
    Text {
      anchors.centerIn: parent
      visible: sign.canvas.zoom >= 1.5
      text: "▤"
      font.pixelSize: Math.max(8, Math.round(9 * sign.canvas.zoom))
      color: "#212121"
    }
  }

  // 🔫 The id points past this map's text. The game reads whatever text comes next. Drawn, editable,
  // and FLAGGED -- exactly like an out-of-set sprite or a door that points nowhere.
  Text {
    visible: !sign.textValid && sign.canvas.zoom >= 1
    anchors.centerIn: parent
    text: "!"
    font.bold: true
    font.pixelSize: Math.max(9, Math.round(10 * sign.canvas.zoom))
    color: "#d55e00"
  }

  // A selection you can lose under a layer is not a selection: it draws above everything.
  Rectangle {
    visible: sign.selected
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

  // ── What it says, on the chip ────────────────────────────────────────────────────────────
  //
  // The one fact about a sign you cannot get by looking at the map. Shown on the selected one and on
  // hover -- not on all of them at once.
  Rectangle {
    visible: (sign.selected || area.containsMouse) && !sign.dragging
    z: 40

    anchors.bottom: parent.top
    // Clear the ✎/✕ row properly (the buttons are 20px tall on a 3px margin). 32px is a real gap.
    anchors.bottomMargin: sign.selected ? 32 : 4
    anchors.horizontalCenter: parent.horizontalCenter

    width: label.implicitWidth + 12
    height: label.implicitHeight + 6
    radius: 3
    color: "#e6212121"

    Text {
      id: label
      anchors.centerIn: parent
      text: sign.textValid
            ? (sign.preview !== "" ? sign.preview : qsTr("(no text)"))
            : qsTr("id points past this map's text")
      font.pixelSize: 11
      color: sign.textValid ? "white" : "#ffb74d"
    }
  }

  // ── The delete button ────────────────────────────────────────────────────────────────────
  //
  // ABOVE the sign, never over it. No ✎ any more -- a plain CLICK opens the Details panel (see
  // onReleased), so an edit button would just be a second way to do what a click already does.
  Row {
    visible: sign.selected && !sign.dragging
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
          brg.map.removeSign(sign.ind);
          sign.canvas.selectedSign = -1;
          sign.canvas.status = qsTr("Sign removed. The signs after it slid up a slot.");
        }
      }
    }
  }

  // ── The ghost ────────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: ghost

    parent: sign.Window.window ? sign.Window.window.contentItem : sign
    visible: sign.dragging && sign.overBin
    z: 9999

    width: 26
    height: 26
    radius: 3
    color: "#cce69f00"
    border.width: 1
    border.color: "#e69f00"

    Text {
      anchors.centerIn: parent
      text: "▤"
      font.pixelSize: 13
      color: "#212121"
    }
  }

  // ── Input ────────────────────────────────────────────────────────────────────────────────
  MouseArea {
    id: area
    anchors.fill: parent

    enabled: !sign.canvas.panning && sign.canvas.tool !== "zoom" && !sign.canvas.placing
    hoverEnabled: true
    cursorShape: sign.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

    preventStealing: true

    property point press
    property bool moved: false

    onPressed: (m) => {
      area.press = Qt.point(m.x, m.y);
      area.moved = false;

      // One selection, one Details panel.
      sign.canvas.selectedSign = sign.ind;
      sign.canvas.selectedNpc = -1;
      sign.canvas.selectedWarp = -1;
      m.accepted = true;
    }

    onPositionChanged: (m) => {
      if (!area.pressed)
        return;

      if (!area.moved
          && Math.abs(m.x - area.press.x) < 4 && Math.abs(m.y - area.press.y) < 4)
        return;

      area.moved = true;

      const g = sign.mapToGlobal(m.x, m.y);
      sign.overBin = sign.canvas.overDeleteZone(g.x, g.y);
      sign.canvas.deleteHover = sign.overBin;

      if (sign.overBin) {
        const w = sign.mapToItem(ghost.parent, m.x, m.y);
        ghost.x = w.x - ghost.width / 2;
        ghost.y = w.y - ghost.height / 2;

        sign.dragX = sign.tileX;
        sign.dragY = sign.tileY;
        return;
      }

      // ⚠️ TILE-SNAPPED FROM THE CURSOR, not from an accumulated delta -- same fix as MapWarp/MapSprite.
      const p = sign.canvas.tileAtGlobal(g.x, g.y);

      sign.dragX = Math.max(0, Math.min(brg.map.blocksWide * 2 - 1, p.x));
      sign.dragY = Math.max(0, Math.min(brg.map.blocksHigh * 2 - 1, p.y));
    }

    onReleased: (m) => {
      if (!sign.dragging && !area.moved) {
        // A plain CLICK opens the Details panel on this sign; a drag does not.
        sign.editRequested();
        return;
      }

      const nx = sign.dragX;
      const ny = sign.dragY;
      const bin = sign.overBin;

      sign.dragX = -1;
      sign.dragY = -1;
      sign.overBin = false;
      sign.canvas.deleteHover = false;
      area.moved = false;

      if (bin) {
        brg.map.removeSign(sign.ind);
        sign.canvas.selectedSign = -1;
        sign.canvas.status = qsTr("Sign removed. The signs after it slid up a slot.");
        return;
      }

      if (nx < 0 || (nx === sign.tileX && ny === sign.tileY))
        return;   // put back where it started: write nothing

      // Exactly two bytes. tst_signs byte-diffs the whole save across this and demands it.
      brg.map.moveSign(sign.ind, nx, ny);
    }

    onCanceled: {
      sign.dragX = -1;
      sign.dragY = -1;
      sign.overBin = false;
      area.moved = false;
    }
  }

  // Esc, mid-drag: put it back, write NOTHING.
  Connections {
    target: sign.canvas
    function onCancelDragChanged() {
      sign.dragX = -1;
      sign.dragY = -1;
    }
  }
}
