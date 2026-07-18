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
| **Tiles** (was "Meaning"/"Components"; renamed 2026-07-15) | Walls | overlay bit `1<<0` | The tileset's own tile meanings MapEngine renders. |
| | Grass | `1<<1` | Each has its own hue **and** its own 8×8 pattern (they stack and still read). |
| | Water | `1<<2` | |
| | **Warp tiles** | `1<<3` | ⚠️ The *tile trait*, **not** the warp objects. Which tile graphics are warp-capable on this tileset (doors, stairs, cave mouths, warp pads) — a tileset fact (`warp_tile_ids.asm`; `IsWarpTileInFrontOfPlayer` checks it to decide a warp can fire). Distinct from the save's warp *list* (the objects in Game View), so it's labelled "Warp tiles". (Twilight, 2026-07-15: kept — it's a real, separate thing.) |
| | Doors | `1<<4` | A door is a passable **tile type** you walk across to reach a warp (not the warp itself) — a real tileset trait, so it belongs here. |
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

**Defaults on open (Twilight, 2026-07-15):** **every Game View layer ON except the Draw area** — the
player, the people, the **warps**, the signs and the screen box; the draw area (engine scratch) off.
**Every Tiles-group overlay OFF** (*the map is the point*). Guides = block grid, map bounds and
connections on; tile grid + border off. (Warps show by default now as the Game View **object** layer,
which is why that object layer is on — it replaced the old warp *tile-trait* overlay that used to stand
in for it.)

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

> **The `AreaMap` "Map page" slice is now researched + console-verified**
> ([`../reference/area-map-state.md`](../reference/area-map-state.md), 2026-07-15): the seven
> leftover fields Twilight is bringing to the map-details panel — `wCurMapScript` (Current Script),
> `wCurrentTileBlockMapViewPointer` + `wMapViewVRAMPointer` (the two UL-corner pointers),
> `BIT_USE_CUR_MAP_SCRIPT` (Run cur map script instead), `BIT_ALWAYS_ON_BIKE` (force bike ride),
> and `wCardKeyDoorX/Y`. Verdicts + the two probe surprises are in that note. Design direction
> (Twilight, 2026-07-15): script = descriptive ComboBox + "Something else…"; the pointer fields
> intuitively selectable, never a raw address except behind "Something else…". Awaiting the design
> pass before any build.

- **Character state** *(AreaNPC)* — **now researched + briefed** as its own right-hand panel
  ([`../reference/npc-character-state.md`](../reference/npc-character-state.md), 2026-07-15). The nine
  map-global flags (`npcsFaceAway`, `scriptedNPCMovement`, `npcSpriteMovement`, `tradeCenterSpritesFaced`,
  `ignoreJoypad`, `joypadSimulation`, `runningTestBattle`, `trainerWantsBattle`, `trainerHeaderPtr`) all
  turned out to be transient script/battle/link scratch, three of v1's labels are wrong, two are zeroed
  on load, and the rest owe an emulator probe. Design + phased build are **Phase 9 (Character State)**
  below — this sketch is superseded by it.
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
where it goes. That is the map emulator finally *being* one. It is scoped as its own phase (**Phase 15**) with
its own question in §14, because it is the one feature here that is a small game engine rather than a
screen.

## 9. The doctrine (non-negotiable, and it is why this is not just a re-skin)

1. **Byte fidelity.** An edit writes *only* the bytes it names. Dragging a warp writes `x` and `y`.
   Nothing else in the save moves — not a checksum region we weren't told to touch, not a "normalised"
   neighbouring field, not an unused bit. See `../context/principles.md` → *Save File Integrity Is Sacred*.
2. **Derived bytes are kept IN SYNC by default — power users can break sync (clarified 2026-07-15,
   Twilight).** A value the game *computes* from another (`currentTileBlockMapViewPointer` from the player's
   coords + the map width; the tileset pointers; the music bank) is, **by default, kept correct
   automatically** — most people editing the Map want that, and it is *bad UX* to let a novice break their
   map because they didn't also hand-edit a derived field they had no reason to touch. The balance:
   **raw-byte editing is always available** (the power path), and a derived value **may be deliberately
   desynced** — via a *break-sync* toggle, by entering a different value (which raises an **alert offering to
   break sync**), or, for the view box, by **dragging it on the canvas**. Once desynced it lives
   independently and the source stops driving it. This is *not* silent corruption — byte fidelity still holds
   (an edit writes only the bytes its action implies) — it is the *opposite* of leaving a novice to discover
   a broken map. Same balance for the tileset pointers and the music bank.
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
8. **`MapWalker`** (Phase 15) — the walk simulation: the passable-tile list, the elevation pairs, the ledge
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

## 12. The programme — the phases, plus one optional  *(Phase 15 "Map Storage" added 2026-07-15)*

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
  tileset, the palette/contrast, the music, the player. Plus **the view box, synced by default** — the
  camera pointer tracks the player automatically, with a *break-sync* toggle (and an alert if a different
  pointer value is entered) so power users can desync it; once desynced the view box is even **draggable on
  the canvas**. This is the keystone: the sync-by-default + break-sync pattern for every derived byte in the app.
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

#### Phase 5f — The Player details panel, in FULL ✅ **BUILT (2026-07-14, `0.39.0-alpha`)**

> **In.** `MapModel::playerFields()`/`setPlayerField()` + `PlayerField.qml` render all 26 bytes in the
> Details panel, grouped, with the rewrite/dead thirteen behind the *Reloaded values* switch. Model
> members kept their (now-documented) names — the rename in **5f-0** below is deferred as an optional
> truth-in-labelling cleanup, since it is internal-only and carries breakage risk for no user-visible
> gain; the panel already carries the correct English names and the load-behaviour notes. Pinned by
> **`tst_player`** (7 cases). ⏳ Owed: Twilight's live pass on the scroll, the switch and the controls.

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

### Phase 7 — CONNECTIONS: the connecting routes  *(BRIEFED 2026-07-15, Twilight)*

> **Read [`../reference/map-connections.md`](../reference/map-connections.md) before touching any of this
> — all of it, and especially the new "Editing a connection — the human model" section.** The strips are
> already *rendered* and **cartridge-verified 78/78**; this phase is about *editing* them, and the whole
> design turns on one fact: **a connection has only two human inputs — the neighbour map and one signed
> offset (−27…+27) — and the other nine bytes are derived by the game's macro.** v1 exposed the nine raw
> and asked a person to keep them consistent by hand; that is exactly why it was error-prone.
>
> Connections ride the machinery Phases 4–6 built (`MapObjectsModel`, canvas selection, the Details
> panel, the field kit) and the renderer that already reproduces the macro (`MapEngine::connectionOf`).
> This is the derived-byte doctrine (§9.2) applied to its hardest case.
>
> **The four design decisions (Twilight, 2026-07-15):**
> 1. **Offset-driven, auto-derive** — pick the neighbour + slide one offset; the editor recomputes the
>    nine derived bytes via the macro. **The Details panel always shows the raw values too** (read-only
>    while synced, editable once desynced).
> 2. **The live connection is drawn as the FULL neighbour map**, bleeding off the edge, **draggable**
>    along the shared edge — sliding it *is* setting the offset. `MapEngine` can already render any map to
>    an image.
> 3. **Both add-gestures** — a ghost edge-arrow you *click* to add (then pick a map), **and** a map you
>    *drag* onto the arrow from a picker/rail.
> 4. **Never auto-rotate.** The neighbour always renders in its natural orientation (rotation is not
>    representable in the save — see the reference note). But **"attach to another edge" IS offered** as an
>    explicit option (a Details-panel direction change and/or a deliberate drag to another edge), because
>    re-homing N↔S↔E↔W *is* representable: it moves one flag bit + one 11-byte slot and recomputes.
>
> ⛔ **SCOPE: connections and nothing else.** Not encounters, not area state — see **§12b**.

---

#### Phase 7a — The model, made TRUE  *(no new UI; nothing gets built on a lie)*

Same shape as 4a/5a/6a. The connection struct is sound (`MapConnData`, cartridge-verified), so this is
lighter than sprites/warps were — but two things must be right first.

- **Add/delete write exactly their bytes.** `AreaMap::connNew`/`connRemove` already set/clear the
  `0x261C` flag bit and (on save) the one 11-byte slot. Pin it with a whole-save byte-diff: **add** touches
  only the flag byte + that slot; **delete** clears only the flag bit; load+resave of an untouched save
  changes **nothing**. (Byte fidelity: delete does **not** scribble the stale slot unless a value is
  written.)
- **A macro helper as the single source of derivation.** `MapEngine::connectionOf` already computes the
  eleven bytes from (direction, both sizes, offset) exactly as the ASM macro does. Expose it to the model
  (`MapModel`/`MapObjectsModel`) as the one function that turns **map id + offset** into the nine derived
  bytes, and the **offset ↔ (`stripMove`,`stripOffset`) inversion** (reference note) so an existing
  connection's offset can be *recovered* for the slider. ⚠️ **Never** derive via
  `MapDBEntryConnect::stripSize()` — it is wrong and `maps.json`'s `flag` only patches it (Open question 4,
  §14; reference note). The panel and the canvas both compute from the macro.
- **`MapConnData` naming honesty.** Keep the fields, but the UI must not conflate `stripWidth`
  (`ConnectionStripLength`: blocks-per-row N/S, **rows** E/W) with `width` (`ConnectedMapWidth`, the source
  row stride). A comment/`regenerated`-style hint in the panel, not a rename of the save struct.
- **Model the legal offset range** (−27…+27 is what ships; the arithmetic permits `_src = −1` at
  `offset = −2`) so the panel can *say* what is ordinary vs. what is a hack value — never refuse it.

Pinned by a new **`tst_connections`**, negative-controlled like `tst_warps`/`tst_signs`.

**Exit:** map id + offset → the nine bytes the cartridge means, and a test says so.

---

#### Phase 7b — Connections on the canvas

> **PART 1 BUILT (2026-07-15, `0.39.2-alpha`) — add / select / delete / slide-with-snap.**
> `MapConnection.qml` (a selectable, deletable strip you **drag along the edge to set the offset**, with
> magnetic **snapping to the landmarks** — corner-aligned / centred / flush — computed in
> `connectionEditList`'s new `snaps`) and `ConnectionArrow.qml` (the **ghostly click-to-add** arrow on
> each empty edge → a grouped map picker → `addConnection`). Four fixed items each (not Repeater
> delegates, so an offset edit never rebuilds one mid-drag); `selectedConnection` joins the
> one-selection-at-a-time model. `tst_qml_screens` 16/16, `tst_connections` 14/14.
> **⏳ Remaining in 7b (owed):** the **full neighbour-map render** bleeding off the edge (Twilight's
> chosen visual; today it's the faithful strip-outline proxy), the **drag-a-map-onto-the-arrow**
> gesture (today: click-to-add only), and **Twilight's live pass** (drag/select/snap can't be reviewed
> from a still PNG). Feature rides the **Connections** layer (turn it on to see arrows + strips).

The four edges become live, on 4b's object machinery + the neighbour-map renderer.

- **Ghost arrows.** A faint, **lightweight** white arrow (~1 block) centred on each of the four edges,
  drawn only where **no** connection exists — an invitation, never chrome. It sits on the **Connections**
  layer (§4). Gone the instant a connection is added; back when it's removed.
- **✅ DONE (2026-07-15, `0.39.7-alpha`): the map's own RING renders the SAVE's connections — a
  Continue-load.** `buildOverworldMap`/`applyConnections` take an optional `SaveConn` list and bleed the
  save's **raw** struct bytes; it rides in the `image://map` URL (`MapModel::source`) and `mapBuffer()`,
  so image + status bar + strip box all agree. Neighbour maps keep their ROM connections (we don't hold
  their save state). ⏳ Remaining nicety: the **Border overlay layer** (`requestOverlay`) still uses ROM
  connections for its ring — only visible with that guide on; low priority.
- **The live connection = the full neighbour map.** When a connection exists, render the neighbour map
  (via `MapEngine`) as an image bleeding off that edge, clipped to a comfortable margin, dimmed slightly so
  *our* map stays the subject. It aligns to the offset — what you'd see if you could peer past the seam.
  > **BUILT (2026-07-15, `0.39.4-alpha`).** Four fixed `PixelImage` items, one per edge, rendering the
  > neighbour's own map (`image://map/<toMap>/<toTileset>/…`, `toTileset` added to `connectionEditList`),
  > positioned so the neighbour's shared map edge meets ours + the offset, drawn **behind** our opaque
  > buffer (z −0.5) so only the off-edge part shows, dimmed 45%, re-positioned live on an offset drag.
  > ⏳ Owed: Twilight's live pass on the alignment (layer-gated + edge-located + zoom-dependent, so it
  > can't be still-captured).
- **Drag = offset.** Grab the neighbour map and slide it **along the shared edge only** (constrained to the
  one axis); the offset snaps to whole blocks; the nine derived bytes recompute live; the border-ring strip
  re-renders as you drag. Live offset + Δ in the context bar, exactly like a warp drag. `Esc` cancels with
  nothing written; release commits only the touched slot's bytes.
- **Snap to the landmarks (Twilight, 2026-07-15).** A raw block slide is fiddly; the drag should **snap to
  the positions a person actually wants**, with a light magnetism (and a modifier — `Alt` — to escape it for
  a free slide). The landmarks, all computable from the two maps' sizes: **offset 0** (the default a fresh
  connection lands on — the game's own resting value, the two maps corner-aligned), **flush edges** (the
  neighbour's far edge lined up with ours — the `_len` clamp's boundary, `offset = curW − toW` and the
  mirror), and **centred** (the neighbour centred on our edge, `offset = (curW − toW) / 2`). Each snap point
  shows a tick on the edge and names itself in the context bar ("Aligned", "Centred", "Flush right") so the
  slide is legible, not magic. A new connection **defaults to the sensible one** (centred if the neighbour is
  narrower, else 0), never a jarring corner overhang.
- **Both add-gestures.** Click a ghost arrow → a **map picker** (all 248, glitch ids named) → the
  connection is created at that edge with offset 0 (or a sane centred default). *And* a **maps rail/picker**
  (the Characters-bar pattern from 4c) whose entries can be **dragged onto an edge arrow** to create the
  connection there.
- **Select / delete / edit** like every other object: click the neighbour (or a small handle chip) to
  select; a **✕ delete** and **✎ edit** (opens the Details panel on this connection).

**Exit:** all four edges show an add-arrow; a connection renders as its real neighbour and slides to set
the offset; add/delete/select behave like warps and signs.

---

#### Phase 7c — The Details panel (the Connection inspector)

> **BUILT (2026-07-15, `0.39.3-alpha`).** The DetailsPanel gains a connection section: neighbour
> picker (`setConnectionMap`), offset spinner + one-tap **snap buttons** (corner-aligned / centred /
> flush), **re-home** direction buttons (free edges only), a **Break sync** switch that unlocks the raw
> nine (`connectionFields` / `setConnectionField` — a raw write moves exactly its own byte(s) and
> desyncs), Delete, and the live/restored-on-re-entry honest note. `tst_connections` 16/16,
> `tst_qml_screens` 16/16. ⏳ Owed: the on-canvas **resize nodes** for a desynced strip (7d) and
> Twilight's live pass.

Nothing selected → the map (as always). A connection selected → its inspector, on the field kit.

- **The two real inputs, up top:** **Neighbour map** (a real map picker) and **Offset** (a slider + numeric
  field over the legal range, hack values flagged in words, never refused). These drive everything.
- **Direction / re-home.** A N/S/E/W control that **re-homes** the connection to another edge (moves the
  flag bit + slot, recomputes for the new direction). This is the "attach to another side" option — explicit,
  never automatic, and only ever what the bytes can express (no rotation).
- **The raw nine, always shown.** `mapPtr`, `stripSrc`, `stripDst`, `stripWidth`, `width`, `yAlign`,
  `xAlign`, `viewPtr` (the v1 fields — Map ID · UL Corner=`viewPtr` · Strip Src/Dest/Width · X/Y Align),
  each **against what the macro computes**. **Read-only while synced** (they track the offset); a
  **break-sync** toggle (or entering a different value → an *alert offering to break sync*, §9.2) makes them
  independently editable for power users and glitch connections. A **Recompute** action re-syncs.
- **Delete** button at the foot, as the sprite/warp/sign panels have.

**Exit:** every connection byte a hex editor shows is here, in English, with its range — set the easy way
(offset) or the raw way (break-sync), and the panel says which.

---

#### Phase 7d — Handles that match the sync state  *(Twilight's "simpler nodes when synced")*

- **Synced (the default): one simple handle** — the neighbour map slides along the edge (7b). That is the
  *only* gesture, because width/src/dst are derived; offering to resize a value the offset controls would be
  a lie about what you're editing.
- **Desynced (break-sync): the richer nodes appear** — an edge handle to resize the **strip width**
  independently, and (advanced) handles to nudge **src/dst**. These write the raw bytes directly and the
  panel shows them diverging from the macro's value, flagged. This is the power path; it never appears
  until the user deliberately breaks sync.

**Exit:** the canvas offers exactly the handles the current sync state can honestly honour — simple when
synced, more when the user has opted into raw editing.

---

#### Phase 7e — Honesty, tests, review

- **The linchpin note, reused.** An edited connection is **live on Continue** (same persistence linchpin as
  warps/signs — `LoadMapHeader` bails on the saved header, so the save's own connection blocks are what the
  console runs), and the game **restores the map's original connections when the player leaves and
  re-enters**. Say it in the panel and status bar, verbatim in spirit to the warp/sign note.
- **`tst_connections`** — negative-controlled: add writes only the flag bit + slot; delete clears only the
  bit; a drag writes only the offset-affected bytes of the one slot; load+resave of an untouched save
  changes nothing; and **map id + offset → the macro's eleven bytes for a sample of real headers**
  (Route 4→S→Route 3 `−25`, Route 11→E→Route 12 `−27`, Viridian→N→Route 2 `+5` — from the reference note).
- **Render parity.** `tst_emu_parity` already proves the *ring* the strips produce matches the console
  byte-for-byte; a drag must leave that oracle green for the new offset.
- **The mandatory screenshot review** at the ghost-arrow state, a live full-neighbour connection, and mid-
  drag; then the live pass with Twilight (`--sav … --screen map`), since the drag/rotate/handles are things
  a still PNG cannot review.

**Exit of Phase 7:** a person can add a connecting route by clicking an edge and picking a map, slide the
neighbour to set the offset without ever knowing what a strip is, re-home it to another side, delete it,
and — if they want — break sync and edit all nine raw bytes; and the save changes by exactly the bytes each
action names.

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
| ~~**Connections**~~ | ✅ **BRIEFED 2026-07-15 — graduated to Phase 7.** | Was held here until Twilight briefed it. She now has: offset-driven editing with auto-derive, the full neighbour map draggable on the canvas, both add-gestures, explicit re-home (no rotation), delete + inspector. See **Phase 7** above and [`../reference/map-connections.md`](../reference/map-connections.md). |
| ~~**Living / wandering NPCs**~~ | ❌ **DECLINED 2026-07-15** — Twilight: *"don't bother with the living wandering NPCs, it wouldn't be good for different reasons."* No wandering / NPC-AI. **But she does want the connecting maps' sprites SHOWN, static** (see the "neighbour sprites" item under Phase 7b). | — |
| ~~**Phase 8 — Encounters**~~ | ✅ **BRIEFED + BUILT 2026-07-15 — the Wild Pokémon panel.** | Twilight briefed it: a left-dock panel, grass + water, enable + encounter-chance slider, the ten slots drawn like the Pokémon box (percent, editable level, species picker, drag-to-reorder), random-seed on enable, never-clear on disable. The research caught a real model bug (species/level byte order). See **Phase 8** below and [`../reference/wild-encounters.md`](../reference/wild-encounters.md). |
| ~~**Character state (`AreaNPC`)**~~ | ✅ **BRIEFED 2026-07-15 — graduated to Phase 9.** | Twilight briefed the right-hand Character panel (the 9 map-global NPC/control/battle flags). Researched: [`../reference/npc-character-state.md`](../reference/npc-character-state.md). See **Phase 9** below. |
| **Area-State leftovers** | the `AreaWarps` state that isn't warp-flow, `AreaLoadedSprites` | Still "the leftovers", which is not a design. Un-briefed (was bundled into the old "Phase 9 — Area State"). |
| **Phase 10 — Tileset & Blocks** | the deep pass | The panels exist; the *deep* pass does not have a brief. |

**What the earlier text in this file says about these is a sketch and carries no authority.** Read it as
"here is what the bytes are", never as "here is what we agreed."

**What IS briefed and safe to build:** Phase 5 (warps), Phase 6 (signs), **Phase 7 (connections —
briefed 2026-07-15)**, and **Phase 9 (Character State — briefed 2026-07-15)** — but Phase 9's *UI* is
**gated** on its research phases finishing first (the model-fix and the emulator probe; see Phase 9
below), and **Phase 8 (Encounters — briefed + built 2026-07-15)**. The Area-state *leftovers* (9b)
remain un-briefed. Where a briefed phase genuinely *needs* one of them, it **reads** it; it does not
build a UI for it.

---

### Phase 8 — Encounters  *(the Wild Pokémon panel — briefed + BUILT 2026-07-15, Twilight)*

> **BUILT.** `WildPokemonPanel.qml` (left dock) → two `WildMonList.qml` sections (grass, water). Each:
> an **Enable** switch (`rate > 0`), an **encounter-chance** slider (Low↔High), and the ten slots drawn
> like the Pokémon box — the slot's fixed **percent** upper-left (pokered's own
> `WildMonEncounterSlotChances`), an **editable level** upper-right, the mon's artwork centre, its name
> below. **Click** a slot → the species picker (the Pokémon-details `SelectSpecies` list); **drag** a
> slot → reorder, which re-weights rarity. **Enabling a blank table seeds ten random mons** (the box's
> new-random); **unchecking only disables — it never clears** (Twilight, 2026-07-15). `MapModel` gained
> rates/enable, `grassMons()`/`waterMons()`, per-slot species+level setters and `moveGrassMon`/
> `moveWaterMon`, each writing only the byte(s) it names.
>
> ⚠️ **The research caught a real save-model bug.** The save stores each slot as **`level, species`**
> and the model read species-first — inverting every real save. Fixed (a phase of its own, before the
> UI), pinned by `tst_area_pokemon::wildTables_byteOrderIsLevelThenSpecies`. Full write-up + the
> Continue-persistence finding: [`../reference/wild-encounters.md`](../reference/wild-encounters.md).
>
> ⏳ **Owed: Twilight's live pass** — the drag-to-reorder, the click-to-pick species, and the inline
> level edit are things a still PNG can only partly review. (Screenshot: `tmp/screenshots/wild-panel2.png`.)

The original sketch (kept for the record): species picker + level, drag-to-reorder, rate 0 said in
words ("no wild Pokémon here"), and a link to the Grass/Water Meaning layers so the panel and the map
agree. The **post-battle cooldown flag** (`wildEncounterCooldown`, once wrongly bundled here as
`pauseMons3Steps`) is a *different* byte and lives on the map-details page — see
[`../reference/wild-encounter-cooldown.md`](../reference/wild-encounter-cooldown.md).

---

### Phase 9 — Character State  *(the right-hand Character panel — briefed 2026-07-15, Twilight)*

> Twilight: *"There needs to be a panel on the right of the map screen that houses character options
> for all characters."* — the nine map-global `AreaNPC` flags: Face-away-on-interaction, scripted
> movement init/running, ignore controls, scripted controls, scripted battle, trainer battle, and the
> trainer pointer (plus the hidden trade-center bit). **Research done this session** and written up in
> [`../reference/npc-character-state.md`](../reference/npc-character-state.md).

**What these are — the one line that changes the design:** they are **not per-NPC**. Every one is a
single **map-global** bit (or a 2-byte word) that governs how *all* the map's characters behave. So
this is a **map-wide state panel**, a sibling of the other Area-state panels — **distinct** from the
per-object NPC editing in Phase 4b (select/drag/add/delete a sprite on the canvas). The two never
merge: 4b edits *a character*; Phase 9 edits *the map's character rules*. It lives as a **dock panel
opened from the right** (one panel at a time — the chassis doctrine; it does not become a second
permanent rail beside the Characters bar).

**The research already overturned the sketch** ([`npc-character-state.md`](../reference/npc-character-state.md)):
every field is transient script/battle/link scratch; **three v1 labels are wrong** ("Scripted Battle"
is really the **debug** test-battle flag; "Scripted Controls" is the scripted-movement *state* bit;
"Face Away" is really *don't-turn-to-face*); the two "Sprites" bits are **zeroed on every load**
(console-verified); and v2's `areanpc.{h,cpp}` carries all of v1's wrong names. So this is another
**truth-in-labelling + persistence** feature in the warp/sprite/player mould, and it is built in
**four ordered sub-phases — the UI is the last one, and it is gated on the two research phases.**

#### Phase 9a — Research  ✅ *(done 2026-07-15)*

The reference note exists, the real names/bytes/bits are pinned against the disassembly, the model
bugs are listed, and the persistence questions are framed. Nothing to build here — it is the gate the
rest pass through.

#### Phase 9b — Model fix (rename + persistence doc)  ✅ *(done 2026-07-15, `areanpc.{h,cpp}`)*

Renamed `AreaNPC`'s nine fields to the truth (`npcsFaceAway`→`npcsDoNotFacePlayer`, `runningTestBattle`
→`testBattle`, `joypadSimulation`→`scriptedMovementActive`, etc.) and documented the console-verified
persistence in the header — the `AreaPlayer`/`AreaWarps`/sprites pass. Byte offsets and bits unchanged,
so save output is **byte-identical** (proven). `setTo()`/`randomize()` are no-ops (no loaded gun). New
`tst_area_npc` pins each field's offset+bit and proves byte-exact fidelity; the one QML consumer
(`tst_brg_chain.qml`) updated. **All affected tests green** (tst_area_npc 5, tst_area 12,
tst_area_fragments 6, tst_area_pokemon 5, tst_qml_brg 11).

#### Phase 9c — Emulator probe  ✅ *(done 2026-07-15, `scripts/emu/probe_npc_character_state.py`)*

Stamped all nine, booted the real ROM, read back. The result **overturned the source-read**: it is not
"all transient bits vanish". **Four are rewritten on load** — the two Sprites bits (whole-byte wipe) and
*Ignore player input* / *Scripted-movement active* (cleared to return the controls); **five keep the raw
value** (*Scripted movement init/running*, *Trainer battle*, *Test battle*, *Trainer pointer*). Output
quoted verbatim in [`npc-character-state.md`](../reference/npc-character-state.md) §5; the `⚠`/`kept`
marks are now `✅`. (This is exactly why the probe is a gate — the guess was wrong.)

#### Phase 9d — The Character panel  ✅ *(built 2026-07-15, `CharacterStatePanel.qml`; awaiting Twilight's live pass)*

A right-dock panel (`id: "charstate"`, glyph ☺, wired in `Map.qml` + `app.qrc`), three titled groups
(v1's own split), each flag a `MapSwitch` with a one-line plain-English blurb and, where the game
rewrites it, an inline `MapWarnIcon` (**no hidden fields** — Twilight 2026-07-15; the ⚠ is a label, not
a reason to hide):

- **Sprites** — *Don't face the player when talked to* (⚠ zeroed on load), *Trade-center sprites have
  faced* (⚠ zeroed; link-trade only). *(The v1-hidden trade-center bit is now a first-class row.)*
- **Controls** — *Scripted movement — starting* / *— running* (kept), *Ignore player input* /
  *Scripted-movement active* (⚠ cleared on load).
- **Battle** — *Trainer battle queued* (kept), *Test battle (debug)* (kept), and **Trainer pointer**
  (`wTrainerHeaderPtr`, kept). **Researched + resolved by design** (reference/npc-character-state.md
  §4a): the pointer is transient scratch the game never reads from a save (the clean `BaseSAV` holds a
  WRAM-range *leftover*), and a "named trainer" picker would be false precision needing per-map ROM
  header addresses we don't carry. Honest treatment shipped: explained plainly, **full-range hex** kept
  (nothing refused), plus a one-click **Clear** for the leftover. A resolved picker is *not* owed here;
  if ever wanted it's its own briefed feature gated on a trainer-header extraction.

`tst_qml_screens` green (16/16). Screenshot-reviewed; then opened in the foreground for Twilight's live
UI/UX pass (she owns the layout/wording/glyph decisions).

> **Area-State leftovers** — the non-warp-flow `AreaWarps` state fields and `AreaLoadedSprites`
> (`loadedSetId` + 11 slots) that used to share the old "Phase 9 — Area State" heading are **still
> un-briefed** and live in §12b. They are **not** part of this Character-state work.

---

### Phase 15 — Map Storage  *(the right-dock per-map persistent-state panel — briefed 2026-07-15, Twilight)*

> A content phase, numbered after the tail; it sits beside Phase 9 (a map-wide state panel), not with
> the polish phases. Briefed this session and researched: [`../reference/gym-safari-state.md`](../reference/gym-safari-state.md).

> Twilight: *"a new maps panel that will be expanded later on the right toolbar … the panel needs a
> map dropdown pre-selected to the current map … have the panel titled **Map Storage** … the [dock]
> button needs to be a primary‑colour, filled‑in storage icon."*

**What it is.** A right‑dock panel (chassis doctrine: one panel at a time, opened from the right rail)
titled **Map Storage**, whose **dock/tool‑rail icon is the primary colour, filled, with a storage
glyph** — signalling it holds *persistent* save storage. A **map combo box** at the top, **pre‑selected
to the save's current map**, lists **only maps that actually have stored bytes**; selecting a map shows
**that map's storage page**. Twilight will *add more maps' storage over time* — the panel is the
container, seeded now with the three locations we have.

**The bytes are GLOBAL, not Area** — the one thing the research changed. These six do **not** live in
the per‑map Area block; they're single global Main‑Data event bytes the game associates with one
location each. So the panel is a **view** that groups global bytes under their owning map — **model home
is a new global class** (e.g. `WorldMinigames`/`AreaMinigameState`), surfaced on `MapModel`; it is
**not** `Area*`. (v1 filed them under `AreaPlayer`/`AreaPuzzle`, which is what made them *look* like map
state.)

**The three seed pages** (addresses + full write‑ups in the reference note; all offsets verified, five
console‑verified durable):

| Map page | Field | byte | notes |
|---|---|---|---|
| **Vermilion Gym** | Trash‑can switch 1 | `wFirstLockTrashCanIndex` `0x29EF` | persistent; sensible range 0–14 (even) |
| | Trash‑can switch 2 | `wSecondLockTrashCanIndex` `0x29F0` | persistent |
| **Cinnabar Gym** | Next wrong‑answer opponent | `wOpponentAfterWrongAnswer` `0x2CE4` | persistent |
| **Safari Zone** | Steps left | `wSafariSteps` `0x29B9` | persistent; **2‑byte BIG‑endian**, fresh 502, range 0–502 |
| | Safari balls | `wNumSafariBalls` `0x2CF3` | persistent; range 0–30 |
| | Game‑over flag | `wSafariZoneGameOver` `0x2CF2` | **shown, marked TEMPORARY** — console‑verified inert (zeroed every frame by `OverworldLoop`→`SafariZoneCheck`) |

**Persistent vs temporary (Twilight's two‑zone idea, now with a real split).** Five bytes are durable
and wear the **persistent** styling (primary‑filled storage marker). `wSafariZoneGameOver` is **shown,
marked temporary** — it stays on the Safari Zone page but visibly flagged inert (editing does nothing;
the probe proved it). This is the amber‑`!`/"reloaded" idiom, applied to exactly one byte.

**Doctrine, as everywhere:** full byte range, **hack/glitch values shown never refused**; every setter
writes **only** its own byte(s) (the steps word touches exactly `0x29B9`+`0x29BA`, high byte first); the
page states the **armed window** plainly (e.g. "only takes effect while your save is inside the Safari
Zone — the gate resets steps/balls to 502/30 on entry").

**Sub‑phases (the Phase 9 mould — UI last, gated on research+probe):**
- **15a — Research** ✅ *(2026‑07‑15)* — [`../reference/gym-safari-state.md`](../reference/gym-safari-state.md).
- **15b — Console probe** ✅ *(2026‑07‑15, `scripts/emu/probe_gym_safari_state.py`)* — 5/6 durable;
  addresses + big‑endian pinned; `wSafariZoneGameOver` inert.
- **15c — Model** ⏳ — a global class holding the six with byte‑exact accessors (big‑endian word for
  steps), surfaced on `MapModel`; `tst_map_storage` byte‑diffs each setter over the whole 32 KB.
- **15d — The panel** ⏳ — `MapStoragePanel.qml`, right dock, primary‑filled storage dock icon, the map
  combo (only‑maps‑with‑storage, current preselected), the per‑map pages, persistent vs temporary
  marking; screenshot‑reviewed then a live pass.

**✅ Safari Zone resolved (Twilight 2026‑07‑15): "combine Safari Zone".** The several safari maps
(Center/East/North/West, rest houses, gate) collapse to **one combined "Safari Zone" entry** in the
combo — the shared counters live on that single page, not repeated per safari map. Design fully locked.

---

### Phase 17 — TRADES, TOWNS, the "completed" one‑shots, the FOSSIL + the two GROUP kinds  *(briefed 2026‑07‑17, Fairy Fox)*

The design of record for the batch of persistent‑storage features briefed across 2026‑07‑17. All the
research + console verification is done and lives in the reference notes; this is the UI/model design
that sits on top. **Content phase; sits beside Phase 15 (Map Storage), not the polish tail.**

**Research + probes (all ✅, all this session):**

| Feature | Reference note | Probe | Model status |
|---|---|---|---|
| In‑game trades (10) | [`in-game-trades.md`](../reference/in-game-trades.md) | `probe_in_game_trades.py` — durable, spare bits preserved | `WorldTrades` correct; `TradesDB`+`trades.json` extended additively (map/coords/bit) |
| Town visited (11) | [`town-visited.md`](../reference/town-visited.md) | `probe_town_visited.py` — durable **except current town re‑marks** | `WorldTowns` correct; `fly.json` ind **fixed** |
| Rods/Lapras/starter/nurse/guards/elite4 (8) | [`world-completed.md`](../reference/world-completed.md) | `probe_world_completed.py` — 8/8 durable, clears surgical | `WorldCompleted` correct; `defeatedLorelei`→`startedElite4` **renamed** |
| Fossil item + result | [`fossil-revival.md`](../reference/fossil-revival.md) | `probe_fossil.py` — durable, the two bytes **independent** | `WorldOther` correct |

**Where each lands (leadership's own placement calls):**

- **Trades** → the map page of their trader's tile, as a **canvas tab** (kind `trade`, half‑block,
  dashed‑family) + a **Trades section** in the Map Storage panel. 9 located; **CHIKUCHIKU (unused, no
  coords) → the General page**. Cinnabar Lab Trade Room carries **two** (DORIS + CRINKLES).
- **Town "visited"** → a **checkbox near the TOP** of that town's page, on the **11 city maps only**
  ("for places that take a visited one"). ⚠️ The current‑town re‑mark trap wears the amber‑`!`
  *dynamically* — only on the row for the map the save is parked on, and only when saved outdoors.
- **Rods** → the three rod houses (Guru tile). **Lapras** → Silph 7F (1,5). **Starter** → Oak's Lab
  (+read on Red's House). **Nurse (ever‑met)** → **shared across every Poké Center**. **Saffron guards**
  → shared across the 4 gates. **startedElite4** → Lorelei's Room + Lobby, **no x/y, no box** (a page
  entry only) with a caution that it *arms an Elite‑4 wipe*.
- **Fossil** → Cinnabar Lab Fossil Room. Show **both** bytes; ⚠️ **sync neither** (console‑proven
  independent) and **do not warn on a mismatched pair** (BaseSAV ships one — resting state).

**🔑 THE TWO GROUP KINDS (leadership, 2026‑07‑17) — the reusable mechanism this phase introduces:**

| Kind | Means | Affords | Members |
|---|---|---|---|
| **Shared** *(exists)* | ONE save bit on several map pages | the same switch, wherever relevant | Silph Co · Saffron guards · starter · the nurse flag |
| **Alike** *(NEW)* | DIFFERENT bits, same *kind* of thing, one per place | **view the whole group at once** (one click) · **check / uncheck all** | Towns (11) · Trades (10) · **Hidden items (54)** · **Hidden coins (12)** · Rods (3) |

> *"group alike things … a click can allow you to view all of the towns together which also allows you
> to do another thing groups can do check and uncheck all … We should do this for other data but i
> cant think of any now."*

- **Alike is a general mechanism, not a towns feature.** It ships with **five** members from the start
  (above). ⚠️ **Hidden items and coins are TWO separate alike groups, never one** (leadership) — same
  ruling as the separate‑arrays rule in [`map-storage-locations.md`](../reference/map-storage-locations.md) §2c.
- **A group header** offers: expand‑to‑view‑all (renders every member together, cross‑map), and a
  **check‑all / uncheck‑all** control.

**Sub‑phases (the Phase 9/15 mould — UI last, gated on research+probe, screenshot‑reviewed, live pass):**

- **17a — Research + probes** ✅ *(2026‑07‑17)* — the four notes + four probes above.
- **17b — Data + DB** ✅ *(2026‑07‑17, `0.42.x`)* — `fly.json` fixed & pinned; `startedElite4` renamed
  (byte‑identical); `trades.json` extended **additively** (0 existing fields changed) + `TradesDB`
  gains map/coords/bit + `MapDBEntry::toTrades`; pinned by `tst_db_integrity` (15/15).
- **17c — MapModel surface** ⏳ — `storageTrades` / `storageTowns` / `storageCompleted` / `storageFossil`,
  the **General page** in `storagePages()`, the `trade` kind in `blockHotspots()`, and the group model
  (each row carrying its `group` id + kind). `tst_map`/`tst_world` byte‑exact keystones.
- **17d — The panel** ⏳ — the new sections in `MapStoragePanel.qml`, the General page, the town Visited
  checkbox, and the **alike‑group header** (view‑all + check/uncheck‑all). `MapSwitch`, never
  `FlatToggle` (the blank‑panel trap). Screenshot‑reviewed.
- **17e — Canvas** ⏳ — trade tabs on the trader tiles (dashed‑family; Cinnabar Trade Room shows two).
- **17f — Live pass** ⏳ — Fairy Fox drives it: the group view‑all/check‑all, the dynamic town `!`, the
  fossil independence, drag/scroll/feel a still PNG cannot judge.

⚠️ **Doctrine, as everywhere:** full byte range, hack/glitch shown never refused; every setter writes
**only** its own bit(s); a derived value is synced by default **except the fossil, which is not derived
and must not be synced**; `legal` ≠ `armed`, so no cry‑wolf warnings (the mismatched‑fossil‑pair and
`dungeonWarpDestMap` lessons).

---

### Phase 10 — Tileset & Blocks, properly

The two panels that already exist get the *deep* pass rather than the re-chrome they got in phase 1:
the tri-state tile animation with its "what this actually does", the grass tile and the three counter slots
via `TileField` + eyedropper, the Strength scratch, and the **Advanced** disclosure (bank / blockPtr /
gfxPtr / collPtr diffed against the cartridge, with Restore). Blocks keeps its click-to-inspect and its
16-tile breakdown, gains the out-of-bounds-block picker, and loses nothing.

---

### Phase 11 — Tools & precision

Place · Eyedropper · Measure · snap modes · nudge keys (1 tile / 1 block / 8 tiles) · align + distribute for
a multi-selection · every action reachable from the keyboard · the shortcut map written down. **A tool is
not done until it has a cursor, a context bar, an empty state and a keyboard path.**

---

### Phase 12 — Motion & polish

Panel slide, chip hover, selection pulse, layer-toggle cross-fade (the overlay *arrives*, it doesn't slam
on), empty states, tooltips on everything that has a name nobody should be expected to know. Then the full
screenshot pass at three window sizes, and the accessibility read (colour is never the only signal — every
layer has a *pattern* as well as a hue, which is already true of the overlay and must stay true of the chips).

---

### Phase 13 — The verification pass

- Full `ctest` (incl. `tst_emu_parity` against the real cartridge).
- **A byte-diff harness run over every edit the screen can make**: load a save, perform the edit, diff the
  whole 32 KB. Anything that changed and shouldn't have is a **release blocker**, not a bug report.
- Linux Docker (standard / asan / xvfb / coverage).
- A live pass with Twilight on the built app.

---

### Phase 14 — The notes pass

`status.md`, `ui-patterns.md` (the new chassis, dock, layer-row and field-kit conventions),
`qt-patterns.md` (whatever Qt tried to do to us), `decisions/architecture.md` (the layer/object models),
`decisions/rejected.md` (whatever we tried that failed), this file (marked *built*), the session log,
`_nav.dox`, `VERSION`, and the credits check.

---

### Phase 16 — FLAG HOTSPOTS: the boxes that link the canvas to the storage  *(BRIEFED 2026-07-16, Fairy Fox)*

> ⚠️ **Doc defect noted, not silently renumbered:** **"Phase 15" is used twice** in this file — *Map
> Storage* (above) and *SIMULATE* (below). They were briefed a session apart and collided. This new phase
> takes **16** to avoid making it worse; which of the two 15s gets renumbered is a call for leadership,
> not a thing to fix by fiat in a plan they own.

> **The brief, verbatim:** *"you will also have to associate sprites and objects and stuff with the flags
> and have a box around it on the map that you can click which opens the persistent storage panel at the
> thing clicked. so like in oaks lab the pokeballs should have boxes around them because there all tied to
> flags"* … *"Have the outline there even if the sprite isnt there"* … *"sprites and objects and stuff
> should have there own outline, if its tied to [event flags] it should have a different color"*
> (she corrected *scripts* → **event flags** herself).

**What it is.** Every object on the map that the save has a flag for wears an **outline box**. Clicking
the box opens **Map Storage** at *that exact flag* — the canvas becomes an index into the panel. It is a
**layer** (Objects group), toggleable like everything else, per the layer doctrine.

#### The data is ALREADY SHIPPED — this phase adds no JSON and no importer

The research (`scripts/extract_flag_locations.py`: 223 maps, **918 objects**, 226 conditional) went
looking for something `maps.json` **already carries**. Every `MapDBEntry::sprites` entry is a ROM object
with `x`/`y` and, when its visibility is flag-governed, a **`missable`** index — and the whole chain to
the save already exists:

`MapDBEntrySprite::getMissable()` → `getToMissable()` → `MissableDBEntry` → `WorldMissables` (228 bits,
`0x2852`, **bit SET = HIDDEN**).

**Oak's Lab is the brief's own example, and it falls straight out of the shipped data:**

```
(4, 3) Rival        missable 42        (2, 1) Book Map Dex  missable 47
(6, 3) Pokeball     missable 43   <-.  (3, 1) Book Map Dex  missable 48
(7, 3) Pokeball     missable 44     |- the three starters
(8, 3) Pokeball     missable 45   <-'  (5,10) Prof. Oak     missable 49   <- Oak's SECOND spot
(5, 2) Prof. Oak    missable 46        (1, 9) Girl          -- no missable, no box
```

Both things she named — *"the pokeballs should have boxes"* and Oak standing in **two** places — are
already in the data. 8 of Oak's Lab's 11 objects are flag-governed; the Girl and the two Aides are not,
and correctly get **no box**.

#### The thing that makes "even if the sprite isn't there" work

The boxes are drawn from the **ROM cast** (`MapDBEntry::sprites`), **not** from the save's 16 sprite
slots. That is the whole trick: a missable whose bit says *hidden* has no sprite on screen, but its ROM
object still has coordinates — so **the box is still there, exactly where the thing would stand.** This
is what she asked for, and it is only possible because the outline's source is the cartridge's list, not
the save's.

#### Colour + state — attachment is the colour, visibility is the line

**Colour = what it is attached to** (her rule):

| Attachment | Meaning | Box |
|---|---|---|
| **Missable** (`missable` index → `WorldMissables`) | the save decides whether it is here at all | the flag-attached colour |
| **Event flag** (a verified link, `EventsDB`) | tied to a story bit | a **different** colour, per the brief |
| **Neither** | plain scenery — the Girl, the Aides | **no box** (clutter is a bug) |

**Line = the flag's current state**, drawn honestly rather than hidden:

- **Solid** — the flag currently **shows** it.
- **Dashed** — the flag currently **hides** it (missable bit set). The box is still drawn; that *is* the
  brief. A dashed box says "this belongs here and your save has it switched off", which is precisely the
  thing a save editor exists to tell you.

#### Click → the panel, at the thing you clicked

`MapStoragePanel` already has the three sections a click can land in — `scriptSection`, `eventSection`,
`missableSection`. Clicking a box opens the panel (right dock, one-panel-at-a-time chassis rules), scrolls
to the row and **highlights** it. Missable-attached → `missableSection`; event-flag-attached →
`eventSection`. No new panel, no second home for the same truth.

#### The traps, written down before anyone hits them

- **Coordinates are in TILES; `maps.json`'s `width`/`height` are in BLOCKS.** One block = 2×2 tiles. This
  already bit the Phase 6 signs work (a Route 22 sign at `y=11` on a `height=9` map). The box geometry is
  `mapBorder * blockPx + x * 16`, the same arithmetic `MapEngine::playerRect` uses.
- **Bit SET = HIDDEN** (`WorldMissables`), not shown. Getting this backwards inverts every box on screen.
- **The ROM cast and the save's sprite slots are different lists** and are *allowed* to disagree — the
  boxes come from the ROM, the sprites from the save. When they disagree, that is information, not a bug.
- **`missable` is `-1` for non-conditional objects** — that is the "no box" signal, not an error.

#### 16f — MANY STORAGE SPOTS ON ONE LOCATION: the tabbed squares  *(BRIEFED 2026-07-17, Fairy Fox — NOT BUILT)*

> *"keep in mind an item or sprite or whatever may have event flags and/or filter flags both on it. If
> a map tile/block whatever has multiple persistent map storage spots for it they need to show as
> tabbed little squares at the top left that can be clicked on and theyll take you to the different
> spots"*
>
> …clarified immediately after, and the clarification is the important half:
>
> *"multiple persistent map storage spots is what i meant to say, like there's a lot of pieces to the
> game and im talking when multiple persistent storage things not just filter flags and event flags
> land on the same location of the map type thing"*

**So this is NOT "flags on an object".** It is **any persistent storage that lands on the same location
of the map** — filter flags and event flags are just the two we happen to have built. The game has many
pieces, and more storage kinds are coming (she has said repeatedly that Map Storage is a container she
will keep adding to). The rule is about a **place**, not about a sprite.

The design above assumes **one spot, one box**, and that assumption is wrong. A single tile can carry a
filter flag deciding whether something is there at all, an event flag recording what happened there,
a script-progress step, a minigame byte — any mix. A box that links to only one of them is telling part
of the truth.

**The shape.** When a location has **more than one** storage spot, the box grows **little tabbed squares
in its top-left corner**, one per spot, each clickable, each jumping to *its own* row in Map Storage.
One spot = no tabs (a plain box, as built).

**What this means for the architecture — and it is worth getting right now, before more is built on
it:** a box must not be modelled as *"a missable with a rectangle"*, which is what 16c/16d currently
are. It has to become **a LOCATION that owns a list of storage spots**, each spot carrying its kind
(filter flag / event flag / script step / …), its label, and where in the panel it lives. The kinds are
open-ended by design: adding the next kind of storage should mean adding a spot type, not rewriting the
box. `MapModel::flagHotspots()` should therefore grow toward
`{x, y, spots:[{kind, ind, name, desc, section}]}` rather than a flat missable — and the sooner that
happens, the less there is to unpick.

**✅ The route in — Fairy Fox's model, and it is source-verified (2026-07-17).** Full write-up +
counts: [`../reference/event-flags.md`](../reference/event-flags.md) → "WHAT AN EVENT FLAG *IS*".

> *"event flags are by scripts for scripts … filter flags are meant for maps … perhaps an x/y location
> for event flags is unnecessary but for scripts in an x/y location that change event flags, those
> flags should be tabs on the script box"* + *"filter flags also point to scripts which change event
> flags"*

This corrects the assumption the section above was drifting toward, and it explains the thing that had
looked like a dead end:

- **Event flags have NO map location, and never did.** They belong to the code. That is *why*
  `extract_flag_locations.py` found only **14** object↔event links across 223 maps — it was hunting a
  relation that does not exist. **So no event-flag boxes, and no x/y for a flag.** Inventing one would
  be inventing a fact.
- **A SCRIPT can have a location**, and that is the real hook. Two kinds, both counted:
  - **Trigger tiles** — `ArePlayerCoordsInArray` + a `dbmapcoord` table: **41** script files. *These*
    get a script box, and the event flags that script writes are its **tabs**.
  - **Through a filter flag** — a script toggles an object's missable *and* writes event flags in the
    same routine (`ld a, TOGGLE_x` / `predef HideObject` / `SetEvent EVENT_y` — `BillsHouse.asm`):
    **22** of 224 script files do both. So an object's tile → filter flag → script → **its** event
    flags, as tabs on that object's box.

**Counts, so this is planned against reality:** 71 `Show/HideObject` sites · 117 `SetEvent`/`ResetEvent`
sites · **22** files with both · **41** files with coord triggers.

**Therefore:**

- **16f-a — Research: script → event flags, and script → location.** Extract per *routine*: the
  `dbmapcoord` tables (script's tile) and the `SetEvent`/`ResetEvent` it writes; plus the
  toggle→event pairing for the 22. Routine boundaries are the unit — ⚠️ **not** proximity: "the flag
  is near the object" is the same static-co-location reasoning that produced the Route 22 false
  positive and shelved the conflicts system. Anything ambiguous gets **no tab**.
  - ⭐ **THE DESIGN OF RECORD: truthful HIGHLIGHT, block-granular HIT TARGET.** *(leadership,
    2026-07-17 — her third and settled statement on it. This **supersedes both** earlier same-day
    calls: ~~"put a box around the whole range"~~ **and** ~~"highlight the blocks separately, per-block
    or per-tile boxes"~~.)*

    > *"literally 2x2, 4x4, 8x8, 16x16, 32x32 are all fine for measurements for the box you mouseover
    > or click on. So, why dont we highlight truthfully the ranges and have clicks and mouse overs be
    > on the blocks to keep things simple so tabs can be there for ranges affected over block
    > including tiles like 8x8 ranges or 4x4 or 2x2"*

    **The insight is to split one thing into two**, which the whole design had been conflating:

    | | Granularity | Job |
    |---|---|---|
    | **Highlight** | **the data's own** — *"use the measurement its suppose to be"* | Tell the truth about what is affected. A tile attribute lights its **8×8**. An object/warp/sign lights its **16×16 half-block**. A coord range lights **exactly the cells it really covers** — including half a block, if that is the truth. |
    | **Hit target** *(hover + click)* | **the block, 32×32, uniform** | Be easy to hit and simple to reason about. One block = one cursor cell = one tab strip. |

    **This dissolves the "a block box would be a lie" objection** (raised against the unit research
    above, and it was right *on its own terms*): a block box is only a lie if it is **claiming to be
    the thing**. Here it claims nothing — it is the **cursor cell**. The highlight carries the truth;
    the tabs carry the disambiguation. Two warps in one block → **two tabs**. That is what tabs are
    *for*, and it is the same job they already do for an object with both a filter flag and event
    flags.

    **The aggregation rule, and it is the whole model:**

    > **A block's tab strip = every storage spot whose true extent intersects that block** — whatever
    > unit that spot is measured in, finer or coarser.

    So a block can carry, at once: an 8×8 tile attribute, a 16×16 object's filter flag, and a coord
    range that only clips its lower half. All three are tabs; all three highlight at their own real
    size. Pallet Town's `cp 1` (the north row = half-block row 1 = the *bottom half* of block row 0)
    highlights truthfully as that bottom half, while the whole block row is hoverable.

    **Consequences:**

    - **A range is a real extent again — but it is NOT a box.** It is highlight geometry. The earlier
      "fan out into one box per unit" is dead *as a box rule*, and so is "there is no w/h": a spot
      carries its true extent for the highlight. What it does **not** get is its own hit target.
    - **The LOCATION model survives, re-homed onto the block.** `flagHotspots()` →
      `blockHotspots()`: `{blockX, blockY, spots:[{kind, ind, name, section, extent}]}`, where
      `extent` is the spot's truthful geometry in its own unit. Kinds stay open-ended.
    - **The unit stays a property of the STORAGE KIND** — for the *highlight*. It no longer has any
      say over interaction, which is always the block. ⚠️ **There are THREE units, not two** — the
      middle one is where nearly everything positional lives, and it had been mis-called a "tile" in
      these notes. Verified against `pret/pokered`; table + proofs in
      [`../reference/map-storage-locations.md`](../reference/map-storage-locations.md) → "The units".

    ✅ **ANSWERED — who wins the click** *(leadership, 2026-07-17)*. The collision was real: a
    uniform block hit-grid lies over a canvas where sprites, warps, signs and the player are already
    **selectable and draggable**, and the shipped Flag boxes layer deliberately clicks the **ring
    only** so *"clicking the sprite itself still opens its details as before"* (0.42.0-alpha). Three
    candidates went up (layer-gated · object-first · block-always-wins). **She picked none of them:**

    > *"any overlap with selections need to be accessible under mouseover, click order is based on
    > layer order in the panel shown to users. So the first click is highest layer however the square
    > color tabs on top allow directly clicking or dragging and accessing."*

    **The rule, in three parts:**

    | | |
    |---|---|
    | **Hover** | **Everything overlapping is accessible under mouseover.** Nothing is unreachable because something sits on top of it. |
    | **Click** | **Priority IS the layer order the user already sees in the Layers panel** — first click goes to the **highest layer**. Not a new hidden precedence: the panel *is* the z-order, it is on screen, and it is already re-orderable and toggleable by the person clicking. |
    | **Tabs** | **The escape hatch, and they are not just labels — you can click AND DRAG them.** A tab reaches its spot **directly**, whatever is stacked above it. |

    ⭐ **This is better than all three candidates, and worth understanding rather than just
    implementing.** The problem was "an invisible precedence rule fights the canvas". The answer
    doesn't invent a rule — it **reuses one already on screen and already under the user's control**,
    so precedence is inspectable and fixable by the person hitting it. And it retires the
    drag-is-at-risk worry outright: **drag is available on the tab**, so a buried object is never
    undraggable — no need to gate, re-order, or special-case the grid at all.

    **The tab strip's own layout** *(leadership, same message)*:

    > *"there is a gap in the tabs to seperate the tile-based tabs and non-tile-based tabs. door and
    > warp tile traits would be an example of tile tabs seperated from non-tile tabs like filter flags
    > or script locations or coord ranges these would be the tab tags on the left"*

    - **Square colour tabs, tagged on the LEFT** of the block's box.
    - **A GAP splits them into two families**, and the split is the **unit** — which is exactly the
      three-units research made legible:
      - **Tile-based tabs** (8×8 tile *traits*, from the tileset's meaning layer) — e.g. **Door**,
        **Warp tiles**.
      - **Non-tile tabs** (the walk-grid/half-block storage) — **filter flags**, **script
        locations**, **coord ranges**.
    - ⚠️ **So tile traits get tabs too** — which means the tab strip spans the **Tiles** group *and*
      the storage kinds, not just Map Storage. `blockHotspots()`'s `spots[]` must therefore admit
      **tile traits** as a kind, and each spot needs its **unit** so the gap can be placed. This is a
      widening of 16f's scope and it lands *before* 16f-c is built, not after.

    ❓ **Still unstated, and small enough to settle at build time with her in the room:** what the
    square's *colour* means (the layer's own colour is the obvious read — the layer tree already
    assigns one per layer), and the hover affordance's exact shape ("accessible under mouseover" —
    peek, or a strip that lists them). Neither blocks 16f-a/16f-b.

      | Unit | Size | What is measured in it |
      |---|---|---|
      | **Tile** | 8×8 | **tile attributes** — collision, ledges, water, counters, bookshelves, warp tiles |
      | **Half-block** *(the walk grid)* | **16×16 = 2×2 tiles** | **`wYCoord`/`wXCoord`, and therefore: objects → their filter flags · warps · signs · hidden items/coins · script coord triggers** |
      | **Block** | 32×32 = 4×4 tiles | the `.blk` map data, map width/height, the border block, connections |

      **Consequence: nothing we box today is block-scoped, and a block-sized box would be a LIE** —
      it is 2×2 walk squares, so it cannot tell two adjacent warps apart. The boxes already ship at
      16×16, which is **correct**; what is wrong is only the *word* — 16×16 is a **half-block**, not
      a tile. Rename, don't resize. Leadership's *"tile attributes would be examples of tile
      measurements"* is **exactly right** and is the one genuinely 8×8 layer.
- **16f-b — Data.** Out of pret via an importer, never hand-written. (⚠️ `maps.json` is data — a new
  field needs leadership's OK, per the standing "don't edit the JSON" rule.)
- **16f-c — The tabs**, on boxes with >1 spot. Now carries the whole interaction model
  (hover-reaches-everything · click-priority-is-layer-order · click-**and-drag**-from-the-tab) and the
  two-family tab strip split by a gap. ⚠️ **Its model work is bigger than "add tabs":**
  `flagHotspots()` → `blockHotspots()` with `spots:[{kind, ind, name, section, extent, unit}]`, and
  **`kind` must admit tile traits**, because the tile-based tabs come from the **Tiles** group, not
  from Map Storage. Do that restructure first.

Until then, boxes link to the filter flag only — **verified**, and it covers the brief's own example
(Oak's Lab's Poké Balls are filter-flag objects).

#### Sub-phases (the Phase 9 mould — UI last)

- **16a — Research** ✅ *(2026-07-16)* — `extract_flag_locations.py`; the finding that the data was
  already in `maps.json` is the reason 16b is empty.
- **16b — Model** — *nothing to build.* `getMissable`/`getToMissable`/`WorldMissables` already exist and
  are already pinned by tests. Recorded so no one "adds" it.
- **16c — The layer** — the boxes on the canvas: `MapModel::flagHotspots()` (ROM cast → x/y, attachment,
  current state), a QML layer in the **Objects** group, colour by attachment, solid/dashed by state.
- **16d — The link** — click → Map Storage → section → scroll + highlight.
- **16e — Verification** — `tst_map_hotspots`: Oak's Lab yields **8** boxes and not 11; the three balls
  sit at (6,3)/(7,3)/(8,3) with missables 43/44/45; flipping a missable bit flips solid↔dashed and
  **writes only that bit** (the byte-diff keystone); mandatory screenshot review; then her live pass.

---

### Phase 15 — SIMULATE: walk the map  *(**OPTIONAL / stretch** — runs last, gates nothing)*

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

- **15a — Collision, for real.** The passable-tile list is already derived from the tileset's `collPtr`
  (the ROM holds a list of what you *can* walk on — "wall" is what's left; see
  [`../reference/tiles.md`](../reference/tiles.md)), plus the elevation-edge pairs.
- **15b — The step.** Arrow keys move him a tile at a time, at the game's own cadence, and he **turns
  before he steps** (the game turns first — get that wrong and it feels wrong immediately).
- **15c — The camera IS the console's.** The screen box and the draw area follow him exactly the way
  `LoadCurrentMapView` slides them — half-block steps, block-aligned scratch. We already reproduce that
  geometry byte-for-byte; now it *moves*.
- **15d — Ledges, water, doors.** A ledge jumps the way its arrow points; water needs Surf; a warp tile
  lights up and **offers** to take you where it goes — it never warps you unasked, because that changes
  `curMap`, and that is a real edit.
- **15e — The save, honestly.** Walking is a **preview** — it moves the player in the model, and the status
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
   stretch** — Phase 15, last, gating nothing. It is deliberately **not** an emulator: no NPC AI, no
   scripts, no battles, no text.
