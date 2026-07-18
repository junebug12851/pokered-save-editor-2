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
 * ONE BLOCK of the map, and the tabbed squares that reach everything on it.
 *
 * ══ THE STANDARD ═══════════════════════════════════════════════════════════════════════════════
 *
 * Fairy Fox has had to say *"there needs to be proper standardization"* three times, because three
 * cuts of this file each invented a local rule. So the whole of it is written here, once, and
 * nothing below is allowed to disagree with it.
 *
 * **1. EVERYTHING on a map is the same kind of thing.** A person, the player, a door, a sign, a
 * filter flag, a script trigger, a buried item, an event flag, grass, water — each is a **spot**,
 * on a **block**, with an **outline** and a **tab**. *"Like anything else they need to be rendered
 * and have a tab."* There is no category that is merely scenery and no category you cannot reach.
 *
 * **2. The OUTLINE says what you can DO** (and nothing else):
 * | | |
 * |---|---|
 * | **solid + filled** | you can move it: drag, delete, insert. The player, people, doors, signs. |
 * | **dashed + hollow** | it lives where the cartridge put it: scripts, filter flags, buried items, event flags, tile traits. You can change what it *does*, never where it *is*. |
 * A block containing anything movable reads solid — the box is the handle for the **cell**.
 *
 * **3. The COLOUR says WHICH LAYER it belongs to** — the same swatch the Layers panel paints on
 * that row, so **the panel is the legend**. It is never a code you have to learn from the map:
 * hover any tab and it tells you in words, and lights its own outline in that same colour so the
 * link is visible rather than remembered.
 *
 * **4. The TABS live at the TOP of the block, and only on mouseover.**
 * *"the tabs are supposed to be on the top and only on mouseover"* + *"tabbed little squares at the
 * top left"*. One horizontal strip along the block's top edge:
 * **non-tile tabs first (left), then a GAP, then the tile traits (right)** — *"i said the tile
 * traits go right not left i said the non tile traits go left"*. Two families, one strip, split by
 * the **unit**: the walk grid (16×16) vs the tile traits (8×8).
 *
 * **5. A tab is a HANDLE, not a label.** Hover → it lights its own thing. Click → it **selects**
 * that thing and opens its editor. (Clicking used to open a panel *without* selecting, so a door's
 * tab appeared to open the sprite's page — which is what *"why is it that color"* was really
 * reporting: not a colour bug, a selection bug.)
 *
 * @see notes/plans/map-screen.md -> Phase 16f
 *
 * ── The two granularities, and keeping them apart IS the design ──────────────────────────────
 *
 * (Leadership's settled call, 2026-07-17: *"why dont we highlight truthfully the ranges and have
 * clicks and mouse overs be on the blocks to keep things simple"*.)
 *
 *  - **The HIGHLIGHT is the truth.** Each spot draws at its OWN real size: a tile trait 8x8, an
 *    object or script tile 16x16, a coord range exactly the row or column it really covers. Drawn
 *    from `extX/extY/extW/extH`, which the model measures in the spot's own unit.
 *  - **The HIT TARGET is the block.** Uniform 32x32, one cursor cell, one tab strip. It claims to
 *    be nothing except a cell -- which is why a block-sized target isn't a lie about a 16x16 thing.
 *
 * ── The tabs ─────────────────────────────────────────────────────────────────────────────────
 *
 * Square colour tabs, and the two families sit on **opposite sides**, split by UNIT:
 * **non-tile tabs on the LEFT** (filter flags, script locations, coord ranges, hidden items,
 * event flags -- all on the 16x16 walk grid) and **tile traits on the RIGHT** (Door, Warp tiles,
 * grass, water, counters -- the 8x8 family).
 *
 * A tab is **the escape hatch**: it reaches its spot **directly, whatever is stacked above it**, so
 * a buried object is never unreachable and the grid needs no gating or special-casing.
 * Deliberately ONE spot = no tabs (a plain box, as before) -- tabs are for disambiguation, and a
 * strip of one disambiguates nothing. Clutter is a bug.
 *
 * @see MapModel::blockHotspots, notes/plans/map-screen.md -> Phase 16f
 */
import QtQuick
import QtQuick.Controls

Item {
  id: hot

  /// The canvas, for the zoom.
  required property var canvas

  /// One entry of MapModel::blockHotspots: `{blockX, blockY, rectX/Y/W/H, spots:[...]}`.
  required property var block

  /// A spot was reached. @p kind so the canvas can SELECT the thing (rule 5: a tab is a handle, not
  /// a label -- clicking one must select what it points at, or the editor opens on whatever was
  /// selected before), @p section so it knows which editor, @p ind which row/slot.
  signal spotClicked(string kind, string section, int ind)

  /// The spot kinds whose LAYER is currently on. A tab exists only for a layer that is showing --
  /// leadership, 2026-07-17: *"the tab strip only needs to have tabs based on the active layers"*.
  ///
  /// This is the same doctrine that decides the click: **priority IS the Layers panel's own order**,
  /// and a layer that is off draws nothing and catches nothing, so a tab into it would be pointing
  /// at something that isn't on screen. It also keeps the strip honest about clutter -- turn a layer
  /// off and its tabs go with it.
  required property var activeKinds

  /// Only what the active layers admit. Everything below -- the tabs, the gap, the single-spot hit
  /// target -- reads THIS, never the raw list, so a hidden layer can never be reached by accident.
  readonly property var spots: hot.block.spots.filter(s => hot.activeKinds.indexOf(s.kind) >= 0)

  /// Is the cursor in this block? Read from the canvas's ONE HoverHandler, never from a MouseArea
  /// of our own.
  ///
  /// ⚠️ This is the flicker fix. A per-block `MouseArea` cannot host this test, because the tab
  /// strip is a CHILD of the block and **a child MouseArea steals hover from its parent**: strip
  /// appears → takes the hover → parent goes false → strip hides → parent true → … at frame rate.
  /// *"the tabs when moused over glitch on and off rapidly"*. The canvas's HoverHandler stays
  /// hovered over children, so there is no loop to enter. @see MapCanvas.hoverBlockX
  ///
  /// ⭐ ...and the strip WITHDRAWS when the pointer is on a movable object in this very cell
  /// (*"the cell tabs go away when mousing over a moveable item"*). Point at the thing and you get
  /// the thing — its own outline, ready to drag. Point at the cell around it and you get the cell's
  /// tabs. Your cursor says which you meant, so nothing has to guess.
  /// Unless it fills the cell, in which case the tabs are the only way in and must stay.
  /// @see MapCanvas.hoverMovable
  readonly property bool showTabs: hot.canvas.hoverBlockX === hot.block.blockX
                                   && hot.canvas.hoverBlockY === hot.block.blockY
                                   && !(hot.canvas.hoverMovable === hot.blockKey
                                        && !hot.canvas.hoverMovableFullCell)

  readonly property string blockKey: hot.block.blockX + "," + hot.block.blockY

  /// The tab the cursor is on, or -1. Hovering a tab LIGHTS ITS OWN SPOT on the map -- which is the
  /// answer to *"nothing comes up when i mouse over a square"*: a strip of coloured dots means
  /// nothing until you can see which one is which. This is also the "everything overlapping is
  /// accessible under mouseover" rule made real: run the cursor down the strip and each thing in
  /// the cell announces itself in turn, however buried it is.
  property int hoveredSpot: -1

  /// The tile-based family (8x8 tile TRAITS) and the walk-grid family, told apart by the spot's own
  /// unit. The gap between them is the whole point of the split, and the unit is what defines it --
  /// which is exactly why `blockHotspots` carries a unit per spot.
  readonly property var tileSpots: hot.spots.filter(s => s.unit === "tile")
  readonly property var gridSpots: hot.spots.filter(s => s.unit !== "tile")

  /// ⭐ EVERY block with anything on it gets tabs. ALWAYS. No threshold, no exception.
  ///
  /// > *"Mousing over things nothing comes up no tabs on water or sprites or anything ... theres no
  /// >  standardixation"* -- Fairy Fox, 2026-07-17
  ///
  /// The first cut only tabbed a block with **more than one** spot, reasoning that "a strip of one
  /// disambiguates nothing". That rule was mine, not hers, and it was wrong for the reason she
  /// gives: it makes the map answer *differently depending on what you point at*. A block with two
  /// things was interactive; the water block beside it was dead. That is not a simplification, it
  /// is an inconsistency — and an inconsistent map is unlearnable, however tidy each half looks.
  ///
  /// One tab per thing, everywhere, is the standard. It also costs nothing: a block with one thing
  /// shows one small square, which is exactly as much furniture as that block's content deserves.
  readonly property bool tabbed: hot.spots.length > 0

  // A block whose every spot belongs to a switched-off layer is not a block with nothing on it --
  // it is a block you asked not to see. Either way it draws nothing.
  visible: hot.spots.length > 0

  // The BLOCK is the hit target: uniform, 32x32, the cursor cell.
  x: hot.block.rectX * hot.canvas.zoom
  y: hot.block.rectY * hot.canvas.zoom
  width: hot.block.rectW * hot.canvas.zoom
  height: hot.block.rectH * hot.canvas.zoom

  // Under the real objects: this annotates them, it must never cover them.
  z: 0

  /// The layer's ink, by what the spot is attached to -- her rule: a thing tied to a flag does not
  /// look like a thing that isn't. Okabe-Ito throughout, and each one is the swatch its own Layers
  /// row paints, so the row IS the legend.
  function inkOf(kind) {
    switch (kind) {
      case "filterFlag":  return "#009e73";  // Flag boxes -- bluish green
      case "eventFlag":   return "#56b4e9";  // Event flags -- sky blue
      case "script":      return "#e69f00";  // Script triggers -- orange
      case "cardKeyDoor": return "#d55e00";  // Card Key doors -- vermillion
      case "hiddenItem":  return "#cc79a7";  // Hidden items -- reddish purple
      case "hiddenCoin":  return "#f0e442";  // Hidden coins -- yellow
      case "tileTrait":   return "#999999";  // Tile traits -- the quiet family
      // The movable objects wear their own LAYER's colour, so a tab is visibly the same thing as
      // the chip it points at and the Layers panel row IS the legend (rule 3). People & objects
      // pink, Warps yellow, Signs orange, the Player his own grey-blue row.
      case "sprite":      return "#cc79a7";
      case "warp":        return "#f0e442";
      case "sign":        return "#e69f00";
      case "player":      return "#0072b2";
    }
    return "#009e73";
  }

  /// Is this spot's object currently switched OFF by the save? Only a filter flag can be, and it is
  /// read live so flipping a switch in Map Storage re-dashes one box instead of rebuilding the list.
  /// (WorldMissables: **bit set = HIDDEN**.)
  function isHidden(spot) {
    if (spot.kind !== "filterFlag")
      return false;
    hot.canvas.revision;
    return hot.canvas.wMissables ? hot.canvas.wMissables.missablesAt(spot.ind) : false;
  }

  /// A spot's tooltip: ONE short line, and never more.
  ///
  /// > *"the tooltips are awful cant read them and way too much reading"* -- Fairy Fox, on the first
  /// > cut, which hung the name + state + full description + caution + an instruction off a
  /// > five-pixel square.
  ///
  /// A tooltip on a dot is a **label**, not a page: it answers *"what is this?"* at a glance. The
  /// state rides along only where it is the whole point (a flag that is off; a flag turned OFF
  /// rather than on). Everything else -- the description, the caution, the raw byte -- is one click
  /// away in the panel, which is where prose belongs.
  function labelOf(spot) {
    switch (spot.kind) {
      case "filterFlag":
        return hot.isHidden(spot) ? qsTr("%1 — hidden").arg(spot.name) : spot.name;
      case "eventFlag":
        // "Turns off" is the surprising one and earns its word; "turns on" is the default reading.
        return spot.action === "reset" ? qsTr("%1 — turned off here").arg(spot.name) : spot.name;
      case "hiddenItem":
      case "hiddenCoin":
        return qsTr("%1 — hidden pickup").arg(spot.name);
      case "script":
        return spot.shape === "scriptRow" ? qsTr("Script — this whole row")
             : spot.shape === "scriptCol" ? qsTr("Script — this whole column")
             : qsTr("Script trigger");
      case "cardKeyDoor":
        return qsTr("Card Key door");
      case "tileTrait":
        // Grass and water are not scenery: they are WHERE THE WILD POKÉMON ARE, and that table is
        // editable. Say so, because "Grass" alone reads like a label and this is a link.
        return (spot.section === "wild") ? qsTr("%1 — wild Pokémon").arg(spot.name) : spot.name;
      case "sprite":
      case "warp":
      case "sign":
      case "player":
        return spot.name;
    }
    return spot.name;
  }

  /// How thick a box's line is. *"since we're only using borders youll have to make them a teensy
  /// bit thicker"* — with no fill to carry it, the LINE is the whole visual language, so solid-vs-
  /// dashed has to be legible at a glance over four shades of Game Boy grey. One px is not enough
  /// to tell a dash from a stroke; two is.
  readonly property int lineWidth: Math.max(2, Math.round(1.5 * hot.canvas.zoom))

  /// Can you MOVE this thing -- drag it, delete it, put a new one down?
  ///
  /// > *"stuff draggable, deletable, insertable, etc.... should have a solid fill and box, stuff
  /// >  that allows editing things from fixed locations like scripts and stuff should have a dashed
  /// >  outline and not be filled."*
  ///
  /// Nothing this component draws is movable **today**: a script trigger sits where the cartridge
  /// tests the player's coords, and a hidden pickup's tile *is* its save bit's identity -- you edit
  /// what they do, never where they are. So this answers false for every current kind, and the
  /// layer is honestly all-dashed.
  ///
  /// It is written as a rule rather than hard-coded to `false` because the movable kinds -- warps,
  /// signs, people -- are persistent storage with locations too, and when they join `spots[]` this
  /// is the one line that has to know. @see MapWarp, MapSign, MapSprite
  function isMovable(kind) {
    return kind === "warp" || kind === "sign" || kind === "sprite" || kind === "player";
  }

  /// ⭐ The BLOCK's outline style, decided by the block and not by each spot:
  ///
  /// > *"if there are multiple tabs fill and use solid line only if any of the tabs contain one
  /// >  thing that matches the rules above for solid fill and solid outline"*
  ///
  /// So one movable thing among many makes the whole box solid and filled -- because the box is the
  /// handle for the CELL, and if anything in that cell can be picked up then the cell can be picked
  /// up. Mixing a dashed box with a solid one inside a single cell would be saying two things about
  /// the same target.
  readonly property bool anyMovable: hot.spots.some(s => hot.isMovable(s.kind))

  // ── The HIGHLIGHTS -- each spot at its own true size, drawn ONCE ───────────────────────────
  //
  // Positioned relative to this block, but sized from the spot's real extent, so a coord range
  // legitimately draws far outside the block that owns this strip: the range IS that wide, and
  // shrinking it to fit the block would be a lie about the data.
  //
  // ⚠️ ONLY the block containing the spot's TOP-LEFT draws it. A spot is filed on every block its
  // extent touches -- that is the aggregation rule, and it is right for the TABS -- but the
  // highlight is one thing in the world, not one per cell. Without this guard Pallet Town's
  // north-row trigger drew its full-width rectangle TEN TIMES, once per block it crosses: the
  // translucent range stacked into a solid bar and the edges got ten times the ink. Caught by the
  // mandatory screenshot review, which is exactly the class of thing a glance misses.
  //
  // ⚠️ TILE TRAITS draw NO box of their own. `MapEngine::overlay()` already paints every one of
  // their tiles at its true 8x8, in the layer's own colour -- so a box here would be a second
  // answer to a question already answered, and one that has to agree with the first forever. Their
  // tab is a handle; the overlay is the highlight.
  //
  // ⚠️ `hilite: false` spots draw NOTHING here either -- the movable objects (people, doors, signs)
  // have their own components, which already draw and drag them. A second outline from this layer
  // would be two answers to one question, and they would have to agree forever. They are in the
  // model to be REACHABLE (a tab), not to be redrawn.
  readonly property var ownSpots: hot.spots.filter(s =>
      s.kind !== "tileTrait"
      && s.hilite !== false
      && Math.floor(s.extX / 32) === hot.block.blockX
      && Math.floor(s.extY / 32) === hot.block.blockY)

  Repeater {
    model: hot.ownSpots

    delegate: Item {
      id: spotItem
      required property var modelData
      required property int index

      /// Is the save currently switching this object OFF? (Only a filter flag can be.)
      readonly property bool dashed: hot.isHidden(modelData)

      /// Is the cursor on this spot's tab? Then say which one you mean.
      readonly property bool lit: hot.hoveredSpot >= 0
                                 && hot.spots[hot.hoveredSpot] === spotItem.modelData

      x: (modelData.extX - hot.block.rectX) * hot.canvas.zoom
      y: (modelData.extY - hot.block.rectY) * hot.canvas.zoom
      width: modelData.extW * hot.canvas.zoom
      height: modelData.extH * hot.canvas.zoom

      // ── DASHED, NEVER FILLED, and that is now a RULE, not a look ────────────────────────────
      //
      // > *"stuff draggable, deletable, insertable, etc.... should have a solid fill and box, stuff
      // >  that allows editing things from fixed locations like scripts and stuff should have a
      // >  dashed outline and not be filled."* -- Fairy Fox, 2026-07-17
      //
      // ⭐ So the outline tells you what you can DO before you touch it. Everything this component
      // draws is FIXED: a script trigger sits where the cartridge tests the player's coords, and a
      // hidden item's tile IS its save bit's identity -- you can edit what they do, never where they
      // are. Nothing here is draggable, so nothing here is filled. The solid-and-filled language
      // belongs to the objects you can pick up: warps, signs, people (MapWarp / MapSign / MapSprite).
      //
      // ⚠️ This RETIRES the old solid-vs-dashed meaning ("solid = shown, dashed = hidden"), which
      // used the same ink for a different question. A switched-off object now says so with the ⚑ and
      // a fainter line -- the outline STYLE is reserved for "can I move this?".
      //
      // The decision is the BLOCK's, not this spot's: one movable thing in the cell makes the whole
      // cell solid and filled. @see hot.anyMovable

      // The SOLID box -- something in this cell can be picked up.
      //
      // ⚠️ NO FILL. Anywhere. Twilight's settled call, 2026-07-17: *"Yea i guess dont fill in the
      // boxes but youll have to do that for all boxes on the map keep and maintain standardization.
      // I still want dashed and solid lines for visual language since we're only using borders
      // youll have to make them a teensy bit thicker."*
      //
      // So the language is carried ENTIRELY by the line: **solid = movable, dashed = fixed**, and
      // both are a touch thicker to make the difference readable at 1x over four shades of grey.
      // Nothing is ever washed -- a fill over Game Boy art repaints the thing you are trying to
      // look at, which is the mistake that tinted every Poké Ball in Oak's Lab green.
      Rectangle {
        anchors.fill: parent
        visible: hot.anyMovable
        color: "transparent"
        border.width: hot.lineWidth
        border.color: hot.inkOf(modelData.kind)
        radius: 2
      }

      Canvas {
        id: dashes
        anchors.fill: parent
        visible: !hot.anyMovable
        // A switched-off object is fainter; a range is a broad claim and says so quietly.
        // Hovering its tab brings it fully forward -- that IS the "which one is this?" answer.
        opacity: spotItem.lit ? 1.0
                              : (spotItem.dashed ? 0.5
                                 : (modelData.kind === "script" && modelData.shape !== "scriptTile"
                                      ? 0.55 : 1.0))
        Behavior on opacity { NumberAnimation { duration: 90 } }
        // The dashes are painted, not bound, so a size change has to ask for a repaint by hand --
        // otherwise zooming leaves the old rectangle stretched.
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
          const ctx = getContext("2d");
          ctx.reset();
          const w = hot.lineWidth;   // the same weight as the solid box, so only the DASH differs
          ctx.lineWidth = w;
          ctx.strokeStyle = hot.inkOf(modelData.kind);
          ctx.setLineDash([Math.max(3, 3 * hot.canvas.zoom), Math.max(3, 3 * hot.canvas.zoom)]);
          ctx.strokeRect(w / 2, w / 2, width - w, height - w);
        }
        // The dashes must be repainted when the zoom changes them.
        //
        // ⚠️ `dashes.requestPaint()`, NOT `parent.requestPaint()`. Inside a Connections nested in a
        // REPEATER DELEGATE, `parent` does not resolve to the enclosing Canvas -- it walks out to
        // the delegate's root (MapBlockHotspot), which has no such method, and every zoom throws
        // `TypeError: Property 'requestPaint' ... is not a function`. The same line worked verbatim
        // in MapFlagBox, which was not inside a delegate. Caught by tst_qml_screens, which is the
        // only thing in the suite that instantiates QML -- a silent binding error that no C++ test
        // could ever see.
        Connections {
          target: hot.canvas
          function onZoomChanged() { dashes.requestPaint(); }
        }
      }

      // ⚑ -- ONLY on an empty box. When the object is there, its own artwork is the thing worth
      // seeing and a glyph over its face is just in the way; the outline already says "the game
      // remembers this". When the flag hides it there IS no sprite, and the mark is what tells you
      // something belongs here.
      Text {
        anchors.centerIn: parent
        visible: spotItem.dashed && hot.canvas.zoom >= 1.5
        text: "⚑"
        font.pixelSize: Math.max(8, Math.round(9 * hot.canvas.zoom))
        color: hot.inkOf(modelData.kind)
      }
    }
  }

  // ── The TAB STRIPS -- square colour tabs, and the two families sit on OPPOSITE SIDES ───────
  //
  // > *"there is a gap in the tabs to seperate the tile-based tabs and non-tile-based tabs. door
  // >  and warp tile traits would be an example of tile tabs seperated from non-tile tabs like
  // >  filter flags or script locations or coord ranges these would be the tab tags on the left"*
  // > ...and, when the first cut put both on the left with a gap between them:
  // > *"i said the tile traits go right not left i said the non tile traits go left"*
  //
  // So the separation is a SIDE, not a gap in one column: **non-tile tabs LEFT, tile traits
  // RIGHT**. Which reads well -- the two families can never be mistaken for one strip, and you
  // always know which side to look at for which kind.

  /// ONE strip, along the block's TOP edge. @see the standard at the top of this file (rule 4).
  ///
  /// ⚠️ ONLY ON MOUSEOVER. *"the tabs are supposed to be on the top and only on mouseover"*. A tab
  /// per thing per block, shown always, dotted the whole map with furniture — every water block wore
  /// a permanent square. Tabs are how you REACH a crowded spot, so they belong to the moment you are
  /// pointing at one. The map is the point; the handles arrive when your hand does.
  ///
  /// It sits INSIDE the block's top edge, not above it. If it floated outside, moving the cursor
  /// from the block to the strip would leave the block, the strip would vanish, and the tabs would
  /// be permanently unreachable — a thing that hides exactly when you reach for it.
  Row {
    id: strip
    visible: hot.tabbed && hot.showTabs
    spacing: 1
    x: 1
    y: 1
    z: 3   // above the outlines AND the objects: a tab must always be reachable

    // Fade rather than blink: the strip is following the cursor, and a hard cut reads as a glitch.
    opacity: hot.showTabs ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 80 } }

    // NON-TILE first: the walk grid (16x16) -- people, doors, signs, filter flags, scripts,
    // buried items, event flags.
    Repeater { model: hot.gridSpots; delegate: tabDelegate }

    // THE GAP: it splits the two families, and the split is the UNIT. Only present when both
    // families are here to be told apart.
    Item {
      visible: hot.tileSpots.length > 0 && hot.gridSpots.length > 0
      height: 1
      width: Math.max(4, Math.round(4 * hot.canvas.zoom))
    }

    // TILE TRAITS last, on the RIGHT: the 8x8 family -- grass, water, doors, warp tiles, counters.
    Repeater { model: hot.tileSpots; delegate: tabDelegate }
  }

  Component {
    id: tabDelegate

    Rectangle {
      id: tab
      required property var modelData

      readonly property bool hovered: tabHit.containsMouse

      // Big enough to actually hit: a 5px square at 1x is a dare, not a target.
      width: Math.max(7, Math.round(6 * hot.canvas.zoom))
      height: width
      radius: 1
      color: hot.inkOf(modelData.kind)
      border.width: 1
      // Hovering lifts the tab out of the strip: a white ring, so it reads on every one of the
      // seven inks and on all four shades of Game Boy grey underneath.
      border.color: tab.hovered ? "#ffffff" : Qt.darker(color, 1.6)
      scale: tab.hovered ? 1.35 : 1.0
      Behavior on scale { NumberAnimation { duration: 90 } }
      // A tab for a switched-off object is hollow -- the same language the box itself speaks.
      opacity: hot.isHidden(modelData) ? 0.45 : 1.0

      // A MouseArea, never a PointerHandler -- a TapHandler fires THROUGH a floating panel, which
      // cost a whole review round once. See notes/reference/qt-patterns.md (top of file).
      MouseArea {
        id: tabHit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        // Tell the block which spot is under the cursor, so ITS box lights up on the map. Hovering
        // a dot must point at something, or the strip is just decoration.
        onEntered: {
          hot.hoveredSpot = hot.spots.indexOf(tab.modelData);
          // Tell the GROUND to stand down: its TapHandler runs before this MouseArea ever sees a
          // press, and takes no grab, so without this the ground eats every tab click.
          // @see MapCanvas.overTab
          hot.canvas.overTab = true;
        }
        onExited: {
          if (hot.spots[hot.hoveredSpot] === tab.modelData)
            hot.hoveredSpot = -1;
          hot.canvas.overTab = false;
        }
        // ⚠️ NEVER disabled. This used to read `enabled: modelData.section !== ""` -- and since tile
        // traits carried section "", that single line switched OFF hover, tooltip AND click for
        // water and grass, i.e. most of a water route. A disabled MouseArea is not a quiet tab; it
        // is a hole in the map. Every kind now has a real destination, so every tab is live.
        onClicked: hot.spotClicked(modelData.kind, modelData.section, modelData.ind)

        // ⚠️ ONE SHORT LINE. A tab is a 5-pixel square, and the first cut hung a paragraph off it --
        // the name, the state, the whole description, a caution, and an instruction. Twilight:
        // *"a very dark and extremely large tooltip filled with tons of text comes up ... the
        // tooltips are awful cant read them and way too much reading"*. She was right: a tooltip on
        // a dot is a LABEL, not a page. It answers "what is this?" in a glance and nothing else --
        // the full story is one click away in the panel, which is where prose belongs.
        //
        // No "Click to open in Map Storage" either: the cursor is already a pointing hand, so the
        // line spent saying it is a line that tells you what you can already see.
        ToolTip.visible: containsMouse
        ToolTip.delay: 300
        ToolTip.text: hot.labelOf(modelData)
      }
    }
  }

  // ⚠️ NO MouseArea here, and that is deliberate on two counts.
  //
  // 1. It cannot host the hover test -- the tab strip is its child, and a child MouseArea steals
  //    hover from its parent, which is the flicker. @see showTabs.
  // 2. It must not accept presses: this item spans a whole block, and an invisible grid that
  //    swallowed clicks would break dragging everywhere on the canvas.
  //
  // Hover comes from the canvas's ONE HoverHandler; the CELL OUTLINE it draws covers every block,
  // including the empty ones. Clicking is the tabs' job, uniformly -- a cell that guessed which of
  // several things you meant would be choosing behind your back.
}
