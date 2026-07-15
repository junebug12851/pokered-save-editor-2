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
 * ONE selectable edge CONNECTION on the map -- the strip where a neighbouring map bleeds into the ring.
 *
 * The sibling of MapWarp.qml / MapSign.qml, and built on the same idea: a MouseArea, a select, a ✕, a
 * click-to-edit. What is different is the DRAG. A connection has no free X/Y -- it has one signed
 * OFFSET, and sliding it along the shared edge IS setting that offset (the nine derived bytes follow;
 * see notes/reference/map-connections.md). So the drag is constrained to the edge's one axis, snaps to
 * whole blocks, and magnetises to the landmark positions (corner-aligned / centred / flush).
 *
 * ⚠️ NOT a Repeater delegate. An offset edit re-derives the strip and bumps `canvas.revision`, which
 * would rebuild a delegate mid-drag and drop the gesture. There are only ever four connections, so the
 * canvas places four fixed MapConnection items (one per direction) that bind to the model in place.
 */
import QtQuick

Item {
  id: conn

  /// The canvas, for the zoom and the selection.
  required property var canvas

  /// Which edge this is -- MapDBEntryConnect::ConnectDir (N 0, S 1, E 2, W 3).
  required property int dir

  /// The strip geometry ({ x, y, w, h, dirName, name } in buffer px) and the edit info
  /// ({ offset, synced, offsetMin, offsetMax, snaps }) -- looked up from the canvas by direction, both
  /// bound to `canvas.revision` so they update in place when the offset changes.
  readonly property var strip: { conn.canvas.revision; return conn.canvas.connStripFor(conn.dir); }
  readonly property var edge:  { conn.canvas.revision; return conn.canvas.connEdgeFor(conn.dir); }

  readonly property bool present: conn.strip !== null && conn.edge !== null && conn.edge.exists === true

  readonly property bool horizontal: conn.dir === 0 || conn.dir === 1   // N / S slide along X; E / W along Y
  readonly property bool selected: conn.canvas.selectedConnection === conn.dir

  signal editRequested()

  visible: present && brg.mapLayers.showConnections

  // Geometry straight from the model, in buffer px * zoom -- QML does no arithmetic of its own.
  x: present ? conn.strip.x * conn.canvas.zoom : 0
  y: present ? conn.strip.y * conn.canvas.zoom : 0
  width: present ? conn.strip.w * conn.canvas.zoom : 0
  height: present ? conn.strip.h * conn.canvas.zoom : 0

  z: conn.dragging ? 30 : (conn.selected ? 25 : 2)

  // ── Drag state ────────────────────────────────────────────────────────────────────────────
  property bool dragging: false
  property real pressPos: 0        // where along the axis the press landed (item px)
  property int  baseOffset: 0      // the offset when the drag began
  property string snapName: ""     // the landmark we are currently magnetised to, "" for none

  readonly property real blockPx: 32 * conn.canvas.zoom

  /// Snap @p off to the nearest landmark within one block; sets snapName. Returns the (possibly
  /// snapped) offset, clamped to the legal range.
  function withSnap(off) {
    conn.snapName = "";
    const lo = conn.edge.offsetMin, hi = conn.edge.offsetMax;
    off = Math.max(lo, Math.min(hi, off));

    const snaps = conn.edge.snaps || [];
    for (let i = 0; i < snaps.length; i++) {
      if (Math.abs(off - snaps[i].offset) <= 1) {
        conn.snapName = snaps[i].name;
        return snaps[i].offset;
      }
    }
    return off;
  }

  // ── The outline ──────────────────────────────────────────────────────────────────────────
  //
  // No fill (Twilight, 2026-07-13): an outline shows you the strip, a wash hides the map under it. The
  // ring's own vermillion (#d55e00, Okabe-Ito) when idle; the selection purple when picked.
  Rectangle {
    anchors.fill: parent
    color: "transparent"
    border.width: Math.max(2, Math.round(conn.canvas.zoom))
    border.color: conn.selected ? "#cc79a7" : "#d55e00"
    opacity: conn.dragging ? 0.8 : 1.0

    // A dashed inner edge when the connection has been RAW-edited (desynced): the offset no longer
    // describes it, so the slider/handle can't, and we say so quietly rather than lie.
    Rectangle {
      visible: conn.present && conn.edge.synced === false
      anchors.fill: parent
      anchors.margins: 3
      color: "transparent"
      border.width: 1
      border.color: "#ffd54f"
    }
  }

  // The grab handle -- a small disc in the middle of the strip, so there is an obvious thing to slide.
  Rectangle {
    anchors.centerIn: parent
    visible: conn.canvas.zoom >= 0.6
    width: 18; height: 18; radius: 9
    color: drag.containsMouse || conn.dragging ? "#d55e00" : "#e6212121"
    border.width: 1
    border.color: "#ffffff"

    Text {
      anchors.centerIn: parent
      text: conn.horizontal ? "↔" : "↕"
      font.pixelSize: 12
      color: "white"
    }
  }

  // The selection ring, above everything, like the door's.
  Rectangle {
    visible: conn.selected
    z: 20
    anchors.fill: parent
    anchors.margins: -2
    color: "transparent"
    border.width: 2
    border.color: "#cc79a7"
  }

  // ── The label ──────────────────────────────────────────────────────────────────────────────
  //
  // Which neighbour, which way, and -- while dragging -- the live offset and the landmark it has
  // snapped to. Shown on the selected one, on hover, and during a drag.
  Rectangle {
    visible: conn.present && (conn.selected || drag.containsMouse || conn.dragging)
    z: 40
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.top
    anchors.bottomMargin: conn.selected ? 30 : 4

    width: lbl.implicitWidth + 12
    height: lbl.implicitHeight + 6
    radius: 3
    color: "#e6212121"

    Text {
      id: lbl
      anchors.centerIn: parent
      text: {
        if (!conn.present) return "";
        let s = conn.edge.dirName + " · " + conn.edge.toName;
        if (conn.dragging || conn.selected)
          s += "  (offset " + conn.edge.offset
             + (conn.snapName !== "" ? " · " + conn.snapName : "") + ")";
        return s;
      }
      font.pixelSize: 11
      color: "white"
    }
  }

  // ── The delete button ────────────────────────────────────────────────────────────────────
  //
  // Above the strip, never over it (the door's rule). Removing a connection clears one flag bit.
  Rectangle {
    visible: conn.selected && !conn.dragging
    z: 45
    anchors.bottom: parent.top
    anchors.bottomMargin: 3
    anchors.horizontalCenter: parent.horizontalCenter
    width: 20; height: 20; radius: 10
    color: delArea.containsMouse ? "#d55e00" : "#212121"
    border.width: 1
    border.color: "#ffffff"

    Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; color: "white" }

    MouseArea {
      id: delArea
      anchors.fill: parent
      hoverEnabled: true
      cursorShape: Qt.PointingHandCursor
      onClicked: (m) => {
        m.accepted = true;
        brg.map.removeConnection(conn.dir);
        conn.canvas.selectedConnection = -1;
        conn.canvas.status = qsTr("Connection removed from the %1 edge.").arg(conn.edge.dirName);
      }
    }
  }

  // ── Input ────────────────────────────────────────────────────────────────────────────────
  MouseArea {
    id: drag
    anchors.fill: parent
    enabled: conn.present && !conn.canvas.panning && conn.canvas.tool !== "zoom" && !conn.canvas.placing
    hoverEnabled: true
    cursorShape: conn.dragging ? (conn.horizontal ? Qt.SizeHorCursor : Qt.SizeVerCursor)
                               : Qt.PointingHandCursor
    preventStealing: true

    property bool moved: false

    onPressed: (m) => {
      conn.canvas.selectedConnection = conn.dir;
      conn.baseOffset = conn.edge.offset;
      conn.pressPos = conn.horizontal ? m.x : m.y;
      drag.moved = false;
      m.accepted = true;
    }

    onPositionChanged: (m) => {
      if (!drag.pressed) return;

      const cur = conn.horizontal ? m.x : m.y;
      const dpx = cur - conn.pressPos;
      if (!drag.moved && Math.abs(dpx) < 4) return;
      drag.moved = true;

      // Slide along the edge -> a whole-block change in offset. A desynced connection can't be driven
      // by the offset knob (its bytes no longer match it), so a drag first re-syncs it to its own
      // recovered offset and then moves from there.
      const deltaBlocks = Math.round(dpx / conn.blockPx);
      const want = conn.withSnap(conn.baseOffset + deltaBlocks);

      if (want !== conn.edge.offset)
        brg.map.setConnectionOffset(conn.dir, want);
      conn.dragging = true;
    }

    onReleased: () => {
      if (!drag.moved) {
        conn.editRequested();   // a plain click opens the Details panel on this connection
        return;
      }
      conn.dragging = false;
      conn.snapName = "";
      drag.moved = false;
    }

    onCanceled: { conn.dragging = false; conn.snapName = ""; drag.moved = false; }
  }

  // Esc mid-drag: put the offset back where it began, write nothing further.
  Connections {
    target: conn.canvas
    function onCancelDragChanged() {
      if (conn.dragging) {
        brg.map.setConnectionOffset(conn.dir, conn.baseOffset);
        conn.dragging = false;
        conn.snapName = "";
      }
    }
  }
}
