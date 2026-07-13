# The Map screen — the complete overhaul

**Status:** design approved by Twilight 2026-07-12 (chassis, layer grouping, edit scope, phased build).
Nothing built yet. This file is the single source of truth for the redesign; update it as the build
lands.

> **Read first:** [`../reference/ui-patterns.md`](../reference/ui-patterns.md) (how this app builds UI),
> [`../reference/tiles.md`](../reference/tiles.md), [`../reference/palettes.md`](../reference/palettes.md),
> [`../reference/sprites.md`](../reference/sprites.md),
> [`../reference/map-connections.md`](../reference/map-connections.md),
> [`../reference/gen1-knowledge.md`](../reference/gen1-knowledge.md).

---

## 1. Why

The map emulator grew organs fast — blocks, tiles, meaning chips, palettes, the player, connections,
music — and every one of them was bolted onto the same screen. The result is what Twilight called it:
**really bad UX**. Concretely:

- **Three bars of unrelated chrome.** An info row that holds the map name *and* a hand-rolled contrast
  stepper *and* three panel toggles; a wrapping chip bar; a footer that is half legend, half zoom.
- **Panels stack sideways and evict each other.** `openPanels` is a queue with an eviction rule — open a
  fourth and the oldest silently closes. That is a workaround for a layout that was never designed to
  hold this much.
- **The layers aren't a layer system.** Nine semantic overlays live as chips; the *guides* (grid, map
  bounds), the *game-view rings* (the red screen box, the accent draw area) and the *player* are
  hard-coded rectangles with no toggles at all — you cannot turn off the red box that sits over the map.
- **Nothing is editable on the canvas.** Warps, signs and NPCs — the things a person actually wants to
  move — are not drawn, not selectable, not draggable. Most of the Area block of the save has **no UI at
  all**.
- **No selection model.** One block can be "selected"; nothing else can.

The fix is not another chip. It is the chassis a real editor has.

---

## 2. The research — Tiled, Photoshop, Aseprite

### What we take

| From | Pattern | Why it fits |
|---|---|---|
| **Photoshop** | **Options bar** — a bar above the canvas whose contents change with the active tool. | Kills the "everything on one bar forever" problem. The bar only ever shows what the current tool/selection needs. |
| **Photoshop** | **Contextual task bar** — an on-canvas bar offering the few actions that make sense for what's selected. | We adopt the *idea* (context-appropriate actions come to you), not the floating widget: our version is the Options bar + the Inspector. |
| **Photoshop** | **Panels collapse to an ICON DOCK.** A dock rail of icons; clicking one expands its panel; clicking again collapses it back to the icon. | Exactly Twilight's "no clutter, things need to collapse in, panels do not need to stack out". |
| **Photoshop** | **Properties panel is contextual** — it shows the *document's* properties when nothing is selected, and the *selection's* properties when something is. | Gives every one of the ~120 Area fields a natural home without inventing 8 tabs. |
| **Photoshop** | Layers panel: **eye + lock per row**, **groups as folders** whose eye/lock/opacity cascade to children, alt-click eye = **solo**. | The exact ask: grouped layers, each toggleable, group toggles the lot. |
| **Tiled** | **Group Layers** ("work like folders… visibility, opacity, offset and lock of a group layer affects all child layers"). | The model for our 4 groups. |
| **Tiled** | **Object layers** — objects are freely positioned, selectable, individually propertied, and *not* part of the tile grid. | Warps / signs / NPCs / connections are precisely this. The save stores them as objects, so the UI should too. |
| **Tiled** | Layers panel is the **primary navigation** — clicking a layer changes what you're editing. | Selecting the "Warps" layer scopes the Move tool to warps: click-through no longer picks the wrong thing. |
| **Aseprite** | **Context bar** — "shows specific options for the active tool. It also changes depending on the state of the active document (e.g. when the selection is moved)." | The precise behaviour we want, including the *state* half (drag in progress → live X/Y readout). |
| **Aseprite** | **Status bar** carries cursor position, what's under it, and the zoom. | Where the coords/legend/zoom belong — not a footer legend nobody reads twice. |
| **Aseprite** | Tool rail on the left, canvas centre, timeline/dock right — **one job per edge**. | The skeleton. |
| **All three** | **Dark, neutral canvas well** so the art reads; the document floats on it with a shadow. | Makes 4-shades-of-grey Game Boy pixel art legible, and it is what every pixel editor does. |

### What we reject

- **Floating / undockable / draggable panels** (Photoshop, Tiled). Power without payoff: this is one
  screen, not a workstation. Docked, collapsible, one column — no window management.
- **Multi-panel stacks** (Tiled's permanent Layers-over-Properties). Twilight: *panels do not need to
  stack out*. One expanded panel at a time, its icon lit in the rail.
- **Old-fashioned chrome** — menu bars, grey 3-D bevels, tiny 16px system icons, tool*bars* with
  separators. This app is themed, flat, rounded, chip-and-segment based. The *ergonomics* of Photoshop,
  the *look* of this app. Nothing here should look like a 2003 desktop tool.
- **Blend modes / opacity per layer / parallax / tint** (Tiled, Photoshop). Meaningless here — our
  layers are annotations over a fixed console rendering. **One exception:** a single global overlay
  opacity slider (the Meaning group's master), because stacked annotation on 4-grey art genuinely needs it.
- **Painting the map.** ⚠️ **The map's block data is in the ROM, not the save.** There is no block array
  in a `.sav` to write. A brush/stamp/bucket would be a lie. The Blocks panel stays an *inspector*, and
  what the user actually edits are the map's **id, size, pointers, tileset, palette, objects and state** —
  every byte the save really holds.

Sources: [Tiled — Working with Layers](https://doc.mapeditor.org/en/stable/manual/layers/),
[Tiled — Introduction](https://doc.mapeditor.org/en/stable/manual/introduction/),
[Aseprite — Context Bar](https://www.aseprite.org/docs/context-bar/),
[Aseprite — Workspace](https://www.aseprite.org/docs/workspace/),
[Photoshop — Contextual Task Bar](https://helpx.adobe.com/photoshop/desktop/get-started/learn-the-basics/boost-workflows-with-the-contextual-task-bar.html),
[Photoshop — Expand or collapse panel icons](https://helpx.adobe.com/photoshop/desktop/get-started/learn-the-basics/collapse-expand-icons.html),
[Photoshop — Layers panel](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/get-started-layers/work-with-the-layers-panel.html).

---

## 3. The chassis

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│  IDENTITY BAR   Pallet Town ▾ │ Overworld ▾ │ 10×9 blocks │ ⚠ unfinished copy of …    │ 34px
├──┬───────────────────────────────────────────────────────────────────────────┬───┬───┤
│T │  CONTEXT BAR   ⟨changes with tool / selection⟩                            │   │   │ 32px
│O ├───────────────────────────────────────────────────────────────────────────┤ D │ I │
│O │                                                                           │ O │ C │
│L │                                                                           │ C │ O │
│  │                     THE MAP  (dark well, map floats,                      │ K │ N │
│R │                      pans + zooms, objects are                            │   │   │
│A │                      selectable and draggable)                            │ 1 │ R │
│I │                                                                           │ p │ A │
│L │                                                                           │ a │ I │
│  │                                                                           │ n │ L │
│44│                                                                           │ e │44 │
│  │                                                                           │ l │   │
├──┴───────────────────────────────────────────────────────────────────────────┴───┴───┤
│  STATUS BAR   tile 12,7 · block $2A “Grass — wild Pokémon” │ Warp 3 selected │ 3× ⊟⊞ Fit│ 26px
└──────────────────────────────────────────────────────────────────────────────────────┘
```

Four thin bars, each with **one** job, and two rails. Nothing wraps. Nothing evicts anything.

- **Identity bar (34px)** — *what is loaded*. Map picker (all 248 ids, glitch ids included and labelled),
  tileset picker, size chip, the unfinished-copy warning, and a save-dirty dot. Nothing else ever moves in
  here.
- **Tool rail (44px, left)** — the tools (§5). Icon + tooltip + single-key shortcut. Active tool = accent
  fill (the app's `SegBtn` language, vertical).
- **Context bar (32px)** — *the options for the current tool, or the fields for the current selection*.
  Empty-ish when nothing is doing anything (it shows the palette/contrast stepper then — the one control
  that is always relevant to what you're looking at).
- **Canvas well** — dark neutral surround, the map floats on it with a soft shadow, integer zoom, pans.
- **Dock (right)** — an **icon rail (44px) that is always there** + **at most one expanded panel column**
  (280–380px, drag-resizable, width remembered). Click a lit icon to collapse back to nothing. The map
  never goes below `mapMinWidth`; if the window is too narrow for a panel, the panel **overlays** the
  canvas edge (with a scrim) instead of squeezing the map — the eviction queue dies here.
- **Status bar (26px)** — cursor coords (tile *and* block, map-relative *and* buffer), what's under the
  cursor in words, the selection summary, and the zoom control.

**Responsive rule:** below `mapMinWidth + panelMinWidth` the dock panel becomes an overlay drawer; below
that again the tool rail collapses to a single ⋮ tool menu. The map is the screen; it never disappears.

---

## 4. The layer system

A real tree: **4 groups → N layers**, all persisted per-session (a view setting; **it never touches the
save**).

| Group | Layer | Kind | Notes |
|---|---|---|---|
| **Guides** | Block grid | guide | 32px cells. Tinted line (grey vanishes into the art). |
| | Tile grid | guide | 8px. New. Off by default. |
| | Map bounds | guide | Where the map ends and the ring begins. |
| | Border ring | overlay bit `1<<7` | The 3-block ring + the out-of-bounds block filling it. |
| **Meaning** | Walls | overlay bit `1<<0` | The nine semantic overlays MapEngine already renders. |
| | Grass | `1<<1` | Each has its own hue **and** its own 8×8 pattern (they stack and still read). |
| | Water | `1<<2` | |
| | Warp tiles | `1<<3` | (The *tile trait*, not the warp objects.) |
| | Doors | `1<<4` | |
| | Ledges | `1<<5` | With the arrow you jump. |
| | Counters | `1<<6` | |
| | Cut trees | `1<<8` | |
| | Elevation edges | `1<<9` | |
| **Game View** | Player | sprite | Drawn where the console's OAM says (4px above his tile row). |
| | Screen box | rect (red) | The 20×18 tiles the Game Boy is actually showing. **Now a layer.** |
| | Draw area | rect (accent) | The 6×5 blocks `LoadCurrentMapView` redraws. **Now a layer.** |
| **Objects** | Warps | objects | 32 max. Draggable, selectable, addable, deletable. |
| | Signs | objects | 16 max. |
| | NPCs / sprites | objects | 16 max. |
| | Connections | objects | 4 (N/S/E/W), drawn as edge bands on the ring. |
| | Boulder | object | The Strength-push scratch (`boulderIndex` → a sprite slot), when set. |

**Row anatomy** (Photoshop, in this app's clothes):

- **eye** — visibility. **Alt-click = solo** (everything else in the group off; alt-click again restores).
- **name** + a muted **count badge** (`Warps ×12`) or **swatch** (Meaning layers show the exact ink the
  renderer uses — the row *is* the legend, so a legend row can never drift out of sync).
- **lock** (Objects only) — locked = not selectable, not draggable. Prevents fat-fingering an NPC while
  dragging a warp.
- **greyed + "none on this map"** when the layer has nothing to show (kept from today's chips — a chip
  that has nothing to say says so).
- **Group row**: chevron (collapse), **tri-state master eye** (all / some / none — one click toggles the
  whole group), name, total count. The Meaning group's row also carries the single **overlay-opacity
  slider**.
- **Click the layer name** = make it the **active layer**: the Move/Select tool then hit-tests *that*
  layer first. (Tiled's model. This is what makes clicking a warp under an NPC possible without a
  right-click menu.)

**Defaults on open:** Guides = block grid + map bounds on; Meaning = all off (*the map is the point*);
Game View = player on, boxes on; Objects = all on. Same doctrine as today, extended.

---

## 5. Tools (left rail)

| Tool | Key | Does |
|---|---|---|
| **Select / Move** | `V` | Click selects (object, or block if the click lands on none). Drag moves the selected object, snapped to the tile grid. Shift-click = add to selection. Arrow keys nudge 1 tile; Ctrl+arrow = 1 block; Shift+arrow = 8 tiles. |
| **Pan** | `H` / hold `Space` | Drag anywhere. (A drag on empty canvas pans with any tool — the hand is for when the canvas is full of objects.) |
| **Zoom** | `Z` | Click in, Alt-click out. Ctrl+wheel and pinch always work, anchored on the cursor. |
| **Place** | `P` | The context bar picks *what*: Warp / Sign / NPC. Click a tile to create one there (respecting the 32/16/16 caps, which the bar states). |
| **Eyedropper** | `I` | Pick the block/tile under the cursor into the Blocks panel (and, when a tile field like *grass* or *counter* is focused, straight into that field — how you set the grass tile without knowing that grass is `$52`). |
| **Measure** | `M` | Drag: Δx, Δy in tiles/blocks, live in the context bar. For lining warps up with their partner. |

**Player is not a tool** — he is an object on the Game View layer, moved with Select/Move like anything
else.

---

## 6. The context bar, by state

| State | The bar holds |
|---|---|
| Nothing selected, Move tool | **Palette / contrast** stepper (`0–9`, name, glitch warning in `errorColor`) — the one thing always worth having; plus **Snap: [Tile ▾ Block ▾ Free]**. |
| Object selected | Its **X / Y** as live numeric fields (tile coords, typed or stepped), the object's headline field (a warp's destination, a sign's text id, an NPC's picture), **lock**, **duplicate**, **delete**. This is the fast lane; the full field set is in the Inspector. |
| Multiple selected | Count, **align** (left/top), **distribute**, **delete all**. |
| Dragging | Live X/Y and the Δ, exactly like Aseprite's moving-selection bar. |
| Place tool | **[Warp | Sign | NPC]** segmented, plus that type's defaults; and "3 of 32 used". |
| Zoom tool | `1× 2× 3× 4× 6× 8×` segments · **Fit** · **100%**. |
| Measure | Δx, Δy, distance in tiles and blocks. |

---

## 7. The dock panels — and where **every** Area field lives

The rule: **every byte the save holds for the area has exactly one home, and hack/glitch values are shown
and editable, never silently rewritten.**

### Inspector (contextual — the Photoshop Properties panel)

| Selection | Groups shown |
|---|---|
| **(nothing)** → *the map itself* | **Identity**: `curMap` · **Size**: `width`, `height`, `width2x2`, `height2x2` · **The world's edge**: `outOfBoundsBlock` · **Where the map lives (ROM)**: `dataPtr`, `txtPtr`, `scriptPtr`, `curMapScript` · **Engine scratch**: `currentTileBlockMapViewPointer` (with a live "matches the player" check + a one-click **Sync** — see §9), `mapViewVRAMPointer` · **Odds and ends**: `cardKeyDoorX/Y`, `forceBikeRide`, `blackoutDest`, `curMapNextFrame` · **Palette**: `AreaGeneral.contrast` (the only AreaGeneral field this screen owns — `noLetterDelay`/`countPlaytime` stay on the trainer card). |
| **Player** | **Position**: `xCoord`, `yCoord`, `xBlockCoord`, `yBlockCoord` · **Facing**: `playerCurDir`, `playerMoveDir`, `playerLastStopDir` · **Movement**: `walkBikeSurf`, `spinPlayer`, `finalLedgeJumping`, `playerJumpingYScrnCoords` · **Where he's standing**: `standingOnDoor`, `movingThroughDoor`, `standingOnWarp` · **What he may do here**: `surfingAllowed`, `flyOutofBattle`, `strengthOutsideBattle`, `noBattles` · **Battle state**: `isBattle`, `isTrainerBattle`, `battleEndedOrBlackout` · **Scratch**: `xOffsetSinceLastSpecialWarp`, `yOffsetSinceLastSpecialWarp`, `usedCardKey`, `usingLinkCable`. |
| **Warp** *(WarpData)* | `x`, `y`, `destMap` (a real map picker — all 248, glitch ids named), `destWarp`. Plus a **"where it goes"** line that resolves the destination map's name and, when the target warp exists, says so. |
| **Sign** *(SignData)* | `x`, `y`, `txtId`. |
| **NPC** *(SpriteData — 20 fields)* | **Who**: `pictureID` (with the sprite drawn), `imageIndex`, `imageBaseOffset` · **Where**: `mapX`, `mapY`, `xDisp`, `yDisp`, `xPixels`, `yPixels` · **Facing & movement**: `faceDir`, `movementStatus`, `movementByte`, `movementDelay`, `rangeDirByte`, `xStepVector`, `yStepVector` · **What it is**: `textID`, `trainerClassOrItemID`, `trainerSetID`, `missableIndex`, `grassPriority` · **Animation scratch**: `intraAnimationFrameCounter`, `animFrameCounter`, `walkAnimationCounter`. |
| **Connection** *(MapConnData)* | direction · `mapPtr`, `stripSrc`, `stripDst`, `stripWidth`, `width`, `yAlign`, `xAlign`, `viewPtr` — each shown against **what the game's macro computes**, with a **Recompute** action. ⚠️ `MapDBEntryConnect::stripSize()` is wrong and `maps.json`'s `flag` patches it; the panel must compute from the macro (as `MapEngine` already does), never from `stripSize()`. See [`../reference/map-connections.md`](../reference/map-connections.md). |

### Blocks panel
The block/tile inspector we already have (16 tiles, each labelled with what it *does*; hover a tile → it
lights on the map), plus the block picker and the out-of-bounds-block picker. Selecting a block on the
canvas auto-focuses this panel.

### Tileset panel
`current` (picker) · `type` → **tile animation** (Indoor / Cave / Outdoor — *not* a place, it's which tiles
move; tri-state, never a bool) · `grassTile` · `talkingOverTiles[3]` (counters) · `boulderIndex`,
`boulderColl` (Strength scratch) · **Advanced (collapsed)**: `bank`, `blockPtr`, `gfxPtr`, `collPtr`, each
diffed against the cartridge with a **Restore** action. (Largely exists today — it keeps its content and
gets the new chrome.)

### Encounters panel *(AreaPokemon — no UI today)*
`grassRate` · 10 grass slots (species + level, drag-to-reorder) · `waterRate` · 10 water slots ·
`pauseMons3Steps`. Rate 0 = "no wild Pokémon here", said in words. Ties into the Grass/Water Meaning
layers: turning the panel on offers to light them up.

### Music panel
`musicID`, `musicBank` (2 / 8 / 31 only — **a bad bank hangs a real console**; a save holding one is
*shown*, never rewritten), `noAudioFadeout`, `preventMusicChange`, the track list with hover-preview.
Exists; keeps its content.

### Area State panel *(the flags with no other home — no UI today)*
- **NPC state** *(AreaNPC)*: `npcsFaceAway`, `scriptedNPCMovement`, `npcSpriteMovement`,
  `tradeCenterSpritesFaced`, `ignoreJoypad`, `joypadSimulation`, `runningTestBattle`, `trainerWantsBattle`,
  `trainerHeaderPtr`.
- **Warp state** *(AreaWarps' non-list fields)*: `scriptedWarp`, `isDungeonWarp`, `skipJoypadCheckWarps`,
  `warpDest`, `dungeonWarpDestMap`, `specialWarpDestMap`, `flyOrDungeonWarp`, `flyWarp`, `dungeonWarp`,
  `whichDungeonWarp`, `warpedFromWarp`, `warpedfromMap`.
- **Loaded sprite set** *(AreaLoadedSprites)*: `loadedSetId` + the 11 fixed slots (`lSpriteAt` /
  `lSpriteSwap`).

Each is a titled group; each field carries a one-line plain-English "what this does" (the app is where
that gets answered — nobody should have to already know what a *counter* is).

### Layers panel
§4.

---

## 8. Editing on the canvas

- **Objects are drawn as chips on the map**: a small rounded badge at the object's tile, coloured by
  layer, carrying a glyph (⇄ warp, 🪧 sign, the NPC's actual sprite for an NPC). At low zoom they shrink to
  a dot; they never obscure the tile they mark.
- **Select**: click. **Marquee**: drag on empty canvas with the Select tool + Shift. **Cycle** overlapping
  objects with repeated clicks (Alt+click cycles down the stack).
- **Drag**: snapped to the **tile** grid (objects are tile-addressed in the save). The drag ghost shows the
  target tile; the context bar shows live X/Y/Δ. Release commits **exactly two bytes** (x, y).
- **Precision**: type the coords in the context bar or the Inspector. Arrow keys nudge. Everything is
  reachable without a mouse.
- **Add / delete**: Place tool, or the panel's "+" row; `Del` deletes the selection. The 32/16/16 caps are
  stated *before* you hit them, not after.
- **Undo**: ⚠️ **there is no undo stack in this app today**. Every canvas edit is therefore a *deliberate,
  visible* mutation with the value shown before and after. A real undo/redo stack for the Area block is
  **out of scope here and worth its own plan** — flagged in §12.

---

## 8b. The map is ALIVE — animation & simulation

**Twilight, 2026-07-12: "map needs to animate and simulate right."** A still frame is not what the console
puts on the screen, and a map editor that shows dead water is showing you something the game never shows
anyone. This is a first-class part of the overhaul, not a garnish — and, like everything else here, it is
an **emulation, not an impression**.

Full domain write-up (read it before building this):
[`../reference/map-animation.md`](../reference/map-animation.md) — verified against the disassembly.

**The truth, in one paragraph.** One byte in the save (`0x3522`, the tileset's animation byte) decides
whether anything moves. `UpdateMovingBgTiles` runs **every frame**: it does nothing for 19 frames, then on
frame 20 it **bit-rotates the 16 bytes of water tile `$14`** — right for four steps, left for four — and,
on a water+flower tileset, on frame 21 it copies **one of three flower tiles** into tile `$03`, choosing by
the *water's* own phase counter. Cycle: **20 frames (water) or 21 (water + flower)** — about three ticks a
second. Water has no frames, it has a **rotation**. The flower's three frames run `1,1,2,3`. Hack values
aren't chaos: **odd → behaves as water-only, even non-zero → behaves as water+flower** (the code tests bit
0), and **0 breaks Surf** — which the app should *say*, in words, to whoever sets it.

What gets built:

- **A frame clock** at the DMG's rate (59.7275 Hz), driving the canvas. `MapEngine::render` takes an
  explicit **frame number**; nothing reads a wall clock. Everything below is a **view** concern —
  ⚠️ **the animation never writes a byte of the save.**
- **Tile animation, exactly as above** — the rotation (not a frame-swap), the 20/21 cadence, the three
  flower frames, the coupled counters, the hack-value behaviour, and the "Indoor on a water map breaks
  Surf" warning.
- **Sprite animation** — the player and the NPCs. The traps are already documented and all three are
  counter-intuitive: **there is no right-facing sprite** (it is left, mirrored), **there is no second
  walking frame** (it is the same frame, flipped), and a sprite sits **4 px above** its tile row.
- **The palette, live** — the map is already drawn *through* whatever `rBGP` the contrast byte produces;
  the fade between contrast levels gets animated the way `LoadGBPal` steps it, so a glitch palette is seen
  arriving, not just sitting there.
- **A transport, in the status bar** — ▶/⏸, **step one frame**, speed (½× · 1× · 2×), and a live frame
  counter. **Paused = frame 0**, always, deterministically.
- 🔴 **Determinism is a hard requirement.** The headless `screenshooter`, `tst_visual_regression` and the
  `--shot` harness render **frame 0** unless told otherwise. A moving map must never make a test flap. The
  frame number is an input, never an ambient global.
- **The console is the judge, again.** The animated tiles live in **VRAM**, so `tst_emu_parity` boots the
  real cartridge, dumps tiles `$14` and `$03` **every frame**, and demands ours match byte-for-byte —
  exactly the doctrine that found three real bugs in the sound engine. If we can't match it, we don't ship
  it.

**And the "simulate" half — walking the map.** With the collision data derived (it is: `collPtr` → the
ROM's list of *passable* tiles), the ledges known, and the warps in hand, the editor can let you **drive
the player around the map with the arrow keys** — real collision, real ledge jumps, the screen box and the
draw area following him exactly as `LoadCurrentMapView` moves them, and a warp tile offering to take you to
where it goes. That is the map emulator finally *being* one. It is scoped as its own phase (**Phase 10**) with
its own question in §14, because it is the one feature here that is a small game engine rather than a
screen.

## 9. The doctrine (non-negotiable, and it is why this is not just a re-skin)

1. **Byte fidelity.** An edit writes *only* the bytes it names. Dragging a warp writes `x` and `y`.
   Nothing else in the save moves — not a checksum region we weren't told to touch, not a "normalised"
   neighbouring field, not an unused bit. See `../context/principles.md` → *Save File Integrity Is Sacred*.
2. **Derived bytes are SHOWN, never silently synced.** `currentTileBlockMapViewPointer` is a function of
   the player's coords and the map width — the game recomputes it, and `tst_map` proves we can too. If the
   user moves the player, the stored pointer no longer matches. We **show the mismatch** ("the save's view
   pointer is for 5,7 — the player is at 12,9") and offer a **one-click Sync**. We do *not* quietly rewrite
   it, and we do *not* leave the user to discover it. Same rule as the tileset pointers and the music bank.
3. **Hack values are first-class.** Every field accepts its full byte range. Out-of-range/glitch values are
   flagged (`errorColor` + a plain-English "what a real console does with this"), and the *dangerous* ones
   (the music bank; a tileset `collPtr` that isn't the cartridge's) are called out — but the editor never
   refuses the value and never rewrites it behind your back.
4. **The map is the point.** Meaning layers off by default; the canvas is never covered by chrome.
5. **No hacks in the code either.** Proper layouts, no pixel offsets, no eviction queues.

---

## 10. What has to be built in C++

The QML cannot do this alone, and two of these are blockers:

1. 🔴 **Un-opaque the Area children.** `area.h` declares `Q_DECLARE_OPAQUE_POINTER` on `AreaMap*`,
   `AreaPlayer*`, `AreaTileset*`, `AreaWarps*`, `AreaSign*`, `AreaSprites*`, `AreaPokemon*`, `AreaNPC*`,
   `AreaLoadedSprites*` — so QML reads `area.map.*` as **`undefined`** (the exact bug that bit
   `AreaAudio` and was fixed on 2026-07-12 by `#include`-ing the header instead). **Every field in §7 is
   unreachable from QML until this is fixed.** Fix: full `#include` + drop the opaque declaration, exactly
   as `AreaGeneral`/`AreaAudio` now are. (`reference/qt-patterns.md`.)
2. 🔴 **Defuse the `MapsDB` deep-link landmine.** `getToMap()`/`getToSprite()`/`getToMusic()` are never
   resolved (`DB::deepLinkAll()` doesn't call `MapsDB::inst()->deepLink()`), which is why `AreaAudio::setTo`
   has been writing `0/0` and why map randomize is still commented out. **"Place the player on any map"
   dereferences those links** — it must be defused before that feature ships. Confirmed safe once called
   (`tst_sprite_data`: all 918 sprites resolve). See `status.md` → Open issues.
3. **`MapLayersModel`** (`brg.mapLayers`) — the tree, flattened to a list model (`depth`, `isGroup`,
   `visible`, `locked`, `soloable`, `colour`, `count`, `applies`, `active`). QML `TreeView` is not worth its
   weight for a fixed 4-group tree; a flat list with a `depth` role is cleaner and testable. Owns
   visibility/lock/solo/active-layer state. **Touches nothing in the save.**
4. **`MapObjectsModel`** (`brg.mapObjects`) — one list over the warps + signs + sprites + connections +
   boulder: `{ kind, index, x, y, label, colour, locked, selected }`. Drives the canvas chips, the hit
   test (`objectAtTile(x, y, activeLayer)`), the drag commit (`moveObject(kind, index, x, y)` — writes the
   two bytes and nothing else), add/delete, and the Inspector's target.
5. **`MapModel` extensions** — selection becomes a *kind + index* (`SelBlock` / `SelObject` / `SelPlayer` /
   `SelNone`) rather than a block-only pair; `viewPointerMatches` + `syncViewPointer()`; `tileAt(px,py)`
   for the status bar; the map picker's list (all 248 ids, glitch labelled).
6. **Rendering** stays where it is: `MapEngine` renders the map + the overlay as **images** (Route 17 is
   20,000+ tiles; per-tile QML delegates would crawl). Object chips are QML items — there are at most ~65
   of them, so that is fine.
7. **`MapEngine::render(frame)` + a `MapClock`** (§8b) — the animation is an *input*, never an ambient
   clock: the renderer is a pure function of (save, layers, **frame**). That is what keeps the screenshot
   and visual-regression suites deterministic, and what lets `tst_emu_parity` compare us to the console
   frame by frame. The clock itself is ~30 lines of QML-facing C++ (`brg.mapClock`: `playing`, `speed`,
   `frame`, `step()`), and it **writes nothing to the save**.
8. **`MapWalker`** (Phase 10) — the walk simulation: the passable-tile list, the elevation pairs, the ledge
   directions, the step cadence, and the camera geometry we already reproduce. Preview-only until committed.

Everything else is QML.

---

## 11. Files

```
screens/non-modal/Map.qml                  the chassis (bars, rails, wiring) — nothing else
screens/non-modal/map/
  MapIdentityBar.qml      MapContextBar.qml     MapStatusBar.qml
  MapToolRail.qml         MapDock.qml           MapCanvas.qml
  canvas/  MapObjectChip.qml  MapGuides.qml  MapGameView.qml  MapSelection.qml
  panels/  LayersPanel.qml    InspectorPanel.qml  BlocksPanel.qml  TilesetPanel.qml
           EncountersPanel.qml  MusicPanel.qml    AreaStatePanel.qml
  parts/   LayerRow.qml  LayerGroupRow.qml  FieldRow.qml  NumField.qml  HexField.qml
           FlagRow.qml   FieldGroup.qml     TileField.qml  MapField.qml
```
`BlockPick.qml` / `TilePick.qml` survive (they are good). `LayerChips.qml` dies with the chip bar.
**Every new `.qml` must be added to `app/app.qrc`** or it fails at runtime.

---

## 12. The programme — thirteen phases, plus one optional

> **The bar (Twilight, 2026-07-12, mandatory):** *"You absolutely have to put in the long work for each of
> these components and pieces."* This is not a re-skin sprint. **Each phase is a full pass on one body of
> work** — designed, built, screenshot-reviewed, tested, documented, and *finished* — before the next one
> starts. No phase is "roughed in and cleaned up later"; there is no later. A phase that is 90% done is
> not done, and the phase after it does not begin.

**Every phase, without exception, ends with all seven of these:**

1. Builds clean (kit dir) and the **affected tests** are green; the **full `ctest`** before any release.
2. `tst_qml_screens` green (it is the only thing that catches a screen that won't open).
3. **The mandatory screenshot review, actually performed** — capture the screen, *look at it*, crop into
   the changed area, hunt overlaps / clipping / cramping / misalignment, and fix what's found. A glance is
   not a review.
4. **Opened in front of Twilight**, on the right screen, with a save loaded (`--sav … --screen map …`) —
   the moment it is worth her time.
5. New knowledge written into the **notes** (this file, `ui-patterns.md`, `qt-patterns.md`,
   `decisions/*`, the session log) — in the same commit.
6. **Credits** checked (did this phase bring in a new source, tool, asset or helper?).
7. A focused commit on `dev` with its **changelog entry inline**, pushed. `main` waits for "ship".

---

### Phase 0 — Unblock the bridge  *(no visible change, and nothing else can happen first)*

The entire §7 field list is **unreachable from QML today**. Two C++ landmines, both already diagnosed:

- **0a — Un-opaque the Area children.** `area.h` `Q_DECLARE_OPAQUE_POINTER`s nine of its eleven children,
  so `area.map.*`, `area.player.*`, `area.warps.*`… all read as `undefined` in QML. Fix each the way
  `AreaAudio` was fixed on 2026-07-12: full `#include`, drop the opaque declaration. Do it **one type at a
  time**, each verified from QML, because this is the bug class that cost the project weeks.
- **0b — Defuse the `MapsDB` deep-link landmine.** `DB::deepLinkAll()` never calls
  `MapsDB::inst()->deepLink()`, so `getToMap()` / `getToSprite()` / `getToMusic()` are null for every map.
  Add it, guard it, and let `tst_area::audio_setTo_keepsIdAndBankApart` (which is currently passing over a
  dormant bug) start doing real work.
- **0c — Regression net.** `tst_area` cases proving every Area child is QML-readable (a `QQmlEngine`
  round-trip, the same shape `tst_qml_screens` uses) and that the deep link resolves for all 248 maps.

**Exit:** QML can read and write every field in §7. Proven by test, not by eye.

---

### Phase 1 — The chassis

The four bars, the two rails, the well. **Structure only** — existing content is re-homed *unchanged* so
that nothing regresses while the frame goes up.

- **1a — The skeleton.** `Map.qml` becomes a chassis and nothing else: identity bar, tool rail, context bar,
  canvas well, dock, status bar. The `openPanels` eviction queue and the wrapping chip bar are **deleted**.
- **1b — The dock.** Icon rail (always present) + at most one expanded panel column. Expand/collapse
  animation, drag-to-resize, remembered width. The **narrow-window rule**: below the floor the panel becomes
  an **overlay drawer with a scrim** (the hand-rolled sliding `Rectangle` pattern —
  `ui-patterns.md` → "View All overview drawer"; **not** a Material `Drawer`, it never sits flush).
- **1c — The canvas well.** Dark neutral surround, the map floating with a soft shadow; zoom/pan carried
  over intact (integer zoom, cursor-anchored, both wheel axes).
- **1d — The status bar.** Live cursor coords (tile + block, map-relative + buffer), what's under the
  cursor in words, zoom control. The footer legend dies here — the Layers panel is the legend now.
- **1e — Re-home.** Tileset / Blocks / Music panels move into the dock verbatim. Contrast moves into the
  context bar. Nothing is redesigned yet; nothing is lost.

**Exit:** the screen looks and behaves like an editor, every existing feature still works, and the
screenshot review is clean at 750×480 *and* maximised.

---

### Phase 2 — The layer system

The heart of the ask. **`MapLayersModel` first, UI second.**

- **2a — The model.** The 4 groups × N layers, flattened (`depth`/`isGroup`/`visible`/`locked`/`colour`/
  `count`/`applies`/`active`). Tri-state group eye. Solo. Active layer. **It never writes the save** — and a
  test says so.
- **2b — The panel.** Rows, group rows, chevrons, eyes, locks, count badges, swatches, the
  "nothing on this map" state, the Meaning group's overlay-opacity slider.
- **2c — Guides become layers.** Block grid, tile grid (new), map bounds, border ring — each independently
  toggleable, each drawn from `brg.map` numbers only.
- **2d — Game View becomes layers.** The **red screen box** and the **accent draw area** — the two rings
  around the player Twilight named — and the **player sprite** itself, each its own toggleable layer inside
  the group, the group toggling all three.
- **2e — Meaning re-homed.** The nine semantic overlays move from chips to layer rows, swatch and all,
  keeping every behaviour (off by default; a layer with nothing to show says so).
- **2f — Wiring.** Active layer scopes hit-testing (phase 3 will consume it); alt-click solo; group
  collapse persists.

**Exit:** every drawable thing on the map can be turned on and off, alone or by group, and the map itself
is never obscured by something you can't dismiss.

---

### Phase 3 — The map is ALIVE (animation)

The spec is [`../reference/map-animation.md`](../reference/map-animation.md), verified against the
disassembly; §8b is the summary. **This is an emulation of `UpdateMovingBgTiles`, not a shimmer effect.**

- **3a — The frame clock.** `MapEngine::render(frame)` — an explicit frame number, never a wall clock.
  A `MapClock` at 59.7275 Hz drives it; **paused = frame 0**, deterministically. It writes nothing to the save.
- **3b — Water.** The 16 bytes of tile `$14`, **bit-rotated** one pixel per step (right ×4, left ×4, off
  `counter2 & 4`), on the 20-frame cadence. Not a frame swap — a rotation.
- **3c — Flowers.** The three flower tiles into tile `$03` on frame 21, chosen by the water's own phase
  counter (`1,1,2,3`), on the 21-frame cadence — and only on a water+flower tileset.
- **3d — The byte, honestly.** The tri-state drives it from the save; **hack values behave the way the
  console behaves** (odd → water-only, even non-zero → water+flower), and **Indoor on a water map breaks
  Surf** — said in words, on the control.
- **3e — Sprites.** The player and NPCs animate: facing (**no right-facing art — it's left, mirrored**),
  the walk cycle (**no second frame — it's the same one, flipped**), the 4px lift.
- **3f — The palette, live.** The contrast fade animated the way `LoadGBPal` steps it.
- **3g — The transport.** ▶/⏸ · step-one-frame · ½× 1× 2× · frame counter, in the status bar.
- **3h — Determinism.** `screenshooter`, `tst_visual_regression` and `--shot` all render frame 0. **A
  moving map must never make a test flap** — this is a release blocker, not a nicety.
- **3i — The console judges it.** `tst_emu_parity` dumps VRAM tiles `$14` and `$03` from the real cartridge
  **every frame** and demands a byte-for-byte match. Negative-control it (break the cadence by one frame and
  watch it fail by name), exactly as the sound engine was.

**Exit:** the water moves, the flowers turn, the people face the right way, and the Game Boy agrees with
every frame of it.

---

### Phase 4 — Objects on the canvas

- **4a — `MapObjectsModel`.** One list over warps + signs + NPCs + connections + boulder. Hit-testing
  (`objectAtTile`, active-layer-first, then z-order), caps, add, delete, and **`moveObject` that writes
  exactly two bytes** — pinned by a whole-save byte-diff test.
- **4b — The chips.** Object badges on the map: layer-coloured, glyphed, the NPC drawn as its **actual
  sprite**, shrinking to a dot at 1× so they never bury the tile they mark.
- **4c — Selection.** Click, shift-click, marquee, alt-click to cycle a stack. A selection you can lose
  under a layer is not a selection — it draws above everything.
- **4d — Drag.** Tile-snapped, live ghost, live X/Y/Δ in the context bar, commit on release. Locked layers
  refuse the grab. `Esc` cancels a drag with **nothing written**.
- **4e — Add / delete.** The Place tool and the panel "+" rows; `Del`; the caps stated before you hit them.

**Exit:** a warp, a sign, an NPC and the player can each be picked up and put somewhere else, and the save
changed by exactly the bytes that describes.

---

### Phase 5 — The Inspector

The single largest content phase: **~120 fields**, each with a home, a control appropriate to its type, and
a sentence explaining what it does.

- **5a — The shell + the field kit.** `FieldRow`, `NumField` (byte/word, dec+hex, full range),
  `HexField`, `FlagRow`, `TileField` (+eyedropper), `MapField` (all 248, glitch ids named), `FieldGroup`
  (a titled, collapsible group — the trainer-card `PlaytimeGroup` language). Build the kit **properly
  once**; every panel after this is assembled from it.
- **5b — Map properties** (nothing selected) — identity, size, the world's edge, the ROM pointers, the
  engine scratch, the odds and ends, the palette.
- **5c — The view-pointer truth-teller.** Live mismatch detection + one-click **Sync** (§9.2). This is the
  phase's keystone: it is the pattern for every derived byte in the app.
- **5d — Warp / Sign** properties, incl. the destination resolver ("→ Viridian City, warp 2").
- **5e — NPC** properties — all 20 fields, grouped Who / Where / Facing & movement / What it is /
  Animation scratch, with the sprite drawn and every enum named.
- **5f — Connection** properties — the eight fields against **what the game's macro computes**, with
  Recompute. (Compute from the macro, never from the broken `stripSize()`.)
- **5g — Player** properties — position, facing, movement, standing-on, what he may do here, battle state,
  scratch.

**Exit:** there is no byte in the Area block that a person can see in a hex editor and cannot see here,
in words, with its range, and change.

---

### Phase 6 — Encounters

`AreaPokemon` has **no UI at all** today. `grassRate` · 10 grass slots · `waterRate` · 10 water slots ·
`pauseMons3Steps`. Species picker + level, drag-to-reorder (the Bag/Moves drag pattern), rate 0 said in
words ("no wild Pokémon here"), and a link to the Grass/Water Meaning layers so the panel and the map agree.

---

### Phase 7 — Area State

`AreaNPC` (9 flags), the `AreaWarps` state fields (12), `AreaLoadedSprites` (`loadedSetId` + 11 slots) —
none of which has a UI today. Three titled groups, every flag explained, the sprite-set slots reorderable.

---

### Phase 8 — Tileset & Blocks, properly

The two panels that already exist get the *deep* pass rather than the re-chrome they got in phase 1:
the tri-state tile animation with its "what this actually does", the grass tile and the three counter slots
via `TileField` + eyedropper, the Strength scratch, and the **Advanced** disclosure (bank / blockPtr /
gfxPtr / collPtr diffed against the cartridge, with Restore). Blocks keeps its click-to-inspect and its
16-tile breakdown, gains the out-of-bounds-block picker, and loses nothing.

---

### Phase 9 — Tools & precision

Place · Eyedropper · Measure · snap modes · nudge keys (1 tile / 1 block / 8 tiles) · align + distribute for
a multi-selection · every action reachable from the keyboard · the shortcut map written down. **A tool is
not done until it has a cursor, a context bar, an empty state and a keyboard path.**

---

### Phase 10 — Motion & polish

Panel slide, chip hover, selection pulse, layer-toggle cross-fade (the overlay *arrives*, it doesn't slam
on), empty states, tooltips on everything that has a name nobody should be expected to know. Then the full
screenshot pass at three window sizes, and the accessibility read (colour is never the only signal — every
layer has a *pattern* as well as a hue, which is already true of the overlay and must stay true of the chips).

---

### Phase 11 — The verification pass

- Full `ctest` (incl. `tst_emu_parity` against the real cartridge).
- **A byte-diff harness run over every edit the screen can make**: load a save, perform the edit, diff the
  whole 32 KB. Anything that changed and shouldn't have is a **release blocker**, not a bug report.
- Linux Docker (standard / asan / xvfb / coverage).
- A live pass with Twilight on the built app.

---

### Phase 12 — The notes pass

`status.md`, `ui-patterns.md` (the new chassis, dock, layer-row and field-kit conventions),
`qt-patterns.md` (whatever Qt tried to do to us), `decisions/architecture.md` (the layer/object models),
`decisions/rejected.md` (whatever we tried that failed), this file (marked *built*), the session log,
`_nav.dox`, `VERSION`, and the credits check.

---

### Phase 13 — SIMULATE: walk the map  *(**OPTIONAL / stretch** — runs last, gates nothing)*

**Twilight, 2026-07-12: "an accurate simulation like a play/pause button on the map might be cool but it's
not a high priority unless you think it's important."** Here is the honest split, because the two halves of
"animate and simulate" are *not* the same size:

- **The animation (Phase 3) IS important, and it is not optional.** The whole project's claim is
  *emulation, not impression* — and a map with dead water is an impression. The loop is twenty lines of
  logic we can read straight out of the cartridge, the console can check every frame of it, and it is
  wrong *today*. It stays where it is, early, mandatory.
- **The ⏸ transport is nearly free, and we need it anyway** — the screenshot and visual-regression suites
  require a frozen frame 0, so the pause *already has to exist*. Exposing it as a ▶/⏸ costs a button.
- **Walking the map is the cool one, and it is genuinely optional.** It is a small game engine — collision,
  step cadence, ledges, camera, warps. Nothing else in the plan depends on it, so it sits **last**, and it
  only gets built when everything above it is done, or when Twilight asks for it.

If/when it is built:

- **13a — Collision, for real.** The passable-tile list is already derived from the tileset's `collPtr`
  (the ROM holds a list of what you *can* walk on — "wall" is what's left; see
  [`../reference/tiles.md`](../reference/tiles.md)), plus the elevation-edge pairs.
- **13b — The step.** Arrow keys move him a tile at a time, at the game's own cadence, and he **turns
  before he steps** (the game turns first — get that wrong and it feels wrong immediately).
- **13c — The camera IS the console's.** The screen box and the draw area follow him exactly the way
  `LoadCurrentMapView` slides them — half-block steps, block-aligned scratch. We already reproduce that
  geometry byte-for-byte; now it *moves*.
- **13d — Ledges, water, doors.** A ledge jumps the way its arrow points; water needs Surf; a warp tile
  lights up and **offers** to take you where it goes — it never warps you unasked, because that changes
  `curMap`, and that is a real edit.
- **13e — The save, honestly.** Walking is a **preview** — it moves the player in the model, and the status
  bar says plainly whether what you're looking at is what's stored. Committing writes `xCoord`/`yCoord`
  (and offers the view-pointer Sync, §9.2) and **nothing else**. `Esc` puts him back.

⚠️ **Scope discipline:** a *walk* simulation. No NPC AI, no scripts, no battles, no text boxes. If it
starts growing those, it has become a different project and it gets its own plan.

---

**Out of scope, and each worth its own plan:** **undo/redo for the Area block** (§8 — the app has no undo
stack at all today, and inventing one under a save editor is a design job of its own), **"place the player
on any map"** (needs phase 0b, then a flow of its own), **map randomize** re-enablement, and **NPC/grass
priority rendering** on the map.

---

## 13. Testing

- `tst_map` keeps its keystone (`viewPointer_matchesWhatTheGameStored`) and gains: `syncViewPointer` writes
  **only** that field; `moveObject` writes **only** x/y (a byte-diff of the whole save before/after — the
  strongest possible statement of the fidelity rule, and cheap).
- `tst_map_layers` — the group/solo/tri-state logic, and that layers never write the save.
- `tst_map_objects` — hit-testing, caps, add/delete/move, and the object↔save round trip.
- `tst_qml_screens` — **must** be green before any merge; every new `.qml` goes through it.
- `tst_emu_parity` — unchanged and still the oracle: whatever we let the user edit, the console's own RAM
  is what says whether our model of it is right.

---

## 14. Open questions for Twilight

1. **The dark canvas well** — Photoshop/Aseprite/Tiled all surround the artwork with a dark neutral so the
   art reads. This app is light-themed. Proposed: a *theme-derived* dark neutral behind the map only (the
   well), everything else stays light. Say the word if you'd rather the well stay light.
2. **Sub-tile precision.** A drag snaps to the tile grid and writes only `x`/`y`. An NPC also has
   `xPixels`/`yPixels` (sub-tile scratch) — proposed: **numeric-only** in the Inspector, never dragged, so a
   drag can never write four bytes when it promised two. Agree?
3. **The view pointer** (§9.2) — show-and-offer-Sync, rather than auto-sync. Confirm.
4. **`stripSize()`** is still wrong in the DB and `maps.json`'s `flag` patches it. Fixing the DB is your
   call (curated data + a public API). The panel will compute from the macro either way.
5. **Does the map animate on open, or on ▶?** Proposed: **it animates on open** (a dead map is a lie about
   what the game shows), with ⏸ in the status bar, and **always frozen at frame 0** for screenshots/tests.
6. **How far does "simulate" go?** Answered 2026-07-12: **the animation is mandatory** (it is a correctness
   bug today — the water is dead and the console's water is not), **the ▶/⏸ transport is nearly free** (the
   screenshot/regression suites need a frozen frame 0 regardless), and **walking the map is an optional
   stretch** — Phase 13, last, gating nothing. It is deliberately **not** an emulator: no NPC AI, no
   scripts, no battles, no text.
