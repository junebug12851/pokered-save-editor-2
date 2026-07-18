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
// MultiEffect -- for the silhouette outline (colorization flattens a copy of the artwork to one
// colour while keeping its alpha, which is what lets the outline hug the character's real shape
// without us ever knowing it). @see the outline Repeater.
import QtQuick.Effects

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

  /// The sub-tile SLIDE, in buffer pixels, while a step is in progress.
  ///
  /// ⚠️ Not decoration -- without it sprites TELEPORT. `TryWalking` moves the tile coordinate to the
  /// destination immediately and then slides the sprite one pixel a frame for sixteen frames, so the
  /// tile is where they are *going*. This is the difference. (@see MapModel::npcList)
  property int offX: 0
  property int offY: 0

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

  // Dragging the PLAYER moves the screen box and the draw area with him -- both are computed FROM
  // his position, and watching them snap into place only on release was the tell that they weren't
  // really his. (Twilight, 2026-07-13.) -1 hands them back to where he actually is.
  onLiveXChanged: if (sprite.isPlayer) sprite.canvas.livePlayerX = sprite.dragging ? sprite.liveX : -1
  onLiveYChanged: if (sprite.isPlayer) sprite.canvas.livePlayerY = sprite.dragging ? sprite.liveY : -1

  onDraggingChanged: {
    if (!sprite.isPlayer || sprite.dragging)
      return;

    sprite.canvas.livePlayerX = -1;
    sprite.canvas.livePlayerY = -1;
  }

  readonly property bool selected: sprite.canvas.selectedNpc === sprite.slot

  // The 4-pixel lift -- where the console's own OAM puts a sprite -- plus the sub-tile slide, which
  // is what makes a step a step instead of a jump. (While you are DRAGGING there is no slide: the
  // preview belongs under your cursor, not sixteen pixels behind it.)
  readonly property int slideX: sprite.dragging ? 0 : sprite.offX
  readonly property int slideY: sprite.dragging ? 0 : sprite.offY

  x: (sprite.canvas.mapBorderPx + sprite.liveX * 16 + sprite.slideX) * sprite.canvas.zoom
  y: (sprite.canvas.mapBorderPx + sprite.liveY * 16 + sprite.slideY - 4) * sprite.canvas.zoom
  width: 16 * sprite.canvas.zoom
  height: 16 * sprite.canvas.zoom

  z: sprite.dragging ? 30 : (sprite.selected ? 25 : 0)

  // (Object stacking was removed 2026-07-15; a sprite always draws, overlapping or not. The player
  //  instance still overrides `visible` at its use-site to gate on showPlayer.)
  visible: true

  /// The silhouette's ink -- the layer's own colour, so the outline, its tab and the Layers panel
  /// row are visibly one thing: the Player his own blue, everybody else People & objects pink.
  /// SELECTION lightens it rather than adding a second box round the first.
  /// AMBER overrides both when the map has not loaded this sprite's picture -- the one thing you
  /// cannot see by looking, because the console would draw garbage there.
  readonly property color outlineColor:
      sprite.selected                     ? "#ffffff"
    : (!sprite.inSet && !sprite.isPlayer) ? "#ffd54f"
    : sprite.isPlayer                     ? "#0072b2"
                                          : "#cc79a7"

  // ⭐ THE OUTLINE HUGS THE CHARACTER, not the tile he stands on.
  //
  // > *"dont put a box around and highlight the transparent areas of sprites and only sprites
  // >  because they are the only ones to have transparent areas"* … *"it would tremendously add to
  // >  the ui/ux"* -- Fairy Fox, 2026-07-17
  //
  // And she is right that sprites are the ONLY case: a warp, a sign, a script trigger, a buried item
  // are all abstract 16x16 cells with nothing to see through, so a rectangle IS their shape. A
  // sprite is artwork on a transparent field -- the console's OAM makes colour 0 transparent
  // always (reference/sprites.md) -- so a box round a person is mostly a box round nothing, and it
  // reads as "this square" instead of "this fellow".
  //
  // HOW: the classic sticker trick, and it needs no shader and no alpha mask of our own. Draw the
  // sprite's own artwork four times, one pixel out in each direction, flattened to the outline
  // colour, BEHIND the real one. Every opaque pixel paints its neighbours; every transparent pixel
  // paints nothing. What is left showing round the edges is exactly the silhouette.
  //
  // `MultiEffect.colorization: 1` flattens each copy to a single colour while KEEPING its alpha --
  // which is the whole reason this works on artwork whose shape we never have to know.
  Repeater {
    model: [Qt.point(-1, 0), Qt.point(1, 0), Qt.point(0, -1), Qt.point(0, 1)]

    delegate: MultiEffect {
      required property var modelData

      // 2px at 1x, and it scales with the zoom so the line does not thin out as you zoom in --
      // matching MapBlockHotspot.lineWidth's weight, since it is the same language.
      readonly property real w: Math.max(1.5, 1.5 * sprite.canvas.zoom)

      x: modelData.x * w
      y: modelData.y * w
      width: sprite.width
      height: sprite.height
      z: -1                       // behind the character

      source: outlineArt
      colorization: 1.0
      colorizationColor: sprite.outlineColor
      opacity: sprite.dragging ? 0.6 : 0.95
    }
  }

  /// The artwork the outline is stamped from. Invisible itself -- it exists to be sampled by the
  /// four copies above. (`layer.enabled` makes it a texture they can source.)
  Image {
    id: outlineArt
    anchors.fill: parent
    source: sprite.art
    visible: false
    layer.enabled: true
    smooth: false
    mipmap: false
    fillMode: Image.PreserveAspectFit
  }

  PixelImage {
    anchors.fill: parent
    source: sprite.art
    opacity: sprite.dragging ? 0.6 : 1.0
  }

  // ⭐ EVERY SPRITE WEARS ITS BOX, ALWAYS -- solid outline + a fill.
  //
  // > *"some sprites have no box, some do"* … *"the player doesnt have a box yet its editable too"*
  // > -- Fairy Fox, twice.
  //
  // Both were one fault: the box appeared only when a sprite was SELECTED, so the map showed
  // outlines on an apparently arbitrary subset and none at all on the player. A person, an item ball
  // and the player are all things you can drag, delete and place; **every one of them is outlined,
  // always**, and "only once you've already found it" is not a rule, it is the absence of one.
  //
  // ⚠️ NO FILL, and no rectangle either -- the outline IS the silhouette (@see the Repeater above).
  // The language rides entirely on the LINE: **solid = you can pick this up**, dashed = it lives
  // where the cartridge put it. A wash over a sprite repaints the character it is describing.

  // (No selection RECTANGLE either. Selecting a sprite turns its own silhouette WHITE -- @see
  //  outlineColor. A second box drawn round the first would be two marks for one state, and would
  //  put back the very rectangle-round-transparent-air this outline exists to remove.)

  // ── The delete button ────────────────────────────────────────────────────────────────────
  //
  // On the selected sprite, ABOVE it -- a delete button drawn over the character you are trying to
  // look at is worse than no button. The player has no ✕: the game requires him. No ✎ any more --
  // a plain CLICK opens the Details panel (see onReleased).
  Row {
    visible: sprite.selected && !sprite.dragging && !sprite.isPlayer
    z: 40
    anchors.bottom: parent.top
    anchors.bottomMargin: 3
    anchors.horizontalCenter: parent.horizontalCenter
    spacing: 3

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

  // ── The ghost ────────────────────────────────────────────────────────────────────────────
  //
  // It only appears once you have dragged OFF the map -- over the Characters panel, where dropping
  // means delete. On the map itself the sprite previews in place, which reads better.
  Image {
    id: ghost

    parent: sprite.Window.window ? sprite.Window.window.contentItem : sprite
    visible: sprite.dragging && sprite.overBin
    z: 9999

    width: 32
    height: 32
    source: sprite.art
    smooth: false
    mipmap: false
    fillMode: Image.PreserveAspectFit
    opacity: 0.9
  }

  /// True while the cursor is over the delete zone mid-drag. @see MapCanvas.overDeleteZone
  property bool overBin: false

  // ── Input ────────────────────────────────────────────────────────────────────────────────
  MouseArea {
    id: area
    anchors.fill: parent

    enabled: !sprite.canvas.panning && sprite.canvas.tool !== "zoom"
    hoverEnabled: true
    cursorShape: sprite.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

    // ⭐ Tell the canvas a MOVABLE thing is under the pointer, so the cell hands the highlight over
    // and withdraws its tabs: *"the highlight changes to the moveable object and away from the cell
    // to indicate its moveable, also the cell tabs go away when mousing over a moveable item"*.
    // Point at the thing → drag the thing. Point at the cell → get the cell's tabs.
    // A sprite is a 16x16 half-block inside a 32x32 cell, so it never fills it -- there is always
    // cell left to point at, and `fullCell` stays false.
    onContainsMouseChanged: {
      const key = Math.floor((sprite.canvas.mapBorderPx + sprite.liveX * 16) / 32) + ","
                + Math.floor((sprite.canvas.mapBorderPx + sprite.liveY * 16) / 32);
      if (area.containsMouse) {
        sprite.canvas.hoverMovable = key;
        sprite.canvas.hoverMovableFullCell = false;
      } else if (sprite.canvas.hoverMovable === key) {
        sprite.canvas.hoverMovable = "";
      }
    }

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

      // A tiny wobble on a click is not a drag.
      if (!area.moved
          && Math.abs(m.x - area.press.x) < 4 && Math.abs(m.y - area.press.y) < 4)
        return;

      area.moved = true;

      // Over the Characters panel? Then this is a delete-in-progress, not a move.
      const g = sprite.mapToGlobal(m.x, m.y);
      sprite.overBin = !sprite.isPlayer && sprite.canvas.overDeleteZone(g.x, g.y);
      sprite.canvas.deleteHover = sprite.overBin;

      if (sprite.overBin) {
        const w = sprite.mapToItem(ghost.parent, m.x, m.y);
        ghost.x = w.x - ghost.width / 2;
        ghost.y = w.y - ghost.height / 2;

        sprite.dragX = sprite.tileX;   // keep the preview where it was; it is on its way to the bin
        sprite.dragY = sprite.tileY;
        return;
      }

      // ⚠️ TILE-SNAPPED FROM THE CURSOR, not from an accumulated delta.
      //
      // The old version rounded (cursor − press) into tile steps, which drifts: the sprite's own
      // coordinate moves under you while `press` stays put, so the two disagree by a tile and the
      // thing skitters. (Twilight: "moving main character around is very glitchy.") Ask where the
      // cursor IS, in map tiles, and put the sprite there. There is nothing to drift.
      const p = sprite.canvas.tileAtGlobal(g.x, g.y);

      sprite.dragX = Math.max(0, Math.min(brg.map.blocksWide * 2 - 1, p.x));
      sprite.dragY = Math.max(0, Math.min(brg.map.blocksHigh * 2 - 1, p.y));
    }

    onReleased: (m) => {
      if (!sprite.dragging && !area.moved) {
        // A plain CLICK opens the Details panel on this sprite (the player included); a drag does
        // not. (Twilight: "details should only open on a click not click and drag".)
        sprite.editRequested();
        return;
      }

      const nx = sprite.dragX;
      const ny = sprite.dragY;
      const bin = sprite.overBin;

      sprite.dragX = -1;
      sprite.dragY = -1;
      sprite.overBin = false;
      sprite.canvas.deleteHover = false;
      area.moved = false;

      // Dropped on the Characters panel: that is a DELETE.
      if (bin && !sprite.isPlayer) {
        brg.map.removeNpc(sprite.slot);
        sprite.canvas.selectedNpc = -1;
        sprite.canvas.status = qsTr("Removed. The sprites after it slid up a slot.");
        return;
      }

      if (nx < 0 || (nx === sprite.tileX && ny === sprite.tileY))
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
      sprite.overBin = false;
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
