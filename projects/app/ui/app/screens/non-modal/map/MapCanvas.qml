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
  MapCanvas.qml -- the map itself, floating in a dark well.

  Every number here comes from brg.map (MapModel) in BUFFER PIXELS -- one screen pixel per Game Boy
  pixel, 32 to a block, origin at the top-left of the 3-block border ring -- and QML's only job is to
  multiply by an integer `zoom`. If a rectangle looks wrong, the bug is in C++ (MapEngine), not here.
  Keep it that way.

  What is drawn, outermost first:
    * the WELL              -- a dark neutral surround. Every pixel editor does this, and for the
                               same reason: Game Boy art is four shades of grey, and a white page
                               behind it drowns the light end. The map floats on it with a shadow.
    * the rendered image    -- the map PLUS its border ring (the game keeps the map inside that ring
                               so connected maps can bleed in)
    * the overlay image     -- the meaning layers, rendered by MapEngine as ONE transparent image
    * the block grid        -- the 32px cells a map is really made of
    * the map bounds        -- where the real map ends and the ring begins
    * the draw area (6x5 blocks) -- what LoadCurrentMapView redraws (wSurroundingTiles)
    * the player            -- where the console's own OAM puts him (4px above his tile row)
    * the screen (20x18 tiles)   -- what the player actually SEES, sliding around inside the draw
                               area in half-block steps

  (In phase 2 those last five stop being hard-coded rectangles and become LAYERS you can switch off.
  Today they are exactly what they were, just re-homed. -- notes/plans/map-screen.md)

  ⚠️ The root id is `canvasRoot`, NOT `top`: this file has Repeaters, and a Repeater delegate cannot
  see the file's root id -- `top.zoom` inside one comes back `undefined`, x goes NaN, and every grid
  line silently collapses onto x = 0 with no warning and a green tst_qml_screens. Inside a delegate,
  reach values through a plain sibling id (`canvas`). See reference/qt-patterns.md.
*/
import QtQuick
import QtQuick.Controls

Item {
  id: canvasRoot

  /// The active tool, from the rail: "select" | "pan" | "zoom".
  property string tool: "select"

  /// The sprite SLOT currently selected (1-15), or -1 for nothing. The ground is deliberately not
  /// clickable -- blocks and tiles are edited in their own panels.
  property int selectedNpc: -1

  /// The DOOR currently selected (0-31), or -1 for nothing. @see MapWarp.qml
  ///
  /// ⚠️ **One selection at a time.** Selecting a door clears the sprite and vice versa: there is one
  /// Details panel, it edits one thing, and a UI that claims two things are selected while only
  /// editing one of them is lying.
  property int selectedWarp: -1

  /// The SIGN currently selected (0-15), or -1 for nothing. @see MapSign.qml -- the doors' sibling,
  /// on the same one-selection-at-a-time rule.
  property int selectedSign: -1

  /// The edge CONNECTION currently selected -- its direction (0-3), or -1 for nothing. @see
  /// MapConnection.qml. Same one-selection-at-a-time rule as the doors and signs.
  property int selectedConnection: -1

  onSelectedNpcChanged: if (canvasRoot.selectedNpc >= 0) { canvasRoot.selectedWarp = -1; canvasRoot.selectedSign = -1; canvasRoot.selectedConnection = -1; }
  onSelectedWarpChanged: if (canvasRoot.selectedWarp >= 0) { canvasRoot.selectedNpc = -1; canvasRoot.selectedSign = -1; canvasRoot.selectedConnection = -1; }
  onSelectedSignChanged: if (canvasRoot.selectedSign >= 0) { canvasRoot.selectedNpc = -1; canvasRoot.selectedWarp = -1; canvasRoot.selectedConnection = -1; }
  onSelectedConnectionChanged: if (canvasRoot.selectedConnection >= 0) { canvasRoot.selectedNpc = -1; canvasRoot.selectedWarp = -1; canvasRoot.selectedSign = -1; }

  /// The ✎ button on a selected sprite -- the Map screen opens the Details panel on it.
  signal editRequested(int slot)

  /// A storage spot was reached: open Map Storage at @p section, on row @p ind.
  ///
  /// `section` is the panel section the spot lives in ("missable" / "event" / "script" / "hidden"),
  /// so one signal serves every storage kind -- adding the next kind means adding a spot type, not
  /// another signal. @see MapBlockHotspot.qml, MapModel::blockHotspots
  signal storageRequested(string section, int ind)

  // ── The MAKER TOOLS ───────────────────────────────────────────────────────────────────────
  //
  // Twilight, 2026-07-14: *"we would need the top toolbar to ironically contain actual tools and
  // this is one of them, a create random sprite here tool, and a create warp here tool."*
  //
  // A tool that MAKES something is a different animal from one that selects: it needs a cursor that
  // says so, a click that lands on the ground rather than on whatever is under it, and a cap stated
  // BEFORE you hit it rather than a click that silently does nothing.

  /// True while a maker tool is in hand ("placeWarp" | "placeSprite" | "placeSign").
  readonly property bool placing: canvasRoot.tool === "placeWarp" || canvasRoot.tool === "placeSprite"
                               || canvasRoot.tool === "placeSign"

  /// How many more of the thing the tool makes this map can hold. 0 -> the tool is dead, and the
  /// context bar says so instead of letting you click into nothing.
  readonly property int placeRoomLeft: canvasRoot.tool === "placeWarp"   ? brg.map.warpRoomLeft()
                                     : canvasRoot.tool === "placeSprite" ? brg.map.npcRoomLeft()
                                     : canvasRoot.tool === "placeSign"   ? brg.map.signRoomLeft()
                                     : 0

  /// A new thing landed. The status bar says what and where.
  signal placed(string kind, int index)

  /// A line for the status bar (a drop, a delete, a cap hit). Never a modal.
  property string status: ""

  /// How many popups are currently open over the canvas. While > 0 the ground does not take clicks.
  property int popupsOpen: 0

  /// The panels that FLOAT OVER this canvas (the two docks). A tap that lands inside an open one is
  /// not a tap on the ground, and the ground must not act on it.
  ///
  /// ⚠️ THIS IS NOT BELT-AND-BRACES. It is the fix, and here is the mechanism, because it is not
  /// obvious and it cost two rounds:
  ///
  /// Qt delivers a press to **every pointer handler on every item under the point, front to back,
  /// BEFORE any item gets a mouse event.** A `TapHandler` whose `gesturePolicy` is the default
  /// `DragThreshold` **does not take an exclusive grab** -- so it does not stop that walk. The
  /// picture picker's chip is a TapHandler in a panel that floats over the map… and the map's own
  /// ground TapHandler is *also* under the point. Both fired. The picker opened and the ground
  /// cleared the selection, so the Details panel fell straight back to showing the map.
  ///
  /// The dock's swallowing MouseArea does not help: a MouseArea is an ITEM, and the item pass
  /// happens *after* the handler pass. By then the damage is done.
  ///
  /// So the ground asks, in as many words, whether the tap was over a panel. It is a plain geometric
  /// containment test -- the same shape as `overDeleteZone`, which we already trust -- and no
  /// handler's grab policy can defeat it.
  property var overlays: []

  /// Is this global point inside one of the panels floating over us?
  ///
  /// ⚠️ It asks the DOCK, because the dock item is only the 40px rail -- the open panel hangs outside
  /// its bounds. Testing `dock.width` tests the rail. @see MapDock.panelContainsGlobal
  function overPanel(sx, sy) {
    for (let i = 0; i < canvasRoot.overlays.length; i++) {
      const dock = canvasRoot.overlays[i];

      if (dock && dock.panelContainsGlobal(sx, sy))
        return true;
    }

    return false;
  }

  /// The 3-block border ring, in buffer pixels. A sprite at map (0,0) starts here.
  readonly property int mapBorderPx: 3 * 32

  // ── DROPPING, done explicitly ──────────────────────────────────────────────────────────────
  //
  // ⚠️ Qt's `Drag` / `DropArea` is GONE from this screen, and it is not coming back.
  //
  // It decides what you are over by intersecting the **dragged item's geometry** with the drop
  // area's -- so it needs the drag source to physically be over the target, which meant reparenting
  // ghosts into the window, and it broke twice in ways that were invisible until Twilight tried it.
  // Two rounds of "dragging and dropping doesn't work" is enough.
  //
  // So the drop is a FUNCTION CALL with a scene coordinate. The dragger asks "is this point over
  // you?", and the answer is a plain geometric containment test we can read, reason about and test.
  // Nothing hidden, nothing implicit.

  /// Where a sprite dragged OFF the map gets deleted -- the left dock, handed over by the screen.
  property var deleteZone: null

  /// True while a map sprite is being dragged over the delete zone. The Characters panel reads this
  /// to light itself up red; it has no DropArea of its own any more.
  property bool deleteHover: false

  /// Is this global point over the delete zone (the Characters panel, while it is open)?
  ///
  /// ⚠️ It asks the DOCK for its PANEL's bounds. It used to test `deleteZone.width` -- and the dock
  /// item is only the 40px rail, so it was testing the icon strip. Dragging somebody out to delete
  /// them only worked if you dropped them on the rail. @see MapDock.panelContainsGlobal
  function overDeleteZone(sx, sy) {
    if (!canvasRoot.deleteZone || canvasRoot.deleteZone.open !== "characters")
      return false;

    return canvasRoot.deleteZone.panelContainsGlobal(sx, sy);
  }

  /// The MAP TILE under a global point. The one place that conversion is written down.
  ///
  /// ⚠️ Dragging a sprite asks *where the cursor is* and puts the sprite there. It does NOT
  /// accumulate a delta from the press point -- that drifts, because the sprite's own coordinate
  /// moves under you while the press point stays put, and after a tile or two the two disagree and
  /// the sprite skitters. (Twilight: *"moving main character around is very glitchy."*)
  function tileAtGlobal(sx, sy) {
    const p = canvas.mapFromGlobal(sx, sy);

    return Qt.point(Math.floor((p.x / canvasRoot.zoom - canvasRoot.mapBorderPx) / 16),
                    Math.floor((p.y / canvasRoot.zoom - canvasRoot.mapBorderPx) / 16));
  }

  /// Drop a character from the Characters panel onto the map. @p sx/@p sy are GLOBAL coordinates.
  /// @return true if it landed.
  function dropCharacter(sx, sy, picture) {
    if (picture <= 0)
      return false;

    // Global -> the canvas item's own coordinates. `canvas` is the scaled map; its origin is the
    // top-left of the border ring.
    const p = canvas.mapFromGlobal(sx, sy);

    if (p.x < 0 || p.y < 0 || p.x >= canvas.width || p.y >= canvas.height)
      return false;   // not over the map at all -- the drag is simply abandoned

    if (brg.map.npcRoomLeft() <= 0) {
      canvasRoot.status = qsTr("This map is full — 15 people and objects is the most the Game Boy can hold.");
      return false;
    }

    const t = canvasRoot.tileAtGlobal(sx, sy);
    const slot = brg.map.addNpc(picture, t.x, t.y);
    if (slot <= 0)
      return false;

    canvasRoot.selectedNpc = slot;
    canvasRoot.status = qsTr("Placed in slot %1.").arg(slot);
    return true;
  }

  // ⚠️ THE REASON THE CHARACTERS DID NOT MOVE.
  //
  // `brg.map.npcList()` is a FUNCTION CALL. A QML binding on a function call only re-evaluates when
  // one of the *properties it happens to read* changes -- and `npcList()` reads none, because it is
  // C++. So the Repeater below bound to it ONCE, at load, and never asked again: you could drag a
  // sprite, let the simulation run, delete somebody -- the model changed underneath and the canvas
  // went on drawing the cast it had memorised at startup.
  //
  // `revision` is the dependency the binding was missing. Bump it on `changed()` and every list on
  // this screen re-asks. (Exactly the bug that made the Details panel come up blank -- same shape,
  // different symptom. If you are binding to a C++ *method*, you need one of these.)
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { canvasRoot.revision++; }

    // ⚠️ The CAST moving gets its own signal, and it must NOT be `changed()`.
    //
    // `changed()` re-emits `sourceChanged()`, and `source` is the map's render URL -- so it
    // re-renders the entire map image. The walk simulation moves somebody ~60 times a second, so
    // routing it through `changed()` re-rendered the whole map 60 times a second to shift one 16x16
    // sprite, and the frame rate collapsed. `castChanged()` bumps the sprite list and nothing else.
    // (@see MapModel::castChanged)
    function onCastChanged() { canvasRoot.revision++; }

    // The DOORS get their own signal for exactly the same reason -- dragging a door must not
    // re-render the whole map image. (@see MapModel::warpsChanged)
    function onWarpsChanged() { canvasRoot.revision++; }

    // The SIGNS, same story. (@see MapModel::signsChanged)
    function onSignsChanged() { canvasRoot.revision++; }
  }

  readonly property var npcs: {
    canvasRoot.revision;   // a dependency, deliberately
    return brg.mapLayers.showNpcs ? brg.map.npcList() : [];
  }

  readonly property var warps: {
    canvasRoot.revision;   // the same missing dependency, and it bites the same way
    return brg.mapLayers.showWarps ? brg.map.warpList() : [];
  }

  readonly property var signs: {
    canvasRoot.revision;   // the same missing dependency, the same fix
    return brg.mapLayers.showSigns ? brg.map.signList() : [];
  }

  /// The FLAG BOXES — the ROM's cast, filtered to the objects the save keeps a flag for.
  ///
  /// ⚠️ Unlike its neighbours above, this list does NOT depend on `revision`, and must not: it is
  /// derived from the cartridge, so nothing the user edits can change it. What DOES change is each
  /// box's `hidden` — a live WorldMissables bit, read per-delegate below so flipping a switch redraws
  /// one box instead of rebuilding the list. @see notes/plans/map-screen.md -> Phase 16
  /// The blocks that have persistent storage on them, each owning the list of spots that land there.
  ///
  /// The tile-trait family is passed the Tiles overlays that are actually SHOWN: they are the 8x8
  /// tabs, and walls are on nearly every block, so an ungated strip would drown the storage tabs it
  /// exists for. @see MapModel::blockHotspots
  readonly property var flagBoxes: brg.mapLayers.showFlagBoxes
                                     ? brg.map.blockHotspots(brg.map.layers)
                                     : []

  /// Which storage kinds the Layers panel is currently showing -- what the tab strip is allowed to
  /// tab. *"the tab strip only needs to have tabs based on the active layers"* (Fairy Fox).
  ///
  /// ⚠️ OPEN, for Fairy Fox: the four storage kinds below all ride the ONE **Flag boxes** layer,
  /// because that is the only row the tree has for them today. They are different things
  /// (an object's filter flag · a hidden pickup · a script trigger · an event flag) and each
  /// probably wants its own row, so they can be turned on and off apart -- but the layer tree is a
  /// design of record, so this does not invent four rows on its own. When the rows exist, only this
  /// list changes. `tileTrait` is already properly gated: its own Tiles overlays decide it, via
  /// `blockHotspots(brg.map.layers)`.
  readonly property var activeStorageKinds: {
    const k = [];
    if (brg.mapLayers.showFlagBoxes)
      k.push("filterFlag", "hiddenItem", "hiddenCoin", "script", "cardKeyDoor", "eventFlag");
    // The tile family is filtered in the model by the Tiles overlays, so anything that survived to
    // here is on by definition.
    k.push("tileTrait");
    return k;
  }

  /// The save's missable bits, for the boxes' dashes. Null until a file is open.
  readonly property var wMissables: {
    if (!brg.file || !brg.file.data || !brg.file.data.dataExpanded || !brg.file.data.dataExpanded.world)
      return null;
    return brg.file.data.dataExpanded.world.missables;
  }

  // The four edge connections, in two views: the STRIP geometry (where each lands in the ring) and the
  // EDIT info (offset, sync, the snap landmarks). Both keyed off `revision` so an offset edit updates
  // the strip in place. @see MapConnection.qml (which is NOT a Repeater delegate, on purpose).
  readonly property var connStrips: {
    canvasRoot.revision;
    return brg.mapLayers.showConnections ? brg.map.connectionList() : [];
  }
  readonly property var connEdges: {
    canvasRoot.revision;
    return brg.map.connectionEditList();
  }

  /// The strip geometry for direction @p dir, or null. @see connStrips.
  function connStripFor(dir) {
    const l = canvasRoot.connStrips;
    for (let i = 0; i < l.length; i++)
      if (l[i].dir === dir) return l[i];
    return null;
  }
  /// The edit info for direction @p dir, or null. @see connEdges.
  function connEdgeFor(dir) {
    const l = canvasRoot.connEdges;
    for (let i = 0; i < l.length; i++)
      if (l[i].dir === dir) return l[i];
    return null;
  }

  // (The "universal object stacking" feature — a group box for objects sharing a tile — was REMOVED
  //  2026-07-15 at Twilight's request: *"it never worked well and there's no point fixing it because I
  //  only added it from a misunderstanding."* Overlapping objects now simply draw over each other, each
  //  its own selectable/draggable chip, which is the ordinary behaviour and the one that works.)

  // ── The two boxes follow the player LIVE ───────────────────────────────────────────────────
  //
  // The screen box and the draw area are both computed FROM the player's position -- so while you
  // are dragging him they should move with him, not snap into place when you let go (Twilight,
  // 2026-07-13). The player's MapSprite publishes the tile under the cursor here; -1 means "not
  // being dragged", and the boxes fall back to where he actually is.
  property int livePlayerX: -1
  property int livePlayerY: -1

  readonly property var boxes: {
    canvasRoot.revision;   // ...and re-ask when he actually moves, too

    const x = canvasRoot.livePlayerX >= 0 ? canvasRoot.livePlayerX : brg.map.playerX;
    const y = canvasRoot.livePlayerY >= 0 ? canvasRoot.livePlayerY : brg.map.playerY;

    return brg.map.viewBoxesAt(x, y);
  }

  // ── Zoom ────────────────────────────────────────────────────────────────────────────────────
  //
  // CONTINUOUS, not integer (Twilight, 2026-07-13: "it's too clunky"). It used to snap to whole
  // multiples because a fractional scale ruins pixel art -- at 2.37x, nearest-neighbour gives some
  // Game Boy pixels two screen pixels and others three, and the map ripples as you zoom.
  //
  // That is now solved properly, in a shader (PixelImage.qml -> shaders/pixelart.frag): anti-aliased
  // point sampling, flat inside a pixel and one screen pixel of blend across the seam. Crisp like
  // nearest, smooth like bilinear, and pixel-identical to the old behaviour at whole zooms. So the
  // zoom can be any real number, and it is.
  // ── The range ────────────────────────────────────────────────────────────────────────────────
  //
  // Effectively "infinite" both ways (Twilight, 2026-07-14), but deliberately BOUNDED, because the
  // brief in the same breath was *"there doesn't need to be any lag or crashing or fragility... at
  // all on the infinite zoom — prioritise the UX."* A hard, generous clamp is the stable choice:
  //   * `minZoom` 0.05 -- the map shrinks into the well until a 78-block route is a thumbnail;
  //   * `maxZoom` 64   -- a single Game Boy pixel fills a large block of screen.
  // Wider than this buys nothing a person actually does and only invites the ripple/precision
  // problems she told us to avoid. The FEEL stays gentle everywhere because every step is a constant
  // RATIO (see the wheel/click handlers), never a fixed pixel jump -- so it is never twitchy zoomed
  // in nor sluggish zoomed out.
  readonly property real minZoom: 0.05
  readonly property real maxZoom: 64

  // ⚠️ TRUE SUB-PIXEL ZOOM -- and only ONE input needs any interpolation at all.
  //
  // The zoom is a real number. Nothing snaps, nothing quantises: at 2.3718x the map really is at
  // 2.3718x, and the pixel-art shader samples it correctly (PixelImage.qml). That is the smooth
  // zoom, and it is not an animation.
  //
  // The catch is the MOUSE WHEEL. A wheel reports 120 units per detent and there is no such thing as
  // half a click -- the hardware simply does not have sub-notch data to give us. So a wheel notch is
  // ALWAYS a jump, and the only choices are to make the jump smaller (which is what the first attempt
  // did, and which Twilight correctly called "still choppy, just finer steps") or to bridge it.
  //
  // So:
  //
  //   * **Continuous input** -- the slider, a trackpad's pixelDelta, a pinch -- goes STRAIGHT
  //     THROUGH, with **no animation whatsoever**. It is already sub-pixel; interpolating it would
  //     only add lag.
  //   * **A wheel notch** gets a 90ms bridge, and nothing else does. That is the "between pixel
  //     sizes, frame to frame" case Twilight allowed -- it exists purely because the hardware left
  //     a gap, not to make anything look fancy.
  //
  // `anchorMap` is the map point that stays under the cursor. It is re-pinned on EVERY frame the
  // zoom changes (see `onZoomChanged`), so it holds during a bridge as well as during a drag.

  /// 0 = "nobody has said otherwise, use the default view".
  property real userZoom: 0

  /// Where the map really is. Follows `userZoom` exactly -- except across a wheel notch.
  property real zoom: userZoom > 0 ? userZoom : defaultZoom

  /// True ONLY while bridging a wheel detent. @see the note above.
  property bool bridging: false

  Behavior on zoom {
    enabled: canvasRoot.bridging

    NumberAnimation {
      duration: 90
      easing.type: Easing.OutQuad
      onFinished: canvasRoot.bridging = false
    }
  }

  /// The map point (buffer px) to keep under `anchorView` while the zoom eases. Null = keep the
  /// middle of the view.
  property var anchorMap: null
  property point anchorView: Qt.point(0, 0)

  onZoomChanged: {
    if (!canvasRoot.anchorMap || view.width <= 0)
      return;

    // Re-pin, every frame of the glide. The content geometry is a binding on `zoom`, so by the time
    // we are called it has already moved -- which is exactly what we want to correct against.
    view.contentX = Math.max(0, Math.min(Math.max(0, view.contentWidth - view.width),
                                         canvas.x + canvasRoot.anchorMap.x * canvasRoot.zoom
                                           - canvasRoot.anchorView.x));
    view.contentY = Math.max(0, Math.min(Math.max(0, view.contentHeight - view.height),
                                         canvas.y + canvasRoot.anchorMap.y * canvasRoot.zoom
                                           - canvasRoot.anchorView.y));
  }

  /// The whole map, fitted. What "Zoom to map" gives you.
  readonly property real fitZoom: (brg.map.imageWidth > 0 && view.width > 0)
    ? Math.max(minZoom, Math.min(maxZoom,
        Math.min(view.width / brg.map.imageWidth,
                 view.height / brg.map.imageHeight)))
    : 1

  /// THE OPENING VIEW (Twilight): the Game Boy's own screen, plus **one block of breathing room on
  /// every side**, with the player centred. You open the map looking at what the player is looking
  /// at -- not at a postage stamp of the whole route.
  ///
  /// The screen is 20x18 tiles = 160x144 buffer px; a block is 32. One block out on each side is
  /// therefore 160+64 by 144+64.
  readonly property real defaultZoom: (view.width > 0 && view.height > 0)
    ? Math.max(minZoom, Math.min(maxZoom,
        Math.min(view.width / (160 + 64), view.height / (144 + 64))))
    : 1

  readonly property real scaledWidth: brg.map.imageWidth * zoom
  readonly property real scaledHeight: brg.map.imageHeight * zoom

  // ── The infinite well (camera-only) ────────────────────────────────────────────────────────
  //
  // Twilight, 2026-07-14: *"the map should have infinite scroll — just infinite invalid area. Things
  // can't be dragged there, this is just for the camera, and it fixes the problem of panels covering
  // things up and not wanting to reflow or resize anything because of panels."*
  //
  // So we pad the Flickable's content by one viewport of empty dark well on every side. That lets you
  // scroll the map far enough that ANY part of it clears a floating panel — the camera has somewhere
  // to go, and the map itself never has to shrink when a panel opens. It is CAMERA-ONLY: the padded
  // area holds no map, and nothing can be dropped or dragged out there — every drop (`dropCharacter`)
  // and every object drag clamps to the map's own tile range (0..blocks*2-1). One viewport each side
  // is enough to pull any edge past any panel, and being a plain bounded number it adds no lag and
  // cannot destabilise the Flickable.
  readonly property real wellPadX: Math.max(0, view.width)
  readonly property real wellPadY: Math.max(0, view.height)

  /// What is under the cursor (brg.map.describeAt()), or null when it is off the map. The status
  /// bar reads this; nothing else does.
  property var at: null

  /// Space held = pan with any tool (the universal convention, and it costs nothing).
  property bool spaceHeld: false

  readonly property bool panning: tool === "pan" || spaceHeld

  /// Bumped to cancel any drag in flight. A delegate watching this puts the sprite back where it
  /// was and writes **nothing** -- Esc must never be a half-commit.
  property int cancelDrag: 0

  Shortcut {
    sequences: ["Escape"]
    onActivated: {
      canvasRoot.cancelDrag++;
      canvasRoot.selectedNpc = -1;
      canvasRoot.selectedConnection = -1;
    }
  }

  // The dark well. Not black: black would make the darkest Game Boy grey vanish into it.
  Rectangle {
    anchors.fill: parent
    color: "#2b2b2b"
  }

  Flickable {
    id: view

    anchors.fill: parent
    clip: true

    // The map, PLUS one viewport of empty well on every side (canvasRoot.wellPad*). @see the note on
    // wellPadX -- this is the "infinite scroll" that lets the camera pull any part of the map out
    // from under a floating panel without the map ever resizing.
    contentWidth: canvasRoot.scaledWidth + 2 * canvasRoot.wellPadX
    contentHeight: canvasRoot.scaledHeight + 2 * canvasRoot.wellPadY
    boundsBehavior: Flickable.StopAtBounds

    // Both axes, and a drag anywhere pans -- the map is the thing, not the scrollbars.
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    interactive: true

    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

    // Genuinely nothing in the game to draw, and nothing this id is a copy of either.
    Text {
      anchors.centerIn: parent
      visible: !brg.map.valid
      text: qsTr("There is no map data in the game for this one.")
      font.pixelSize: 13
      color: "#bdbdbd"
    }

    Item {
      id: canvas

      visible: brg.map.valid

      // Centre the map while it is smaller than the view, then let it grow past it.
      x: Math.max(0, (view.contentWidth - width) / 2)
      y: Math.max(0, (view.contentHeight - height) / 2)
      width: canvasRoot.scaledWidth
      height: canvasRoot.scaledHeight

      readonly property real gridStep: brg.map.blockSize * canvasRoot.zoom

      // The map floats: a soft shadow under it, so the well reads as BEHIND rather than as a
      // border drawn around the art.
      Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        color: "#40000000"
        radius: 3
        z: -1
      }

      // ── The full NEIGHBOUR maps (Phase 7b part 2) ────────────────────────────────────────────
      //
      // Twilight: *"I think it might be better to have the full connecting map on there to make it
      // easier to slide around."* So each connection renders its neighbour map, bleeding off the edge,
      // aligned so the neighbour's shared edge meets ours (shifted by the offset). It sits BEHIND our
      // own map image (z −0.5), which is opaque, so our buffer (ring + bled strip) covers the overlap
      // and only the part beyond our edge shows in the well. Dimmed, so our map stays the subject.
      //
      // Alignment, in buffer px (ring = 3 blocks = 96): the neighbour image has its own 3-block ring,
      // so its map area starts at (96,96). We place the image so that map area's shared edge lands on
      // ours. Four fixed items, bound to `revision`, so an offset drag re-positions them live.
      Repeater {
        model: 4
        delegate: PixelImage {
          id: nbr
          required property int index

          readonly property var e: { canvasRoot.revision; return canvasRoot.connEdgeFor(index); }
          readonly property bool present: e && e.exists === true && e.toTileset >= 0
                                          && brg.mapLayers.showConnections && brg.map.valid

          visible: present
          z: -0.5
          opacity: 0.45

          // The live frame, so the neighbour's water and flowers animate in step with our map
          // (Twilight, 2026-07-15). Cached per frame; re-renders ~3x a second like the main map.
          source: present ? ("image://map/" + e.toMap + "/" + e.toTileset + "/" + brg.map.frame + "/"
                             + brg.map.contrast + "/-1/-1/-1") : ""

          readonly property real ring: 96
          readonly property real off: (present ? e.offset : 0) * 32
          readonly property real nbTW: present ? e.toW * 32 : 0   // neighbour map area, buffer px
          readonly property real nbTH: present ? e.toH * 32 : 0

          readonly property real mX: brg.map.mapX
          readonly property real mY: brg.map.mapY
          readonly property real mW: brg.map.mapW
          readonly property real mH: brg.map.mapH

          // Image top-left in buffer px, so the neighbour's map edge meets ours.
          readonly property real imgX: index === 2 ? (mX + mW - ring)          // East
                                     : index === 3 ? (mX - ring - nbTW)        // West
                                     : (mX + off - ring)                       // North / South
          readonly property real imgY: index === 0 ? (mY - ring - nbTH)        // North
                                     : index === 1 ? (mY + mH - ring)          // South
                                     : (mY + off - ring)                       // East / West

          x: imgX * canvasRoot.zoom
          y: imgY * canvasRoot.zoom
          width: present ? (nbTW + 2 * ring) * canvasRoot.zoom : 0
          height: present ? (nbTH + 2 * ring) * canvasRoot.zoom : 0

          // The neighbour's own sprites, STATIC (Twilight: "sprites there, just not moving"). From the
          // DB/ROM (a neighbour isn't in the save). Children of the dimmed neighbour image, so they
          // inherit its 45% opacity and read as context. Positioned in the neighbour's own map: its map
          // area starts at the 3-block ring (96 px), and a sprite step is 16 px (with the 4 px OAM lift).
          Repeater {
            model: { canvasRoot.revision;
                     return nbr.present ? brg.map.neighbourSprites(nbr.index) : []; }
            delegate: Image {
              required property var modelData
              source: modelData.source
              smooth: false
              fillMode: Image.PreserveAspectFit
              width: 16 * canvasRoot.zoom
              height: 16 * canvasRoot.zoom
              x: (nbr.ring + modelData.x * 16) * canvasRoot.zoom
              y: (nbr.ring + modelData.y * 16 - 4) * canvasRoot.zoom
            }
          }
        }
      }

      // The whole overworld buffer: the map inside its border ring.
      //
      // A PixelImage, not an Image: it samples through the pixel-art shader, which is what lets the
      // zoom be a real number instead of snapping to whole multiples. See PixelImage.qml.
      PixelImage {
        anchors.fill: parent
        source: brg.map.source
      }

      // The semantic overlay -- walls, grass, warps... -- rendered by MapEngine as ONE transparent
      // image exactly the size of the map, so it can simply sit on top.
      //
      // An image, not a pile of QML rectangles, and that is not premature: Route 17 is 78 blocks
      // tall, which is over 20,000 tiles. As delegates it would crawl; as an image it scales with
      // the zoom for free and fades as one thing.
      PixelImage {
        anchors.fill: parent
        source: brg.map.overlaySource

        // The map stays the point. The layers arrive, they don't slam on. How HARD they are painted
        // is the Layers panel's one dial -- stacked annotation over four shades of grey needs it.
        opacity: brg.map.layers !== 0 ? brg.mapLayers.overlayOpacity : 0
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
      }

      // ── The block grid ──────────────────────────────────────────────────────────────────────
      //
      // TINTED rather than grey on purpose: the map is four shades of grey, so a grey line
      // disappears into it (over the black trees or over the white paths, depending which grey you
      // pick). A low-alpha colour has nothing to hide against and reads everywhere without
      // shouting over the three boxes.
      // ── The ink ──────────────────────────────────────────────────────────────────────────────
      //
      // Okabe-Ito, the colour-blind-safe set, re-picked 2026-07-13 (Twilight: "lots of red outlines
      // and it looks confusing"). Three warm theme colours -- error red, primary pink, accent blue --
      // over a grey map read as one alarming mess and told you nothing. These stay distinct to every
      // kind of colour vision and sit quietly over four shades of grey.
      //
      // The SAME values are in MapLayersModel, which is what the Layers panel's swatches show -- so
      // the panel is the legend, and it cannot drift.
      readonly property color boundsColor: "#0072b2"   // blue   -- where the map ENDS
      readonly property color drawColor:   "#009e73"   // green  -- what the game REDRAWS
      readonly property color screenColor: "#e69f00"   // orange -- what the player SEES
      readonly property color selectColor: "#cc79a7"   // purple     -- what YOU picked
      readonly property color connectColor: "#d55e00"  // vermillion -- the NEIGHBOURS in the ring

      readonly property color gridColor: Qt.rgba(0.34, 0.71, 0.91, 0.42)
      readonly property color tileGridColor: Qt.rgba(0.34, 0.71, 0.91, 0.18)
      readonly property real tileStep: brg.map.blockSize / 4 * canvasRoot.zoom

      // The TILE grid (8px). Off by default -- it is four times as many lines, and at 1x they would
      // be a hairline every eight pixels. Under the block grid, so the coarse structure still reads.
      Repeater {
        model: brg.mapLayers.showTileGrid && canvas.tileStep >= 4
               ? Math.floor(canvas.width / canvas.tileStep) + 1 : 0
        Rectangle {
          required property int index
          x: index * canvas.tileStep
          y: 0
          width: 1
          height: canvas.height
          color: canvas.tileGridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showTileGrid && canvas.tileStep >= 4
               ? Math.floor(canvas.height / canvas.tileStep) + 1 : 0
        Rectangle {
          required property int index
          x: 0
          y: index * canvas.tileStep
          width: canvas.width
          height: 1
          color: canvas.tileGridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showBlockGrid ? Math.floor(canvas.width / canvas.gridStep) + 1 : 0
        Rectangle {
          required property int index
          x: index * canvas.gridStep
          y: 0
          width: 1
          height: canvas.height
          color: canvas.gridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showBlockGrid ? Math.floor(canvas.height / canvas.gridStep) + 1 : 0
        Rectangle {
          required property int index
          x: 0
          y: index * canvas.gridStep
          width: canvas.width
          height: 1
          color: canvas.gridColor
        }
      }

      // ── The connections ──────────────────────────────────────────────────────────────────────
      //
      // The ring is NOT a wall of trees: the game bleeds each neighbouring map's edge into it, and
      // where each strip lands is the hardest arithmetic in the whole engine (a clamp that turns one
      // signed offset into two numbers, and two different loop shapes -- see
      // reference/map-connections.md). It is worth being able to look at it, so here it is: the
      // strip, its neighbour's name, and its size in blocks.
      //
      // Every number comes from brg.map (connectionList / connectionEditList) in buffer pixels; QML
      // multiplies by zoom. Phase 7b: the strips are now EDITABLE -- each is a selectable, draggable
      // MapConnection (slide it along the edge to set the offset, ✕ to delete), and every edge with no
      // connection carries a ghostly ConnectionArrow to add one. Four fixed items each, NOT Repeater
      // delegates, so an offset edit (which bumps `revision`) never rebuilds one mid-drag.
      Repeater {
        model: 4
        delegate: MapConnection {
          required property int index
          canvas: canvasRoot
          dir: index
          onEditRequested: canvasRoot.editRequested(-1)   // -1 = "not a sprite"; panel reads selectedConnection
        }
      }

      Repeater {
        model: 4
        delegate: ConnectionArrow {
          required property int index
          canvas: canvasRoot
          dir: index
        }
      }

      // Where the real map ends and the 3-block border ring begins.
      Rectangle {
        visible: brg.mapLayers.showMapBounds
        x: brg.map.mapX * canvasRoot.zoom
        y: brg.map.mapY * canvasRoot.zoom
        width: brg.map.mapW * canvasRoot.zoom
        height: brg.map.mapH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.boundsColor
      }

      // The draw area: the 6x5 blocks LoadCurrentMapView redraws. Always block-aligned.
      Rectangle {
        visible: brg.mapLayers.showDrawArea
        x: canvasRoot.boxes.drawX * canvasRoot.zoom
        y: canvasRoot.boxes.drawY * canvasRoot.zoom
        width: canvasRoot.boxes.drawW * canvasRoot.zoom
        height: canvasRoot.boxes.drawH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.drawColor
      }

      // The player. He is drawn 4px ABOVE his tile row -- "which makes sprites appear to be in the
      // centre of a tile" (ram/wram.asm), confirmed against the console's own OAM. Facing right is
      // facing LEFT, mirrored: there is no right-facing art in the game.
      //
      // He is drawn through the OBJECT palette (rOBP0), which is the one the "harmless" glitch
      // palettes actually wreck -- contrast 1 and 2 leave the map looking perfectly normal and do
      // their damage here. (reference/sprites.md)
      // ── The PLAYER ────────────────────────────────────────────────────────────────────────
      //
      // He is slot 0, and he is a sprite like any other: click him, drag him. There was never a
      // reason he should be the one thing on the map you could not pick up (Twilight, 2026-07-13).
      MapSprite {
        // Gated on his layer only. (He used to also hide when stacked under another object; the
        // stacking feature was removed 2026-07-15.)
        visible: brg.mapLayers.showPlayer

        canvas: canvasRoot
        slot: 0
        tileX: brg.map.playerX
        tileY: brg.map.playerY
        art: brg.map.playerSource
        inSet: true                       // his picture is always loaded

        onEditRequested: canvasRoot.editRequested(0)
      }

      // ── Everybody else ────────────────────────────────────────────────────────────────────
      //
      // The other fifteen sprite slots: every NPC, item ball and boulder the save has put on this
      // map. Same geometry as the player (they ARE the player's geometry), same OBJECT palette,
      // same "there is no right-facing art" rule.
      //
      // Click one to select it. The ground is NOT clickable (Twilight): blocks and tiles are edited
      // in their panels, and the canvas should not compete with them.
      Repeater {
        model: canvasRoot.npcs      // @see canvasRoot.revision -- a plain npcList() never re-asked

        delegate: MapSprite {
          required property var modelData

          canvas: canvasRoot
          slot: modelData.slot
          tileX: modelData.x
          tileY: modelData.y
          art: modelData.source
          inSet: modelData.inSpriteSet

          // ⚠️ THE SLIDE. `TryWalking` moves the sprite's TILE to the destination at once and then
          // slides it a pixel a frame for 16 frames -- so the tile is where they are GOING, and
          // drawing straight from it skipped the whole step and they teleported. The model hands us
          // the exact sub-tile offset. (@see MapModel::npcList)
          offX: modelData.offX
          offY: modelData.offY

          onEditRequested: canvasRoot.editRequested(modelData.slot)
        }
      }

      // ── The DOORS ─────────────────────────────────────────────────────────────────────────
      //
      // The map's warp points, as objects you can pick up. Same machinery as the sprites -- select,
      // drag, ✕, ✎ -- and the same one Details panel.
      //
      // ⚠️ An edited door is GENUINELY LIVE: `LoadMainData` sets BIT_NO_PREVIOUS_MAP on the saved
      // tileset byte, so the next `LoadMapHeader` bails out before it can rebuild the warp list from
      // ROM. Verified on the cartridge. The game DOES put the map's original doors back the moment
      // the player leaves and walks in again -- which the Details panel says, in words.
      // See notes/reference/warps.md.
      Repeater {
        model: canvasRoot.warps   // @see canvasRoot.revision -- a plain warpList() never re-asks

        delegate: MapWarp {
          required property var modelData

          canvas: canvasRoot
          ind: modelData.ind
          tileX: modelData.x
          tileY: modelData.y
          destName: modelData.destName
          destValid: modelData.destValid
          isReturn: modelData.isReturn

          onEditRequested: canvasRoot.editRequested(-1)   // -1 = "a door, not a sprite"
        }
      }

      // ── The SIGNS ─────────────────────────────────────────────────────────────────────────
      //
      // ⚠️ An edited sign is GENUINELY LIVE, on the same linchpin as a door: `.loadSignData` sits
      // inside `LoadMapHeader`, behind BIT_NO_PREVIOUS_MAP, so the next load bails before it rebuilds
      // the sign list from ROM. The game restores the map's original signs when the player leaves and
      // walks back in -- which the Details panel says, in words. See notes/reference/signs.md.
      Repeater {
        model: canvasRoot.signs   // @see canvasRoot.revision -- a plain signList() never re-asks

        delegate: MapSign {
          required property var modelData

          canvas: canvasRoot
          ind: modelData.ind
          tileX: modelData.x
          tileY: modelData.y
          preview: modelData.preview
          textValid: modelData.textValid

          onEditRequested: canvasRoot.editRequested(-1)   // the panel reads selectedSign
        }
      }

      // The STORAGE BLOCKS -- one per block of this map that has anything the save remembers on it.
      // Each carries `z: 0`, so they sit UNDER the real objects: a box annotates a thing, it must
      // never cover it. A box with nothing on top of it is not a gap -- it is the save hiding that
      // object, which is what the dashes say. @see notes/plans/map-screen.md -> Phase 16f
      Repeater {
        model: canvasRoot.flagBoxes   // ROM-derived: no `revision` dependency, and it must not have one

        delegate: MapBlockHotspot {
          required property var modelData

          canvas: canvasRoot
          block: modelData
          activeKinds: canvasRoot.activeStorageKinds

          onSpotClicked: (section, ind) => canvasRoot.storageRequested(section, ind)
        }
      }

      // (The MapObjectStack "group box" for overlapping objects was REMOVED 2026-07-15 — Twilight:
      //  the feature never worked well and was added from a misunderstanding. Overlapping chips just
      //  draw over each other now, each independently selectable.)

      // The visible screen: the 20x18 tiles actually on the Game Boy's screen.
      Rectangle {
        visible: brg.mapLayers.showScreenBox
        x: canvasRoot.boxes.screenX * canvasRoot.zoom
        y: canvasRoot.boxes.screenY * canvasRoot.zoom
        width: canvasRoot.boxes.screenW * canvasRoot.zoom
        height: canvasRoot.boxes.screenH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.screenColor
      }

      // ── Pointing at things ──────────────────────────────────────────────────────────────────
      //
      // ⚠️ The GROUND IS NOT SELECTABLE (Twilight, 2026-07-13). Clicking a block used to mark it;
      // it no longer does anything at all. Blocks and tiles are edited in their own panels, and a
      // clickable floor only fights the thing that should be clickable. **Sprites are, for now,
      // the only selectable object on the map** -- warps, signs and connections join them later,
      // on this same machinery.
      //
      // The hover readout stays: the status bar still tells you what you are pointing at, which
      // costs nothing and interrupts nobody.
      //
      // Below the Flickable's own drag handling, so a drag still pans the map and only a genuine
      // click acts.
      HoverHandler {
        id: hover
        cursorShape: canvasRoot.panning ? Qt.OpenHandCursor
                   : canvasRoot.tool === "zoom" ? Qt.CrossCursor
                     // A maker tool says so with the cursor. A crosshair over a full map would be a
                     // lie -- with no room left, the tool is dead, and it looks it.
                   : (canvasRoot.placing && canvasRoot.placeRoomLeft > 0) ? Qt.CrossCursor
                   : canvasRoot.placing ? Qt.ForbiddenCursor
                   : Qt.ArrowCursor

        onPointChanged: {
          const px = Math.floor(point.position.x / canvasRoot.zoom);
          const py = Math.floor(point.position.y / canvasRoot.zoom);
          canvasRoot.at = brg.map.describeAt(px, py);
        }
      }

      TapHandler {
        onTapped: (eventPoint) => {
          // ⚠️ THE TAP LANDED ON A PANEL, NOT ON THE MAP.
          //
          // The docks FLOAT over this canvas, so a point inside one of them is also inside us -- and
          // Qt walks every pointer handler under the point, front to back, before any item sees a
          // mouse event. A TapHandler on the default `DragThreshold` policy takes no exclusive grab,
          // so it does not stop that walk: the picker's handler fired AND this one fired, and this
          // one cleared the selection the Details panel was editing. (Reproduced with the harness's
          // `tap` -- a real pointer event -- 2026-07-13.)
          //
          // Asking "was it over a panel?" is a plain containment test and no grab policy can defeat
          // it. @see overPanel.
          const g = canvas.mapToGlobal(eventPoint.position.x, eventPoint.position.y);
          if (canvasRoot.overPanel(g.x, g.y))
            return;

          const px = Math.floor(eventPoint.position.x / canvasRoot.zoom);
          const py = Math.floor(eventPoint.position.y / canvasRoot.zoom);

          if (canvasRoot.tool === "zoom") {
            // A RATIO, not a fixed step -- the same reason the wheel uses one. A 1.4x bite feels the
            // same at 0.6x as it does at 8x; "+1" does not.
            // A click IS a detent -- there is nothing between one click and the next -- so this one
            // gets the same 90ms bridge the wheel does.
            const bite = (eventPoint.modifiers & Qt.AltModifier) ? (1 / 1.4) : 1.4;
            view.zoomAround(canvasRoot.zoom * bite, eventPoint.position, true);
            return;
          }

          if (canvasRoot.panning)
            return;   // the hand does not select

          // ⚠️ A POPUP IS OPEN OVER US -- so this tap is not somebody clicking the ground, it is the
          // press that dismissed the popup, arriving here afterwards.
          //
          // That is the bug Twilight hit: open the picture picker from the Details panel and you were
          // dropped straight back to the map's details, because the picker's overlay leaked its
          // dismiss-press down onto this handler, which cleared the selection the panel was editing.
          //
          // Making the picker non-modal fixes the leak; this makes it *impossible*. Any popup over
          // the canvas raises this count, and while it is up the ground does not take clicks.
          if (canvasRoot.popupsOpen > 0)
            return;

          // ── A MAKER TOOL IS IN HAND: the click puts something there ────────────────────────
          //
          // The cap is stated on the context bar *before* you get here, and the cursor has already
          // gone to a "no" -- so a click with no room left is not a silent no-op, it is a click on
          // something that has been visibly dead for a while. We still say why.
          if (canvasRoot.placing) {
            if (canvasRoot.placeRoomLeft <= 0) {
              canvasRoot.status = canvasRoot.tool === "placeWarp"
                ? qsTr("This map already has all 32 warps the game can hold.")
                : canvasRoot.tool === "placeSign"
                ? qsTr("This map already has all 16 signs the game can hold.")
                : qsTr("This map already has all 15 characters the game can hold.");
              return;
            }

            const tx = Math.floor(px / 16);
            const ty = Math.floor(py / 16);

            if (canvasRoot.tool === "placeWarp") {
              const ind = brg.map.addWarp(tx, ty);
              if (ind < 0)
                return;

              // A tool that makes a thing you then cannot see is a bug. Light its layer.
              if (!brg.mapLayers.showWarps)
                brg.mapLayers.setKeyVisible("warps", true);

              canvasRoot.selectedWarp = ind;
              canvasRoot.placed("warp", ind);
              return;
            }

            if (canvasRoot.tool === "placeSign") {
              const sind = brg.map.addSign(tx, ty);
              if (sind < 0)
                return;

              if (!brg.mapLayers.showSigns)
                brg.mapLayers.setKeyVisible("signs", true);

              canvasRoot.selectedSign = sind;
              canvasRoot.placed("sign", sind);
              return;
            }

            // A RANDOM character -- but only ever one of the eleven pictures **this map has actually
            // loaded**, so the tool can never conjure one of the amber "the console would draw
            // garbage here" sprites. That is the whole difference between a quick tool and a trap.
            // (The Characters bar remains the precise path: drag exactly who you want.)
            const slot = brg.map.addRandomLoadedNpc(tx, ty);
            if (slot < 0) {
              canvasRoot.status = qsTr("This map has no character pictures loaded to pick from.");
              return;
            }

            if (!brg.mapLayers.showNpcs)
              brg.mapLayers.setKeyVisible("npcs", true);

            canvasRoot.selectedNpc = slot;
            canvasRoot.placed("sprite", slot);
            return;
          }

          // Clicking the ground clears the selection -- and does nothing else. The block under the
          // cursor is NOT selected any more; see the note above.
          canvasRoot.selectedNpc = -1;
          canvasRoot.selectedWarp = -1;
          canvasRoot.selectedSign = -1;
          canvasRoot.selectedConnection = -1;
        }
      }

    }

    // ── Navigation ───────────────────────────────────────────────────────────────────────────
    //
    // Zoom ANCHORS on the thing you are pointing at (the cursor, or the middle of the pinch) rather
    // than the top-left corner. Zooming into a corner you aren't looking at is the single most
    // annoying thing a map viewer can do.

    /// Zoom to @p newZoom, keeping the map point under @p centre (viewport coords) exactly where it
    /// is -- on **every frame**, not just at the end.
    ///
    /// @p bridge is true ONLY for a mouse-wheel detent, where the hardware left a gap that has to be
    /// crossed somehow. Everything else -- the slider, a trackpad, a pinch -- is already continuous
    /// and goes straight through with no interpolation at all. @see canvasRoot's zoom note.
    function zoomAround(newZoom, centre, bridge) {
      newZoom = Math.max(canvasRoot.minZoom, Math.min(canvasRoot.maxZoom, newZoom));
      if (Math.abs(newZoom - canvasRoot.zoom) < 0.0001)
        return;

      // Where that point sits on the MAP right now, in unscaled buffer pixels. This is what has to
      // stay put.
      canvasRoot.anchorMap = Qt.point((view.contentX + centre.x - canvas.x) / canvasRoot.zoom,
                                      (view.contentY + centre.y - canvas.y) / canvasRoot.zoom);
      canvasRoot.anchorView = centre;

      canvasRoot.bridging = (bridge === true);
      canvasRoot.userZoom = newZoom;
    }

    /// Centre the view on a point in BUFFER pixels. Used by "Go to…", which sets the zoom first --
    /// so the anchor is the destination itself and it stays dead centre while the map eases in.
    function centreOn(bx, by) {
      canvasRoot.anchorMap = Qt.point(bx, by);
      canvasRoot.anchorView = Qt.point(view.width / 2, view.height / 2);

      // Nudge the pin once now, in case the zoom did not actually change (in which case there is no
      // animation to ride, and onZoomChanged will never fire).
      Qt.callLater(function() {
        view.contentX = Math.max(0, Math.min(Math.max(0, view.contentWidth - view.width),
                                             canvas.x + bx * canvasRoot.zoom - view.width / 2));
        view.contentY = Math.max(0, Math.min(Math.max(0, view.contentHeight - view.height),
                                             canvas.y + by * canvasRoot.zoom - view.height / 2));
      });
    }

    // Pinch: touchscreen, and a touchpad's two-finger pinch. It tracks the gesture continuously --
    // there is nothing to snap to any more.
    // A pinch is CONTINUOUS. Straight through, no interpolation -- it is already smooth, and
    // bridging it would only put lag between your fingers and the map.
    PinchHandler {
      target: null
      onActiveScaleChanged: {
        if (!active)
          return;
        view.zoomAround(canvasRoot.zoom * (1 + (activeScale - 1) * 0.25), centroid.position, false);
      }
    }

    // Ctrl+wheel (and a touchpad's pinch, which most platforms report as Ctrl+wheel).
    //
    // MULTIPLICATIVE, not additive: a fixed step feels violent when you are at 0.6x and glacial when
    // you are at 10x. A constant ratio per notch feels the same everywhere, which is the whole point
    // of a smooth zoom. 1.12 per notch is about eight notches per doubling.
    WheelHandler {
      acceptedModifiers: Qt.ControlModifier
      onWheel: (event) => {
        // A TRACKPAD reports a continuous pixelDelta -- true sub-pixel input, so it goes straight
        // through with no bridge. A MOUSE WHEEL reports 120 units per detent and nothing in between,
        // so that one (and only that one) gets bridged. @see canvasRoot's zoom note.
        const fine = (event.pixelDelta.y !== 0);

        const notches = fine ? event.pixelDelta.y / 50
                             : event.angleDelta.y / 120;

        const from = canvasRoot.userZoom > 0 ? canvasRoot.userZoom : canvasRoot.zoom;

        view.zoomAround(from * Math.pow(1.12, notches), point.position, !fine);
      }
    }

    // A plain wheel scrolls -- vertically, and HORIZONTALLY when the wheel/trackpad says so
    // (Shift+wheel, or a real two-finger sideways swipe). Flickable does the vertical axis on its
    // own; the horizontal one it ignores unless we hand it over.
    WheelHandler {
      acceptedModifiers: Qt.NoModifier | Qt.ShiftModifier
      onWheel: (event) => {
        const dx = (event.angleDelta.x !== 0)
                 ? event.angleDelta.x
                 : ((event.modifiers & Qt.ShiftModifier) ? event.angleDelta.y : 0);

        if (dx !== 0) {
          view.contentX = Math.max(0, Math.min(view.contentWidth - view.width,
                                               view.contentX - dx));
          event.accepted = true;
          return;
        }

        event.accepted = false;  // let the Flickable scroll vertically
      }
    }
  }

  // ── The one zoom API ──────────────────────────────────────────────────────────────────────
  //
  // Everything that zooms goes through here: the slider, the Go-to entries, the wheel, the pinch.
  // There is exactly one zoom in this screen and this is it (Twilight: "I just don't want multiple
  // places where zoom is").

  /// Zoom, keeping the middle of the view where it is. What the slider drives.
  /// The SLIDER. Continuous input -- straight through, no bridge, no interpolation. This is the true
  /// sub-pixel zoom: drag it and the map is exactly where the slider says, every frame.
  function zoomToCentre(z) {
    view.zoomAround(z, Qt.point(view.width / 2, view.height / 2), false);
  }

  // ── The opening view ──────────────────────────────────────────────────────────────────────
  //
  // You open a map looking at what the PLAYER is looking at (Twilight): the Game Boy's screen with
  // a block of breathing room round it, him in the middle. Not a postage stamp of a 78-block route.
  //
  // `framed` is a one-shot per map: after that the view is yours, and re-centring it under you every
  // time something redraws would be maddening.
  property bool framed: false

  function frameOnPlayer() {
    if (canvasRoot.framed || view.width <= 0 || view.height <= 0 || !brg.map.valid)
      return;

    canvasRoot.framed = true;
    canvasRoot.userZoom = 0;   // = defaultZoom
    view.centreOn(brg.map.playerRectX + 8, brg.map.playerRectY + 8);
  }

  Component.onCompleted: Qt.callLater(canvasRoot.frameOnPlayer)
  onWidthChanged: Qt.callLater(canvasRoot.frameOnPlayer)

  /// The map we last framed. MapModel publishes one `changed()` for everything, so we watch the id
  /// ourselves rather than re-framing the view on every animation frame.
  property int framedMap: -1

  Connections {
    target: brg.map

    // A different map is a different place -- frame it the same way.
    function onChanged() {
      if (brg.map.mapInd === canvasRoot.framedMap)
        return;

      canvasRoot.framedMap = brg.map.mapInd;
      canvasRoot.framed = false;
      Qt.callLater(canvasRoot.frameOnPlayer);
    }
  }

  /// Go and look at something. @p kind is a MapModel::zoomTarget kind, plus "map" and "camera",
  /// which are about the FRAME rather than a point and so are answered here.
  function goTo(kind) {
    if (kind === "map") {
      canvasRoot.userZoom = canvasRoot.fitZoom;
      view.centreOn(brg.map.imageWidth / 2, brg.map.imageHeight / 2);
      canvasRoot.status = qsTr("The whole map.");
      return;
    }

    if (kind === "camera") {
      // Exactly what the Game Boy is showing -- no breathing room at all. (The DEFAULT view is this
      // plus one block on each side; this one is the screen itself.)
      canvasRoot.userZoom = Math.max(canvasRoot.minZoom,
                                     Math.min(canvasRoot.maxZoom,
                                              Math.min(view.width / 160, view.height / 144)));
      view.centreOn(brg.map.screenX + brg.map.screenW / 2,
                    brg.map.screenY + brg.map.screenH / 2);
      canvasRoot.status = qsTr("What the Game Boy is showing.");
      return;
    }

    const t = brg.map.zoomTarget(kind);
    if (!t.ok) {
      canvasRoot.status = qsTr("This map hasn't got one of those.");
      return;
    }

    // Close enough to see it, not so close you have lost the map around it. Four blocks of context
    // is about a screen and a half.
    const wanted = Math.min(view.width, view.height) / (4 * 32);
    canvasRoot.userZoom = Math.max(canvasRoot.minZoom, Math.min(canvasRoot.maxZoom, wanted));

    view.centreOn(t.x, t.y);
    canvasRoot.status = t.label ? qsTr("Went to %1.").arg(t.label) : "";
  }

  /// "x,y" in BUFFER pixels -> select the block there, exactly as a click would. (DEBUG harness:
  /// it can only set properties on named items, and `brg.map` is a model, not an item.)
  property string selectAt: ""
  onSelectAtChanged: {
    const p = selectAt.split(",");
    if (p.length !== 2)
      return;

    brg.map.selectAtPixel(parseInt(p[0]), parseInt(p[1]));
  }
}