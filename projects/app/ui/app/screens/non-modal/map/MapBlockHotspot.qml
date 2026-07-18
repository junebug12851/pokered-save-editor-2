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

  // (The hovered-tab state and THE TAB STRIP ITSELF live on the CANVAS now -- `canvas.litSpot` +
  //  MapTabStrip.qml -- so the tabs can stack above the objects while these boxes stay under
  //  them. A child cannot out-stack its parent's siblings; splitting them was the only shape.)

  // A block whose every spot belongs to a switched-off layer is not a block with nothing on it --
  // it is a block you asked not to see. Either way it draws nothing.
  visible: hot.spots.length > 0

  // The BLOCK is the hit target: uniform, 32x32, the cursor cell.
  x: hot.block.rectX * hot.canvas.zoom
  y: hot.block.rectY * hot.canvas.zoom
  width: hot.block.rectW * hot.canvas.zoom
  height: hot.block.rectH * hot.canvas.zoom

  // ⭐ ALWAYS under the real objects (leadership, 2026-07-18: *"sprites should always be shown …
  // Player and people objects are at the top"*). A box annotates a thing; it never covers it —
  // and there is NO hover exception any more: the first cut raised the whole block (boxes and
  // all) over the sprites while hovered, which put a box across a character's face at exactly
  // the moment you were looking at him. The TABS still need to be above everything, and a child
  // cannot out-stack its parent's siblings — so the strip lives at CANVAS level now, one for the
  // whole map, following the hovered cell. @see MapTabStrip.qml
  z: 0

  /// The spot's ink -- carried BY the spot, from the model, out of the ONE canonical table
  /// (MapEngine::ink; tile traits wear their own overlay layer's swatch). This component used to
  /// keep a private kind→colour map here, which is exactly how the canvas and the Layers panel
  /// came to disagree (leadership, 2026-07-18: colours on the map that were in no panel row).
  /// Nothing in this file states a colour of its own now.
  function inkOf(spot) {
    return spot.ink !== undefined ? spot.ink : brg.map.ink(spot.kind);
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

      /// Is the cursor on this spot's tab? Then say which one you mean. (The strip lives on the
      /// canvas now, so the lit spot does too. Same JS object identity: both read storageBlocks.)
      readonly property bool lit: hot.canvas.litSpot === spotItem.modelData

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
        border.color: hot.inkOf(modelData)
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
        Behavior on opacity { NumberAnimation { duration: 60 } }
        // The dashes are painted, not bound, so a size change has to ask for a repaint by hand --
        // otherwise zooming leaves the old rectangle stretched.
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
          const ctx = getContext("2d");
          ctx.reset();
          const w = hot.lineWidth;   // the same weight as the solid box, so only the DASH differs
          ctx.lineWidth = w;
          ctx.strokeStyle = hot.inkOf(modelData);
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
        color: hot.inkOf(modelData)
      }
    }
  }

  // ── The TAB STRIP moved OUT (2026-07-18) ────────────────────────────────────────────────
  //
  // It lives at CANVAS level now -- MapTabStrip.qml, one strip for the whole map, following
  // the hovered cell -- so the tabs can stack ABOVE the objects while these boxes stay UNDER
  // them (leadership: "sprites should always be shown ... Player and people objects are at the
  // top"). A child cannot out-stack its parent's siblings, so the split is structural.

  // ⚠️ NO MouseArea here, and that is deliberate on two counts.
  //
  // 1. It cannot host the hover test -- the tab strip is its child, and a child MouseArea steals
  //    hover from its parent, which is the flicker. @see MapTabStrip.qml.
  // 2. It must not accept presses: this item spans a whole block, and an invisible grid that
  //    swallowed clicks would break dragging everywhere on the canvas.
  //
  // Hover comes from the canvas's ONE HoverHandler; the CELL OUTLINE it draws covers every block,
  // including the empty ones. Clicking is the tabs' job, uniformly -- a cell that guessed which of
  // several things you meant would be choosing behind your back.
}
