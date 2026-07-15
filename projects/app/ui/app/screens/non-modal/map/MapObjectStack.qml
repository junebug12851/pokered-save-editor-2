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
 * A STACK of two or more map objects sharing one tile -- the universal grouping Twilight asked for
 * (2026-07-14): *"why don't we have this behaviour with anything. All objects dragged onto one
 * another stack, tabbed like that easily accessible on the left, delete group on right, move group
 * from center."*
 *
 * When a warp, a sign, an NPC and/or the player land on the same tile they would otherwise draw on
 * top of each other and you could only ever grab the top one. So the overlapping chips step aside
 * (each hides itself -- see MapWarp/MapSign/MapSprite `visible`) and ONE of these is drawn there
 * instead: a distinct-coloured group box that, on hover or when one of its members is selected,
 * reveals a small uniform toolbar:
 *
 *      ┌───────────────────────────────┐
 *      │ [tab][tab][tab]  ✥ move   ✕ del│   ← screen-fixed, above the tile
 *      └───────────────────────────────┘
 *              ┌────┐
 *              │ ×3 │   ← the group box, on the tile
 *              └────┘
 *              → where the active one goes
 *
 *   * LEFT  -- one tab per member (its own colour). CLICK a tab = select that piece and open its
 *              Details; DRAG a tab off = move that piece to another tile, which UNMERGES it.
 *   * CENTRE -- ✥, drag to move the WHOLE group to another tile (they stay stacked).
 *   * RIGHT -- ✕, delete every deletable piece here at once (the player is never deletable).
 *
 * Built on exactly the sibling machinery: a MouseArea (never PointerHandlers), tile-snap-FROM-the-
 * cursor (never an accumulated delta -- it drifts), `preventStealing` so the Flickable can't pan the
 * map out from under a drag, and Esc that puts everything back and writes NOTHING. All the byte
 * writing goes through the canvas dispatch (`moveObject`/`removeObject`/`moveGroup`/`removeGroup`),
 * which is the ONLY thing that touches the save. See MapCanvas.qml.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: stack

  /// The canvas, for the zoom, the selection and the dispatch functions.
  required property var canvas

  /// The shared tile.
  required property int tileX
  required property int tileY

  /// The members on this tile. Each: { kind, ind, glyph, art, fill, border, label, deletable, valid }.
  /// @see MapCanvas.stackList
  required property var members

  // ── Which one is "active" ──────────────────────────────────────────────────────────────────
  //
  // A stack is active when one of ITS members is the canvas selection. Reading the three selection
  // properties directly keeps this reactive -- they are not part of `revision`.
  function memberSelected(m) {
    if (m.kind === "warp") return stack.canvas.selectedWarp === m.ind;
    if (m.kind === "sign") return stack.canvas.selectedSign === m.ind;
    return stack.canvas.selectedNpc === m.ind;   // npc or player (slot 0)
  }

  readonly property int activeIndex: {
    // touch the three so the binding re-runs when the selection moves
    stack.canvas.selectedWarp; stack.canvas.selectedSign; stack.canvas.selectedNpc;
    for (let i = 0; i < stack.members.length; i++)
      if (stack.memberSelected(stack.members[i]))
        return i;
    return -1;
  }

  readonly property bool active: stack.activeIndex >= 0
  readonly property var activeMember: stack.members[stack.activeIndex >= 0 ? stack.activeIndex : 0]

  // Rolled over the piece? The toolbar (tabs on top, move + delete) appears on rollover -- so this has
  // to be TRUE the instant the cursor is on the box AND stay true as it travels up onto a tab. A
  // HoverHandler on a zone spanning the box + the strip above it does exactly that, with no dead gap.
  //
  // ⚠️ The first cut used one MouseArea behind the box for this and it never fired: the box's OWN
  // MouseArea (bodyArea) sits on top and consumes the hover, so the one behind it saw nothing and the
  // tabs never popped up. A HoverHandler is passive -- it reports the pointer regardless of the
  // MouseAreas over it -- which is the whole reason it works here. (@see hoverZone.)
  readonly property bool hovered: zoneHover.hovered || bodyArea.containsMouse

  // ── Drag state ─────────────────────────────────────────────────────────────────────────────
  //
  // Two kinds of drag: the whole GROUP (the ✥ handle), or ONE member out of it (a tab). Never both.
  property int groupDragX: -1
  property int groupDragY: -1
  readonly property bool groupDragging: stack.groupDragX >= 0

  property int memberDrag: -1        // index into members, or -1
  property int memberDragX: -1
  property int memberDragY: -1
  readonly property bool memberDragging: stack.memberDrag >= 0

  readonly property bool dragging: stack.groupDragging || stack.memberDragging

  // The box previews at the group's drag tile while the whole group is moving; otherwise it sits on
  // its real tile (a member leaving does not move the box).
  readonly property int liveX: stack.groupDragging ? stack.groupDragX : stack.tileX
  readonly property int liveY: stack.groupDragging ? stack.groupDragY : stack.tileY

  x: (stack.canvas.mapBorderPx + stack.liveX * 16) * stack.canvas.zoom
  y: (stack.canvas.mapBorderPx + stack.liveY * 16) * stack.canvas.zoom
  width: 16 * stack.canvas.zoom
  height: 16 * stack.canvas.zoom

  z: stack.dragging ? 31 : (stack.active ? 26 : 2)

  // The group's own colour -- deliberately NOT any single layer's ink, so a stack reads instantly as
  // "several things here" rather than as one warp or one sign. A bright violet, distinct from the
  // pink selection ring and from every Okabe-Ito layer colour.
  readonly property color groupColor: "#9d6bff"

  // ── Snap a global point to a clamped map tile ───────────────────────────────────────────────
  function tileFromGlobal(gx, gy) {
    const p = stack.canvas.tileAtGlobal(gx, gy);
    return Qt.point(Math.max(0, Math.min(brg.map.blocksWide * 2 - 1, p.x)),
                    Math.max(0, Math.min(brg.map.blocksHigh * 2 - 1, p.y)));
  }

  // ── The group box ───────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: box
    anchors.fill: parent

    color: Qt.rgba(stack.groupColor.r, stack.groupColor.g, stack.groupColor.b, 0.40)
    border.width: Math.max(1, Math.round(stack.canvas.zoom))
    border.color: stack.groupColor
    radius: Math.max(1, Math.round(2 * stack.canvas.zoom))
    opacity: stack.dragging ? 0.65 : 1.0

    // ✥ in the centre -- the cue that dragging the box moves the whole group, and the thing the
    // toolbar's move handle echoes. Hidden when the tile is too small to draw it.
    Text {
      anchors.centerIn: parent
      visible: stack.canvas.zoom >= 1.2
      text: "✥"
      font.pixelSize: Math.max(8, Math.round(9 * stack.canvas.zoom))
      color: "#ffffff"
    }

    // How many are here -- a small badge in the corner, always legible.
    Rectangle {
      anchors.top: parent.top
      anchors.right: parent.right
      anchors.margins: 1
      width: cnt.implicitWidth + 4
      height: cnt.implicitHeight + 2
      radius: 2
      color: "#cc212121"
      visible: stack.canvas.zoom >= 1

      Text {
        id: cnt
        anchors.centerIn: parent
        text: "×" + stack.members.length
        font.pixelSize: Math.max(7, Math.round(7 * stack.canvas.zoom))
        font.bold: true
        color: "#ffffff"
      }
    }
  }

  // A member on this tile points nowhere real -- the same amber "!" the lone chips wear.
  Text {
    visible: stack.canvas.zoom >= 1 && stack.members.some(function(m) { return !m.valid; })
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.margins: 1
    text: "!"
    font.bold: true
    font.pixelSize: Math.max(9, Math.round(10 * stack.canvas.zoom))
    color: "#d55e00"
  }

  // The selection ring, above everything, when one of the members is selected.
  Rectangle {
    visible: stack.active && !stack.dragging
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

  // ── The active member's label, BELOW the box ────────────────────────────────────────────────
  //
  // Where the active warp goes / what the active sign says -- the one thing you cannot read off the
  // map. Below, so it never fights the toolbar above. Shown on hover or when active.
  Rectangle {
    visible: (stack.active || stack.hovered) && !stack.dragging && lbl.text !== ""
    z: 40
    anchors.top: parent.bottom
    anchors.topMargin: 4
    anchors.horizontalCenter: parent.horizontalCenter

    width: lbl.implicitWidth + 12
    height: lbl.implicitHeight + 6
    radius: 3
    color: "#e6212121"

    Text {
      id: lbl
      anchors.centerIn: parent
      text: stack.activeMember && stack.activeMember.label ? stack.activeMember.label : ""
      font.pixelSize: 11
      color: (stack.activeMember && stack.activeMember.valid === false) ? "#ffb74d" : "white"
    }
  }

  // ── The toolbar, ABOVE the box ──────────────────────────────────────────────────────────────
  //
  // Screen-fixed sizes (never scaled by zoom): a toolbar you cannot read or hit at 0.5x is not a
  // toolbar. Revealed on hover or when active; a member drag or group drag hides it.
  Item {
    id: bar
    // Shown for editing -- so it yields entirely to the pan/zoom/place tools (an invisible Item takes
    // no mouse events, so hiding it also stops its buttons swallowing a zoom-click over the stack).
    visible: (stack.active || stack.hovered) && !stack.dragging
             && !stack.canvas.panning && stack.canvas.tool !== "zoom" && !stack.canvas.placing
    z: 50

    width: barRow.width
    height: barRow.height
    anchors.bottom: parent.top
    anchors.bottomMargin: 6
    anchors.horizontalCenter: parent.horizontalCenter

    Row {
      id: barRow
      spacing: 3

      // ── LEFT: one tab per member ────────────────────────────────────────────────────────────
      Repeater {
        model: stack.members

        delegate: Rectangle {
          id: tab
          required property var modelData
          required property int index

          width: 20; height: 20; radius: 4
          color: "#212121"
          border.width: tabArea.containsMouse || stack.activeIndex === tab.index ? 2 : 1
          border.color: stack.activeIndex === tab.index ? "#ffffff" : tab.modelData.border

          // Art for people; a glyph for warps and signs.
          Image {
            visible: tab.modelData.art !== ""
            anchors.fill: parent
            anchors.margins: 2
            source: tab.modelData.art
            smooth: false
            mipmap: false
            fillMode: Image.PreserveAspectFit
          }

          Text {
            visible: tab.modelData.art === ""
            anchors.centerIn: parent
            text: tab.modelData.glyph
            font.pixelSize: 12
            color: tab.modelData.border
          }

          MouseArea {
            id: tabArea
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            cursorShape: stack.memberDrag === tab.index ? Qt.ClosedHandCursor : Qt.PointingHandCursor

            property point press
            property bool moved: false

            onPressed: (m) => {
              tabArea.press = Qt.point(m.x, m.y);
              tabArea.moved = false;
              // Selecting a member is the same as clicking its chip: it selects, one thing at a time.
              stack.canvas.selectObject(tab.modelData.kind, tab.modelData.ind);
              m.accepted = true;
            }

            onPositionChanged: (m) => {
              if (!tabArea.pressed)
                return;
              if (!tabArea.moved
                  && Math.abs(m.x - tabArea.press.x) < 4 && Math.abs(m.y - tabArea.press.y) < 4)
                return;

              tabArea.moved = true;
              stack.memberDrag = tab.index;

              const g = tab.mapToGlobal(m.x, m.y);
              const t = stack.tileFromGlobal(g.x, g.y);
              stack.memberDragX = t.x;
              stack.memberDragY = t.y;

              // The ghost follows the cursor wherever it goes.
              const w = tab.mapToItem(ghost.parent, m.x, m.y);
              ghost.x = w.x - ghost.width / 2;
              ghost.y = w.y - ghost.height / 2;
            }

            onReleased: (m) => {
              if (!tabArea.moved) {
                // A plain CLICK on a tab opens that member's Details -- click, not drag.
                stack.canvas.editObject(tab.modelData.kind, tab.modelData.ind);
                return;
              }

              const nx = stack.memberDragX;
              const ny = stack.memberDragY;

              stack.memberDrag = -1;
              stack.memberDragX = -1;
              stack.memberDragY = -1;
              tabArea.moved = false;

              // Dropped where it already was: write nothing. Otherwise move it -- which unmerges it
              // from this stack. Exactly two bytes, through the canvas dispatch.
              if (nx >= 0 && (nx !== stack.tileX || ny !== stack.tileY))
                stack.canvas.moveObject(tab.modelData.kind, tab.modelData.ind, nx, ny);
            }

            onCanceled: {
              stack.memberDrag = -1;
              stack.memberDragX = -1;
              stack.memberDragY = -1;
              tabArea.moved = false;
            }
          }
        }
      }

      // ── CENTRE: move the whole group ─────────────────────────────────────────────────────────
      Rectangle {
        width: 20; height: 20; radius: 4
        color: moveArea.containsMouse ? "#56b4e9" : "#212121"
        border.width: 1
        border.color: "#ffffff"

        Text {
          anchors.centerIn: parent
          text: "✥"
          font.pixelSize: 12
          color: "white"
        }

        MouseArea {
          id: moveArea
          anchors.fill: parent
          hoverEnabled: true
          preventStealing: true
          cursorShape: stack.groupDragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor

          property point press
          property bool moved: false

          onPressed: (m) => {
            moveArea.press = Qt.point(m.x, m.y);
            moveArea.moved = false;
            m.accepted = true;
          }

          onPositionChanged: (m) => {
            if (!moveArea.pressed)
              return;
            if (!moveArea.moved
                && Math.abs(m.x - moveArea.press.x) < 4 && Math.abs(m.y - moveArea.press.y) < 4)
              return;

            moveArea.moved = true;

            const g = moveArea.mapToGlobal(m.x, m.y);
            const t = stack.tileFromGlobal(g.x, g.y);
            stack.groupDragX = t.x;
            stack.groupDragY = t.y;
          }

          onReleased: (m) => {
            if (!moveArea.moved)
              return;

            const nx = stack.groupDragX;
            const ny = stack.groupDragY;

            stack.groupDragX = -1;
            stack.groupDragY = -1;
            moveArea.moved = false;

            // Move every member to the new tile -- they stay stacked. Two bytes per member, and
            // nothing else. (moveGroup writes each through the same dispatch a lone drag uses.)
            if (nx >= 0 && (nx !== stack.tileX || ny !== stack.tileY))
              stack.canvas.moveGroup(stack.members, nx, ny);
          }

          onCanceled: {
            stack.groupDragX = -1;
            stack.groupDragY = -1;
            moveArea.moved = false;
          }
        }
      }

      // ── RIGHT: delete the whole group ────────────────────────────────────────────────────────
      Rectangle {
        width: 20; height: 20; radius: 4
        color: delArea.containsMouse ? "#d55e00" : "#212121"
        border.width: 1
        border.color: "#ffffff"

        Text {
          anchors.centerIn: parent
          text: "✕"
          font.pixelSize: 12
          color: "white"
        }

        MouseArea {
          id: delArea
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: (m) => {
            m.accepted = true;
            stack.canvas.removeGroup(stack.members);
          }
        }
      }
    }
  }

  // ── The member drag ghost ───────────────────────────────────────────────────────────────────
  //
  // Reparented to the window so it can follow the cursor anywhere, exactly like the lone chips.
  Rectangle {
    id: ghost

    parent: stack.Window.window ? stack.Window.window.contentItem : stack
    visible: stack.memberDragging
    z: 9999

    width: 26
    height: 26
    radius: 4
    color: stack.memberDrag >= 0
           ? Qt.rgba(0.13, 0.13, 0.13, 0.9)
           : "transparent"
    border.width: 1
    border.color: stack.memberDrag >= 0 ? stack.members[stack.memberDrag].border : "transparent"

    Image {
      visible: stack.memberDrag >= 0 && stack.members[stack.memberDrag].art !== ""
      anchors.fill: parent
      anchors.margins: 3
      source: stack.memberDrag >= 0 ? stack.members[stack.memberDrag].art : ""
      smooth: false
      mipmap: false
      fillMode: Image.PreserveAspectFit
    }

    Text {
      visible: stack.memberDrag >= 0 && stack.members[stack.memberDrag].art === ""
      anchors.centerIn: parent
      text: stack.memberDrag >= 0 ? stack.members[stack.memberDrag].glyph : ""
      font.pixelSize: 14
      color: stack.memberDrag >= 0 ? stack.members[stack.memberDrag].border : "white"
    }
  }

  // ── The hover zone ──────────────────────────────────────────────────────────────────────────
  //
  // An invisible region spanning the box AND the strip above it where the toolbar lives, carrying a
  // passive HoverHandler that drives `stack.hovered`. A HoverHandler (not a MouseArea) is the point:
  // it reports the pointer even when the box's or a tab's MouseArea is under the cursor, so the
  // toolbar appears the instant you roll onto the piece and stays up as you slide onto a tab across
  // the gap. It consumes nothing -- clicks fall straight through to the buttons and the box body.
  Item {
    id: hoverZone
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    width: Math.max(parent.width, barRow.width + 8)
    height: parent.height + bar.anchors.bottomMargin + barRow.height + 4

    HoverHandler { id: zoneHover }
  }

  // ── The box body: click to select+edit the active member, drag to move the group ────────────
  //
  // The big, easy target. A press selects (the active member, or the top one if none is), a plain
  // click opens its Details, and a drag moves the whole group -- the same three gestures a lone chip
  // has, with "move" meaning the group. The ✥ toolbar handle is the same action for when the box is
  // small.
  MouseArea {
    id: bodyArea
    anchors.fill: parent

    enabled: !stack.canvas.panning && stack.canvas.tool !== "zoom" && !stack.canvas.placing
    hoverEnabled: true
    preventStealing: true
    cursorShape: stack.groupDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

    property point press
    property bool moved: false

    onPressed: (m) => {
      bodyArea.press = Qt.point(m.x, m.y);
      bodyArea.moved = false;
      // Select the active member, or the top one if nothing here is selected yet.
      const mem = stack.activeMember;
      if (mem)
        stack.canvas.selectObject(mem.kind, mem.ind);
      m.accepted = true;
    }

    onPositionChanged: (m) => {
      if (!bodyArea.pressed)
        return;
      if (!bodyArea.moved
          && Math.abs(m.x - bodyArea.press.x) < 4 && Math.abs(m.y - bodyArea.press.y) < 4)
        return;

      bodyArea.moved = true;

      const g = bodyArea.mapToGlobal(m.x, m.y);
      const t = stack.tileFromGlobal(g.x, g.y);
      stack.groupDragX = t.x;
      stack.groupDragY = t.y;
    }

    onReleased: (m) => {
      if (!bodyArea.moved) {
        // A plain CLICK opens the active member's Details; a drag does not.
        const mem = stack.activeMember;
        if (mem)
          stack.canvas.editObject(mem.kind, mem.ind);
        return;
      }

      const nx = stack.groupDragX;
      const ny = stack.groupDragY;

      stack.groupDragX = -1;
      stack.groupDragY = -1;
      bodyArea.moved = false;

      if (nx >= 0 && (nx !== stack.tileX || ny !== stack.tileY))
        stack.canvas.moveGroup(stack.members, nx, ny);
    }

    onCanceled: {
      stack.groupDragX = -1;
      stack.groupDragY = -1;
      bodyArea.moved = false;
    }
  }

  // Esc, mid-drag: put everything back, write NOTHING.
  Connections {
    target: stack.canvas
    function onCancelDragChanged() {
      stack.groupDragX = -1;
      stack.groupDragY = -1;
      stack.memberDrag = -1;
      stack.memberDragX = -1;
      stack.memberDragY = -1;
    }
  }
}
