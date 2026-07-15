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

- **Top bar (36px)** — **the tools, then what is loaded, then the palette** (revised 2026-07-13, Twilight):
  `[ ↖ ✥ ⌕ ] │ [ Pallet Town · Overworld ⌄ ] [ 100% ⌄ ] [ 10×9 ] [ ⚠ unfinished copy ]`.
  - **The tools live here, not in a left rail.** A rail was 44px of chrome down the full height of the
    screen to hold three glyphs; the left edge went back to the map.
  - **One picker, three pointers** (`MapPicker.qml`): the **map id** (all 248, glitch ids labelled with
    what they're a copy of), the **tileset** the graphics come from — carrying **Indoor / Cave / Outdoor**,
    which is not a place but which tiles *move* — and the **blockset** the map is *built* from. The save
    keeps `gfxPtr` and `blockPtr` as **two separate pointers** and a console obeys both, so they get two
    separate controls, and a save that disagrees with itself is **shown** doing so.
  - Picking a map writes **one byte** (`wCurMap`). The stored map size lives in other bytes: the picker
    **says** when they no longer match and offers **Fix** — it never rewrites them behind you (§9.2).
  - **The palette** (`ContrastPicker.qml`) reads as a **percentage** and drops a **segmented slider**.
    Four segments by default (the game's only real levels: 0/3/6/9); a **switch** grows it to ten, the six
    glitch values appearing as their own **differently-coloured** segments — because they are not levels,
    they are a read that lands between two.
- **Context bar (32px)** — *the options for the current tool, or the fields for the current selection*.
  Empty-ish when nothing is doing anything (it shows the palette/contrast stepper then — the one control
  that is always relevant to what you're looking at).
- **Canvas well** — dark neutral surround, the map floats on it with a soft shadow, integer zoom, pans.
- **Dock (right)** — an **icon rail (44px) that is always there** + **at most one expanded panel column**
  (240–420px, drag-resizable, width remembered). Click a lit icon to collapse back to nothing. The map
  never goes below `mapMinWidth`; if the window is too narrow for a panel, the panel **floats over** the
  canvas edge instead of squeezing the map — the eviction queue dies here. **No scrim** (decided while
  building phase 1): a scrim would dim the map you are working on, and Photoshop/Tiled don't dim theirs
  either. The floating panel gets a shadow (so it reads as *above*) and swallows its own clicks, wheel and
  hover so nothing falls through to the map behind it.
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

## 12. The programme — fourteen phases, plus one optional

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

### Phase 4 — SPRITES: the canvas becomes editable ✅ **DONE (2026-07-13, `0.27.0-alpha`)**

> Rewritten **2026-07-13** after the sprite research pass (Twilight). The research is in
> [`../reference/sprites.md`](../reference/sprites.md) — read Parts 3, 5 and 6 before touching any of this.
> Four sub-phases, each finished — designed, built, screenshot-reviewed, tested, documented — before the
> next begins.
>
> **All four are in.** `tst_map_sprites` (10 cases) pins them; its keystone byte-diffs the whole save
> across a drag and demands exactly `mapX` + `mapY` moved. Full `ctest` 83/83.
>
> ⏳ **Owed: Twilight's live pass** — the drag, the drop, the delete and the panel are all things a still
> PNG cannot review.

#### What changed in the design, and why

Three decisions supersede what phases 4–5 used to say:

1. **Background squares are no longer selectable.** Clicking a block/tile on the canvas is **removed**.
   There is no benefit to it — blocks and tiles are better edited in their panels, and making the ground
   clickable fights the thing that *should* be clickable. **Sprites are, for now, the only selectable
   objects on the map.** (Warps, signs and connections join them later, on the same machinery.)
2. **The "Sprite panel" is a DETAILS panel**, and it lives **on the left**. It edits whatever is
   selected; with **nothing selected it shows the map's own details** (the Area block). One panel is the
   single home for "edit what this thing is", and it is never blank.
3. **The Characters bar** — a separate, permanent left-hand rail of *available* sprite pictures — is where
   sprites are **dragged onto the map from**, and **dragged back out to** in order to delete them. It is
   **not** the sprite-set panel (which stays what it is: the map's 11 loaded VRAM slots).

**And the thing the screen must say out loud.** An edited sprite is genuinely there when the save is
loaded — the console proved it — but the game **rebuilds the map's original cast from ROM the moment the
player leaves the map and walks back in**. That is the cartridge's behaviour, not our gap, and it is a
textbook case of the *derived-byte doctrine*: **show it plainly, never hide it, never silently "fix" it.**
A line in the status bar, and a note in the Details panel when a sprite has been added or moved.

---

#### Phase 4a — The data model, made TRUE  *(no new UI; nothing else may be built on a lie)*

The sprite model is a straight port of v1 and it carries **four real bugs** that write wrong bytes into
saves. Every one is confirmed against the disassembly, three of them against the cartridge
([`../reference/sprites.md`](../reference/sprites.md) → Part 5).

- **Fix `SpriteMobility`** — it is inverted. `STAY = $FF`, `WALK = $FE`.
- **Fix `SpriteData::load(MapDBEntrySprite*)`** — it maps `"Stay"` to `$FE`, i.e. it writes **WALK for a
  STAY sprite**.
- **Fix `SpriteGrass`** — it is inverted. **`$80` = in grass.** `reset()` currently flags every blank
  sprite as standing in grass.
- **Fix `face` / `range`** — they are **the same byte** (movement byte 2, `wMapSpriteData`). `face` is
  being routed into `faceDir`, the *animation* facing byte, which is a different field in a different
  table. One byte, one home; `faceDir` becomes what it actually is.
- **Complete `SpriteMovement`** — add `ANY_DIR ($00)` and `NONE ($FF)`; delete the "I have no idea"
  comments and replace them with what the ROM says.
- **Model the missing fields** — StateData1 `a`/`b`/`c` (Y-adjusted, X-adjusted, collision data),
  StateData2 `9` (original facing) and `d` (the duplicate picture id).
- **Model `wToggleableObjectFlags`** (`0x28A0`, 32 bytes, *bit set = hidden*) — the flags that actually
  decide whether a missable NPC appears. We currently model only the per-map **list** (`0x287A`), which
  the game rebuilds from ROM on every map load and which therefore does nothing.
- **Add a `group` field to `sprites.json`** — People / Trainers / Pokémon / Objects, curated from the
  ROM's own names, **approved by Twilight before it is written**. The Characters bar needs it.

Pinned by a new `tst_sprite_data` expansion: every fix gets a **negative control** (put the bug back, the
test fails by name with the exact byte), and a whole-save **byte-diff** proves that loading and re-saving
an untouched save changes **nothing**.

**Exit:** the sprite bytes we write are the bytes the cartridge means, and a test says so.

---

#### Phase 4b — NPCs on the canvas, and they are SELECTABLE

- **Draw them.** The other 15 slots, with everything the player sprite already knows: the **4-pixel lift**,
  **no right-facing art** (it's left, mirrored), the `$FF` = off-screen rule, and the VRAM slot resolved
  through the map's **sprite set**. A sprite whose picture isn't in this map's set is drawn — and **flagged**,
  because that is exactly the "glitchy outdoor sprite" the game will show.
- **Remove background-square selection.** Block/tile clicking on the canvas goes. The Blocks and Tileset
  panels keep their own pickers; the canvas stops competing with them.
- **`MapObjectsModel`.** Hit-testing (`objectAtTile`), selection state, and a **`moveSprite` that writes
  exactly two bytes** — pinned by a whole-save byte-diff.
- **Selection.** Click to select; the selected sprite draws a ring **above every layer** (a selection you
  can lose under a layer is not a selection). `Esc` clears.
- **Two buttons on the selected sprite:** a **✕ delete** and a **✎ edit** — and ✎ **opens the Details
  panel on that sprite**, no hunting for it.
- **Drag to move.** Tile-snapped, live ghost, live X/Y/Δ in the context bar, commit on release. `Esc`
  cancels a drag with **nothing written**.
- **Delete slides the rest up.** The 16 slots are an ordered array; removing one compacts the list, exactly
  as the game packs them. When a compaction moves a **toggleable-object link**, the status bar says so.

**Exit:** every NPC on the map is drawn where the console draws it, can be picked up, moved, and deleted —
and the save changes by exactly the bytes that describes.

---

#### Phase 4c — The Characters bar

A permanent **left-hand rail**, separate from the dock, listing the sprite pictures you can place.

- **1–2 columns**, each cell the sprite's **actual artwork** with its name.
- **Grouped, sorted, organised** — People / Trainers / Pokémon / Objects (the new `group` field), each
  group a collapsible header, with a filter box. 72 entries is a long scroll otherwise.
- **Drag a character onto the map → a new sprite.** It lands tile-snapped, with the game's own sane
  defaults (`movementStatus = Ready`, `imageIndex = $FF`, `yDisp = xDisp = 8`, `STAY`, not in grass) — and
  **the caps are stated before you hit them**: 15 NPCs max, and the sprite-set warning if its picture isn't
  loaded on this map.
- **Drag a sprite off the map onto the bar → delete it.** The bar highlights as a drop target; the same
  compaction rule as ✕.

**Exit:** a person can build a map's cast with the mouse, and never once type a number.

---

#### Phase 4d — The Details panel

The left-hand panel that edits **whatever is selected**, built on the field kit — and the pattern every
future inspector copies.

- **4d-i — The field kit, built properly once.** `FieldRow`, `NumField` (byte/word, dec + hex, **full
  range**), `HexField`, `FlagRow`, `EnumField` (every value **named**, hack values included and labelled),
  `FieldGroup` (titled, collapsible — the trainer-card `PlaytimeGroup` language).
- **4d-ii — Nothing selected → the MAP's details.** The Area block: identity, size, the world's edge, the
  tileset, the palette/contrast, the music, the player. Plus **the view-pointer truth-teller** — live
  mismatch detection and a one-click **Sync**. This is the keystone: it is the pattern for every derived
  byte in the app.
- **4d-iii — A sprite selected → the sprite's details.** Every field, grouped and explained:
  **Who** (picture, with the artwork drawn) · **Where** (map X/Y, the +4 bias shown honestly; screen
  pixels) · **Movement** (movement byte 1 `WALK`/`STAY`; movement byte 2 — `ANY_DIR` / `UP_DOWN` /
  `LEFT_RIGHT` / face `DOWN`/`UP`/`LEFT`/`RIGHT` / `NONE` / `BOULDER` — **one byte, one control**) ·
  **Facing** (the animation facing, `$0`/`$4`/`$8`/`$C`) · **In grass** (`$80`) · **What it is** (text id,
  and the item / trainer-class + set / Pokémon + level from `wMapSpriteExtraData`) · **Visibility** (the
  toggleable-object flag) · **Animation scratch** (image index, base offset, the counters, the step
  vectors, Y/X-adjusted, collision data — every one shown, every one editable).
- **Every value editable across its full byte range**, hack and glitch values included, flagged in words
  when it's a value no real game would hold, and **never refused or rewritten behind the user's back**.
- **The honest note:** when the map's cast differs from the ROM's, the panel says the game will restore the
  original on the next map re-entry.

**Exit:** there is no byte of a sprite that a person can see in a hex editor and cannot see here, in words,
with its range, and change.

---

### Phase 5 — WARPS: the doors ✅ **BUILT (2026-07-14, `0.36.0-alpha`)**

> **All of 5a–5e are in.** `tst_warps` (24 cases) pins them; both keystones are negative-controlled.
> Full `ctest` **85/85**.
>
> **What the screenshot review caught, and it would have shipped:** the fixture save — an entirely
> ordinary one — holds `dungeonWarpDestMap = 194` and `whichDungeonWarp = 0`, a pair that is not in
> `DungeonWarpList`. The first cut screamed at it in red. But **0 is the resting value** —
> `IsPlayerOnDungeonWarp` writes it as its first instruction whenever you are *not* on a hole — so
> **every save ever made carries one**, and `BIT_DUNGEON_WARP` is off, so the console never looks.
> Flagging it is crying wolf on every file anybody opens: exactly the mistake the sprite
> "your cast has changed" notice made first time round. The red **!** now fires only on
> `illegal && armed` — out of the table **and** something is going to read it. Pinned by
> `guns_dontCryWolfOnAnOrdinarySave`.
>
> ⏳ **Owed: Twilight's live pass** — the drag, the drop, the delete, the maker tools and the pickers
> are all things a still PNG cannot review.

---

#### The design of record *(as built)*

> **Read [`../reference/warps.md`](../reference/warps.md) before touching any of this.** It is
> verified against the cartridge, and it changes what several of these fields *are*. The headlines:
> an **edited warp is genuinely live** on Continue (and the game restores the map's original doors when
> the player leaves and re-enters — the sprite rule, again); **two of v1's fields are dead bytes**
> nothing in the game reads; **two of them are loaded guns**; **two whole flags are wiped on load**;
> and **the two bytes that actually matter aren't on the warps screen at all.**
>
> Warps ride on the machinery Phase 4 already built (`MapObjectsModel`, canvas selection, tile-snapped
> drag, the Details panel, the field kit). This phase is mostly **honesty and naming** — plus the one
> piece of new chrome the screen has been missing: **tools that make things.**
>
> ⛔ **SCOPE: warps and nothing else.** Not signs, not connections, not encounters — see **§12b**. They
> are un-briefed, and "it's the same shape" / "it's in the same ROM block" is not a reason to build one.

---

#### Phase 5a — The model, made TRUE  *(no new UI; nothing gets built on a lie)*

Same shape as 4a, and for the same reason.

- **Rename `AreaWarps::skipJoypadCheckWarps` → `forcedWarp`** (`BIT_FORCED_WARP`). The `??` in its
  comment goes with it.
- **Move `AreaMap::blackoutDest` into `AreaWarps` as `escapeWarp`** (`BIT_ESCAPE_WARP`, `0x29DE` bit 6).
  It is not a destination and it is not unused: it is **Dig / Escape Rope / blacked out**.
- **Fix `AreaWarps::setTo()`** — it currently invents `dungeonWarpDestMap`, `specialWarpDestMap`,
  `whichDungeonWarp` and `warpedFromWarp` **at random**, and three of the four are values the console
  has no table entry for (§5 of `warps.md`). A map has **no opinion** about where your last Fly went:
  `setTo()` sets the **warp list** from the map's `warpOut` and **touches no state byte**. Same for
  `randomize()`, which additionally must pick only from the **13 legal fly maps** and the **12 legal
  (dungeon map, hole) pairs** — and hole numbers are **1-based**.
- **Model the legal-value tables** (`FlyWarpDataPtr`, `DungeonWarpList`) so the panel can *say* what is
  legal rather than the code merely avoiding the illegal. Imported from `pret/pokered`, per the
  standing file-format rule.
- **Surface `wLastMap` + `wLastBlackoutMap`.** They already exist (`WorldGeneral::lastMap` /
  `lastBlackoutMap`) — **no new C++ modelling** — they just have to be reachable from the map screen.
- **A `regenerated` signal on the field kit.** Not warp-specific: a **`FieldRow` property** saying "the
  game rewrites this on load" so *every* panel can mark them. Phase 5 is where it gets built, because
  warps are where it first bites.

Pinned by a new **`tst_warps`**, negative-controlled the way `tst_map_sprites` was, with a whole-save
byte-diff proving that loading and re-saving an untouched save changes **nothing**, and that dragging a
warp writes **exactly `x` and `y`**.

**Exit:** the warp bytes we write are the bytes the cartridge means, and a test says so.

---

#### Phase 5b — Warps on the canvas

The doors join the NPCs as first-class objects, on 4b's machinery. Nothing new is invented.

- **Draw them** — a warp chip on its tile (⇄ glyph, its own layer colour), on the **Warps** layer.
- **Select · drag · ✕ delete · ✎ edit** — identical to sprites. A drag commits **exactly two bytes**.
- ⛔ **Signs are NOT in this phase.** They load out of the same ROM block and they are the same shape, and
  that is exactly why they nearly got dragged in here. **Twilight has not briefed signs.** They get their
  own phase, after their own conversation. See "Not yet briefed" below.
- **The pairing line.** Selecting a warp draws, in the status bar, **where it goes** — resolved through
  `MapDBEntryWarpIn`: *"→ Viridian City, arrival point 2 (11, 5)"*. If the destination map has no such
  arrival point, it says **that**, in red, with how many it does have.
- **The `$FF` door.** A warp whose destination is `$FF` is drawn differently and reads as
  **"→ back outside (Pallet Town)"** — resolving through `wLastMap`, live. This is the thing that makes
  `wLastMap` suddenly make sense to a person, and it is why it goes in the toolbar (5c).

---

#### Phase 5c — The toolbar becomes TOOLS  *(Twilight, 2026-07-14)*

> *"We would need the top toolbar to ironically contain actual tools and this is one of them — a create
> random sprite here tool, and a create warp here tool. I guess they can be next to the cursor and stuff."*

The tool group in the top bar (`[ ↖ ✥ ⌕ ]`) grows a **make** section. §5 of this file already specified
a generic **Place** tool; this supersedes it with something better, because a *segmented sub-picker* on
a context bar is a worse affordance than **a tool per thing you make**:

```
 [ ↖  ✥  ⌕ ] │ [ ⇄+  🧍+ ] │ [ Pallet Town · Overworld ⌄ ] [ Outside is: Pallet Town ⌄ ] [ 100% ⌄ ] …
   select    │  the makers  │        what is loaded
```

**Exactly the two makers Twilight named, and no others.** The rail is designed to *grow* one slot at a
time as each object type gets its own conversation — it is not a place to pre-emptively park a tool for
something nobody has specced.

- **⇄+ Place warp** — click a tile, a door appears there. Defaults to `$FF` ("back outside") because
  that is what a door usually is. The **32-cap is stated before you hit it** ("3 of 32"), never after.
- **🧍+ Place sprite** — click a tile, a person appears there, **picture picked at random from the
  map's own loaded sprite set** (so it can never be one of the amber "this map hasn't loaded that
  picture" sprites). This is the "create random sprite here" tool, and it is the fast path that the
  Characters bar's drag-and-drop is the *precise* path for. 15-cap stated up front.
- Each is a real tool: **a cursor, a context bar, an empty state, a keyboard path** (§9's rule), and
  each **respects the active layer** (placing a warp switches the Warps layer on if it was off — a tool
  that makes a thing you then can't see is a bug).

**And the toolbar carries `wLastMap`** — labelled in words, because "wLastMap" means nothing to anyone:

> **`[ Outside is: Pallet Town ⌄ ]`**

It sits with *what is loaded*, not in a panel, because **every `$FF` door on the map re-reads as you
change it** — it is the single control that changes what a dozen warps *mean*, and watching them
re-label as you pick is the whole point. (This is the field Twilight was reaching for with "From map".
`wWarpedFromWhichMap`, the byte actually named that, is **dead** — see 5d.)

---

#### Phase 5d — The Warp State group, in the Details panel

Right-hand side, in the Details panel, as a **titled collapsible group** (the `FieldGroup` language) —
`⇄ Warp State`, with **one ? icon** in the group title (the app's one allowed tooltip-icon; see
`ui-patterns.md`). Every field gets a **plain-English name** and a **one-line "what this does"**, because
the app is where that gets answered.

**The group, top to bottom:**

| Shown as | Real byte | The one-liner |
|---|---|---|
| **Wake up at…** *(map picker)* | `wLastBlackoutMap` | "Blacking out, Dig and Escape Rope all bring you here." |
| **Fly sends you to…** *(picker, **13 maps only**)* | `wDestinationMap` | "Where the last Fly / special warp was headed." 🔫 |
| **Falling drops you onto…** *(picker, **7 maps only**)* | `wDungeonWarpDestinationMap` | "The floor below, if you're falling down a hole." 🔫 |
| **…through hole #** *(1–3, **paired** with the above)* | `wWhichDungeonWarp` | "Which hole. Seafoam B1F has two; Victory Road 2F only has a #2." 🔫 |
| **Arriving at door #** | `wDestinationWarpID` | "Which arrival point of the map you're entering. `255` = don't move me." |
| — *a divider* — | | |
| **A special warp is in progress** | `BIT_FLY_OR_DUNGEON_WARP` | "Set while a Fly / hole / Dig warp is mid-flight." |
| **Arrive with the drop-in animation** | `BIT_FLY_WARP` | "How you land off a warp pad or a Fly." |
| **You fell down a hole** | `BIT_DUNGEON_WARP` | "Sends the destination lookup to the hole table instead of the fly table." |
| **Dig / Escape Rope / blacked out** | `BIT_ESCAPE_WARP` | "Sends you to *Wake up at…*" |
| **Doors fire without walking into them** | `BIT_FORCED_WARP` | "Normally you must walk *into* a door. This makes touching the tile enough. (It's how the Seafoam current sweeps you along.)" |

**🔫 The two loaded guns get the music-bank treatment.** The pickers offer **only the legal values**
by default, with a **switch** to show the full 248 — and a save that already holds an illegal one is
**shown holding it**, in `errorColor`, with the plain-English consequence ("no console has a table entry
for this; the game will read whatever ROM bytes follow the table and drop you somewhere undefined"). It
is **never refused and never silently rewritten.** Same doctrine, same words, as the music bank.

**And the ! group at the bottom — collapsed behind a switch, exactly as asked:**

> **`[ ⚠ Show 4 fields the game rewrites on load ]`**  *(a `FlatToggle`; off by default)*

Flip it and four rows appear, each carrying a **yellow `!`** (`MapWarnIcon` — the *other* allowed
tooltip-icon):

| ! | Shown as | Real byte | Why it's marked |
|---|---|---|---|
| ⚠ | **Script is warping you now** | `BIT_WARP_FROM_CUR_SCRIPT` | **Zeroed on load.** `wStatusFlags3` shares a byte with `wCableClubDestinationMap`, and `SpecialEnterMap` writes `0` to it on the Continue path. *Console-verified.* |
| ⚠ | **Standing on a hole** | `BIT_ON_DUNGEON_WARP` | Same byte. **Zeroed on load.** |
| 💀 | **Came in through door #** | `wWarpedFromWhichWarp` | **Survives the load perfectly — and nothing in the game ever reads it.** Two writes, zero reads, in the whole disassembly. |
| 💀 | **Came from map** | `wWarpedFromWhichMap` | Same. A breadcrumb the game drops and never picks up. |

Two different kinds of "this does nothing", and the panel **says which is which** — a wiped byte and an
unread byte are not the same fact, and collapsing them into one grey "unused" would be exactly the kind
of hand-wave this project doesn't do.

> **The `!` is a general mechanism, not a warp feature.** `wStatusFlags3` is zeroed on load *in its
> entirety*, so the same mark is owed to `AreaNPC::npcsFaceAway`, `AreaNPC::tradeCenterSpritesFaced`,
> and `AreaPlayer`'s `isBattle`/`isTrainerBattle` (which are really `BIT_TALKED_TO_TRAINER` and
> `BIT_PRINT_END_BATTLE_TEXT` — misnamed too). Phase 7 inherits the mechanism and the list.

**The honest note**, in the panel and in the status bar, when the map's doors differ from the ROM's:

> *"These doors are live — the game will use them when this save loads. It restores the map's original
> doors when the player leaves the map and walks back in."*

**Exit:** there is no warp byte a person can see in a hex editor and cannot see here, in words, with its
range, its legal values, whether the game will keep it, and whether anything reads it at all.

---

#### Phase 5e — The Player *(the last piece the warp work leans on)*

**Player** properties — position, facing, movement, standing-on (`BIT_STANDING_ON_DOOR` /
`BIT_EXITING_DOOR` / `BIT_STANDING_ON_WARP` — the warp-adjacent trio), what he may do here, battle state,
scratch. This one stays in Phase 5 because the warp panel keeps pointing at it: a door only means
anything relative to where the player is standing.

**Exit of Phase 5:** there is no *warp* byte a person can see in a hex editor and cannot see here, in
words, with its range, its legal values, whether the game will keep it, and whether anything reads it.

---

#### Phase 5f — The Player details panel, in FULL *(briefed 2026-07-14, Twilight)*

> *"Let's flesh out more character details — the player detail panel should have Moving, Last Stop,
> Current direction, X/Y Coords, X/Y Block Coords, Jumping Y, Using strength, Using Fly, Surfing
> Allowed, Battle Ongoing, Trainer Battle, Prevent Battles, Battle Ended, X/Y Offset Special Warp,
> Standing on Door, Moving through door, Standing on warp, Walk/Bike/Surf status, End Edge jump, Spin
> Player, Used Card Key, Using Link Cable. These are related to map state so they should be on the
> player box. Figure out what all those options are, figure out which ones are rewritten on startup,
> organize structure, follow the ui/ux."* — Twilight

5e shipped **just the player's position** in the Details panel. This phase puts **all 26 bytes of
`AreaPlayer`** there — the full v1 `area-player` field set (Direction / Coordinates / HMs / Battle /
Warps / Other), rebuilt to our doctrine. **Read [`../reference/player-state.md`](../reference/player-state.md)
before touching any of it** — it is researched and **console-verified byte-by-byte**
(`scripts/emu/probe_player_state.py`), and it settles the "which are rewritten on startup" question the
brief asks.

> ⛔ **SCOPE: the player box and nothing else.** These bytes describe *the player on the map* and belong
> on the player object's panel. Not area state, not encounters, not the tileset deep pass — see **§12b**.

**5f-0 — Make the model honest first (no UI).** Same precedent as 5a / sprites 4a: the byte offsets and
bit numbers in `AreaPlayer` are **all correct**, but five fields are **misnamed** and the "rewritten on
load" facts are undocumented. Before the panel is built on them, rename + document (there is **no loaded
gun** here — `setTo()` only touches coords/dirs — so this is a truth-in-labelling pass, not a
save-safety one):

| v2 field | → rename toward | because |
|---|---|---|
| `isBattle` | `talkedToTrainer` | `BIT_TALKED_TO_TRAINER`; **zeroed on load** |
| `isTrainerBattle` | `printEndBattleText` | `BIT_PRINT_END_BATTLE_TEXT`; **zeroed on load** |
| `flyOutofBattle` | `arrivedByFly` | `BIT_USED_FLY` — the drop-in animation, not "Fly usable" |
| `finalLedgeJumping` | `ledgeHopOrFishing` | `BIT_LEDGE_OR_FISHING` — cleared at load by `HandleMidJump` |
| `usedCardKey` | `unusedCardKey` (dead) | `BIT_UNUSED_CARD_KEY`, setter is `; never checked` |

**5f-1 — The panel, organised into four honest buckets + the collapsed rewrite group** (mirrors the
warp panel's `[ ⚠ Show N fields the game rewrites on load ]`):

- **Position** *(the durable core)* — Tile **X/Y** and half-block **X/Y** grouped into coordinate
  controls (the pattern the sprite/sign panels use: one X+Y control, not four loose fields);
  **Current direction** *(⚠ forced DOWN on Continue)*, **Last stop**, **Moving** *(0 in any real save)*;
  and the **Walk / Bike / Surf** segmented control (`SegSel`, 3 options).
- **Here he may…** — **Surfing allowed** *(`~` recomputed near water)*, **Arrived by Fly** *(`~`)*,
  **Using Strength** *(⚠ reset on an ordinary Continue; survives only if Battle ended is set)*.
- **Battle** — **Prevent battles** *(durable)*, **Battle ongoing** *(⚠)*, **Trainer battle** *(⚠)*,
  **Battle ended / blackout** *(⚠ always cleared on entry)*.
- **Warp-adjacent trio** — **Standing on warp** *(`~`)*, **Standing on door** *(⚠)*, **Walking through
  door** *(⚠)*. These are the same three the door panel points at (`warps.md` §6), shown here where the
  player lives.

**5f-2 — The `[ ⚠ Show N fields the game rewrites on load ]` switch** (off by default). Flipping it
reveals the **ten `!` fields** each with a yellow `!` (`MapWarnIcon`) explaining *what* the console does
(forces / zeroes / clears), and the **three `💀` dead fields** (`x/yOffsetSinceLastSpecialWarp`,
`usedCardKey`) each with the grey "the game writes this and never reads it" line. **Jumping Y** and the
ledge/spin scratch bits live here too. A wiped byte and an unread byte are **different facts** — say
which, exactly as the warp panel does.

**The doctrine holds:** every value stays **full-range and editable, hack values included**; the panel
**never refuses and never silently rewrites** — it only tells the truth about the next load. And the
**view-pointer truth-teller** (§13-style, already designed) still applies: editing the player's coords
invalidates the stored view pointer, so the panel *shows* the mismatch and offers one-click **Sync** —
never a quiet rewrite.

**5f-3 — Tests.** `tst_area` (or a new `tst_player`) gets a keystone in the sprite/warp mould: set each
player field, `save()`, and **byte-diff the whole 32 KB** — exactly the intended bytes/bits moved,
nothing else. Plus a round-trip `load()`→`save()` identity on `BaseSAV`.

**Exit of 5f:** there is no player byte a person can see in a hex editor and cannot see here, in words,
with its range, whether the game keeps it on load, and whether anything reads it at all.

**Exit of Phase 5:** there is no *warp or player* byte a person can see in a hex editor and cannot see
here, in words, with its range, its legal values, whether the game will keep it, and whether anything
reads it.

---

### Phase 6 — SIGNS: the placards ✅ **BUILT (2026-07-14, `0.38.0-alpha`)**

> **All of 6a–6d are in.** `tst_signs` (15 cases) pins them; the keystone byte-diffs the whole 32 KB
> across a drag. Full test set green. The importer (`scripts/import_sign_text.py`) is additive-only —
> strip its `textEntries` back out and `maps.json` is byte-identical to before. ⏳ Owed: Twilight's
> live pass on the drag, the delete, the tool and the grouped picker.

> *"Let's add sign features to the map — an add-sign tool, and the details panel on the left needs
> x,y coordinates and an actual combo box to select from the text on the map. The usual delete
> options, much the same as warps and sprites."* — Twilight, 2026-07-14. Then, on the combo: *"all
> entries that can be referenced on a sign no matter what the text is meant for — group them"*; and
> on scope: *"break it into as many phases as you need to fully and comprehensively cover this."*

Signs are the second briefed object type, and they ride on everything Phases 4–5 built: the
`MapObjectsModel` machinery, canvas selection, tile-snapped drag, the Details panel, the field kit,
the tool rail. The one genuinely new thing is **the text** — the save holds a text *id*, and to let a
person *"select from the text on the map"* the app has to know the map's actual words, which are in
`pret/pokered` and not yet in our data.

> **Read [`../reference/signs.md`](../reference/signs.md) before touching any of this.** The
> headlines: **our save model is already correct** (the rare pass with no bug to fix first); a sign
> rides the **same persistence linchpin as a warp** (live on Continue, restored on re-entry — the
> panel must say so); and the text id is a **1-based index into the map's `def_text_pointers`
> table**, whose entries are shared by signs, people and script-only text — which is why the picker
> **groups** them.

> ⛔ **SCOPE: signs and nothing else.** Not connections, not encounters, not the NPC state — see
> **§12b**. Reading the map's text table is fine (a sign *needs* it); building a UI for anything else
> is not.

---

#### Phase 6a — The map's text, extracted from `pret/pokered`  *(the data; no UI)*

The one piece of new ground. `maps.json` ships a sign's `text` **id** but not the words. This phase
puts the words in, per the standing file-format rule (extend the map data we already ship; don't
invent a parallel file).

- **A new self-validating importer** — `scripts/import_sign_text.py` (precedent: `import_tile_traits.py`,
  `import_sprites.py`). Per map it reads: `data/maps/objects/<Map>.asm` (the `bg_event` /
  `object_event` lines → which text ids are **signs**, which are **people**), `scripts/<Map>.asm` (the
  `def_text_pointers` list → id → label and the map's text **count**), and `text/<Map>.asm` + its
  `text_far` targets (label → **the actual string**, decoded through the game's control codes —
  reuse the app's existing font/text decode; `#` → POKé, `line`/`para`/`cont`/`@`).
- **Output** rides in `maps.json` as a per-map **text table**: an ordered list, index `1..N`, each
  entry `{ "string": "...", "category": "sign" | "person" | "other" }`. A `text_asm` entry with no
  single literal gets `string: null` and is shown as `(scripted text)`.
- **DB wiring** — `MapDBEntry` gains `getTextTable()` / a QML-reachable grouped accessor so the panel
  can offer the map's text without the save model needing to know about strings at all.
- **Self-validating** the way the tile importer is: assert every `maps.json` sign's `text` id lands
  inside its map's table, and re-derive the count independently. If a map's numbers don't line up,
  the importer fails by name.

**Exit:** given a map id, the app can hand QML the map's whole text table, grouped, in real English.

---

#### Phase 6b — Signs on the canvas

The placards join the doors and the NPCs as first-class objects, on 4b/5b's machinery. Nothing new is
invented.

- **Draw them** — a sign chip on its tile (🪧 glyph, its own layer colour), on a new **Signs** layer
  in the **Objects** layer group (beside People & objects, Warps).
- **Select · drag · ✕ delete · ✎ edit** — identical to warps and sprites. A drag commits **exactly two
  bytes** (the sign's Y and X, in their own coord array). The keystone test byte-diffs the whole 32 KB
  and proves only those two moved.
- **The reading line.** Selecting a sign shows, in the status bar, **what it says** — the first line of
  its text, resolved through 6a's table: *"🪧 (7, 9) — “PALLET TOWN / Shades of your…”."* If the id
  points past the map's table, it says **that**, in `errorColor`, with how many entries the map has.

---

#### Phase 6c — The Place sign tool

The tool rail (`[ ↖ ✥ ⌕ ] │ [ ⇄+ 🧍+ ]`) grows exactly **one** slot — the third maker Twilight has now
briefed, and no others:

```
 [ ↖  ✥  ⌕ ] │ [ ⇄+  🧍+  🪧+ ] │ [ Pallet Town · Overworld ⌄ ] [ Outside is: … ⌄ ] [ 100% ⌄ ] …
```

- **🪧+ Place sign** — click a tile, a sign appears there, defaulting to the map's **first sign text**
  (or text id 1 if the map has none), because a fresh placard should say *something* real. The
  **16-cap is stated before you hit it** ("3 of 16"), never after.
- A real tool by §9's rule: a cursor, a context bar, an empty state, a keyboard path; and it **respects
  the active layer** — placing a sign switches the Signs layer on if it was off.

---

#### Phase 6d — The sign Details panel

Left-hand Details panel, the same one warps/sprites use, showing the selected sign (and the map's own
details when nothing is selected). A **titled group** (`🪧 Sign`) in the `FieldGroup` language, with one
`?` in the title.

**The group, top to bottom:**

| Shown as | Real byte | Notes |
|---|---|---|
| **Position** *(one X + Y control)* | `wSignCoords[i]` (Y, X) | Grouped into a single control, per the sprite-panel lesson (2026-07-13) — not two loose number fields. Tile-range for the map; also drives the canvas drag. |
| **Says…** *(grouped ComboBox)* | `wSignTextIDs[i]` | The map's whole text table from 6a, **grouped** — *Signs* first, then *People*, then *Other* — each row the id + its real words (scripted entries read `(scripted text)`). The `music`-picker pattern: a real grouped `ComboBox`, **selection commits, nothing on hover.** |

- **Every value editable, hack included.** The combo lists what the map has, but the underlying id is a
  full byte: a switch reveals the raw id spinner (0–255), and a save holding an id past the map's table
  is **shown holding it**, in `errorColor`, with the plain-English consequence ("this map has 7 text
  entries; id 9 points past them — the game reads past the table"). Never refused, never rewritten.
- **A Delete button** (not a cryptic ✕) at the foot of the group, exactly as the sprite/warp panels do.
- **The honest note**, in the panel and status bar, when the map's signs differ from the ROM's:
  > *"These signs are live — the game will use them when this save loads. It restores the map's
  > original signs when the player leaves the map and walks back in."*
  The same mechanism the warp panel earned (§5's linchpin), reused verbatim.

**Exit of Phase 6:** a person can place a sign, drag it, delete it, and choose what it says from the
map's own words — and there is no sign byte visible in a hex editor that is not visible here, in
English, with its range and whether the game keeps it.

Pinned by a new **`tst_signs`**, negative-controlled like `tst_warps`/`tst_map_sprites`: a whole-save
byte-diff proving load+resave of an untouched save changes **nothing**, that dragging a sign writes
**exactly its two coord bytes**, and that every `maps.json` sign id resolves inside its map's 6a table.

---

## 12b. ⛔ NOT YET BRIEFED — do not design these, do not build them  *(2026-07-14, Twilight)*

> *"Let's not get too far ahead of ourselves. Signs and stuff, connecting routes, wild Pokémon — these are
> examples of things I haven't gotten to yet. I'd hate to have to undo a lot of work because it was done
> before I explained anything."*

The phases below are **placeholders, not designs.** They were sketched early, from the *save layout* —
which is a map of what bytes exist, not of what Twilight wants a person to be able to *do*. Every screen
in this project that got designed from the bytes had to be built twice.

**The rule:** a phase in this list gets **its own conversation with Twilight first**, then a research pass
(`CLAUDE.md` → *RESEARCH LANDS IN THE NOTES*), then a design written *here*, and only then code. **A
neighbouring phase does not get to absorb one of these because the data happens to sit next to it.**
Signs nearly rode into Phase 5b on exactly that logic — they load out of the same ROM block as warps —
and that is precisely the mistake this section exists to prevent.

| Phase | Un-briefed | The temptation to resist |
|---|---|---|
| ~~**Signs**~~ | ✅ **BRIEFED 2026-07-14 — graduated to Phase 6.** | Was cut out of Phase 5b, held here until Twilight briefed it. She now has: add-sign tool, X/Y, a grouped text picker of the map's real text. See **Phase 6** above and [`../reference/signs.md`](../reference/signs.md). |
| **Connections** | 🔗 the four edge connections (N/S/E/W) — "connecting routes" | The strips are already *rendered* and fully understood ([`../reference/map-connections.md`](../reference/map-connections.md)), so *editing* them looks like a small step. It isn't — nobody has said what editing a connection should mean. |
| **Phase 7 — Encounters** | 🌿 wild Pokémon (`grassRate`, 10 grass slots, `waterRate`, 10 water slots, `pauseMons3Steps`) | The Grass/Water meaning layers already exist, so it looks like the panel is half-built. It isn't. |
| **Phase 8 — Area State** | the `AreaNPC` flags, the `AreaWarps` state that isn't warp-flow, `AreaLoadedSprites` | It is "the leftovers", which is not a design. |
| **Phase 9 — Tileset & Blocks** | the deep pass | The panels exist; the *deep* pass does not have a brief. |

**What the earlier text in this file says about these is a sketch and carries no authority.** Read it as
"here is what the bytes are", never as "here is what we agreed."

**What IS briefed and safe to build:** Phase 5 (warps, 5a–5e) and **Phase 6 (signs, 6a–6d — briefed
2026-07-14)**, and nothing that touches connections, encounters or area state. Where a briefed phase
genuinely *needs* one of them, it **reads** it; it does not build a UI for it.

---

### Phase 7 — Encounters

`AreaPokemon` has **no UI at all** today. `grassRate` · 10 grass slots · `waterRate` · 10 water slots ·
`pauseMons3Steps`. Species picker + level, drag-to-reorder (the Bag/Moves drag pattern), rate 0 said in
words ("no wild Pokémon here"), and a link to the Grass/Water Meaning layers so the panel and the map agree.

---

### Phase 8 — Area State

`AreaNPC` (9 flags), the `AreaWarps` state fields (12), `AreaLoadedSprites` (`loadedSetId` + 11 slots) —
none of which has a UI today. Three titled groups, every flag explained, the sprite-set slots reorderable.

---

### Phase 9 — Tileset & Blocks, properly

The two panels that already exist get the *deep* pass rather than the re-chrome they got in phase 1:
the tri-state tile animation with its "what this actually does", the grass tile and the three counter slots
via `TileField` + eyedropper, the Strength scratch, and the **Advanced** disclosure (bank / blockPtr /
gfxPtr / collPtr diffed against the cartridge, with Restore). Blocks keeps its click-to-inspect and its
16-tile breakdown, gains the out-of-bounds-block picker, and loses nothing.

---

### Phase 10 — Tools & precision

Place · Eyedropper · Measure · snap modes · nudge keys (1 tile / 1 block / 8 tiles) · align + distribute for
a multi-selection · every action reachable from the keyboard · the shortcut map written down. **A tool is
not done until it has a cursor, a context bar, an empty state and a keyboard path.**

---

### Phase 11 — Motion & polish

Panel slide, chip hover, selection pulse, layer-toggle cross-fade (the overlay *arrives*, it doesn't slam
on), empty states, tooltips on everything that has a name nobody should be expected to know. Then the full
screenshot pass at three window sizes, and the accessibility read (colour is never the only signal — every
layer has a *pattern* as well as a hue, which is already true of the overlay and must stay true of the chips).

---

### Phase 12 — The verification pass

- Full `ctest` (incl. `tst_emu_parity` against the real cartridge).
- **A byte-diff harness run over every edit the screen can make**: load a save, perform the edit, diff the
  whole 32 KB. Anything that changed and shouldn't have is a **release blocker**, not a bug report.
- Linux Docker (standard / asan / xvfb / coverage).
- A live pass with Twilight on the built app.

---

### Phase 13 — The notes pass

`status.md`, `ui-patterns.md` (the new chassis, dock, layer-row and field-kit conventions),
`qt-patterns.md` (whatever Qt tried to do to us), `decisions/architecture.md` (the layer/object models),
`decisions/rejected.md` (whatever we tried that failed), this file (marked *built*), the session log,
`_nav.dox`, `VERSION`, and the credits check.

---

### Phase 14 — SIMULATE: walk the map  *(**OPTIONAL / stretch** — runs last, gates nothing)*

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
