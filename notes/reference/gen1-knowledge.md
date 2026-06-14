# Gen 1 (Red/Blue) Save-Format & Game Knowledge

Domain knowledge about *the game and its save file* — distinct from the codebase architecture
(that's `systems/savefile.md`). This is the reverse-engineering and gameplay lore the editor
encodes, mined from the code comments and the in-app tooltips. Read this when working on the
save layer, the databases, or the randomizer, so the editor keeps matching how the real games
actually behave.

Everything here is asserted by the code (and verified against it); inline source comments are
the primary record, and `version.md` shows where several facts were figured out (e.g. the
tile-block map-view VRAM pointer in `14a73be`). **One exception:** the "Map geometry & structure"
section is drawn from an external mapping project (clearly marked there) and is orienting context to
confirm against `pret/pokered` / our `MapsDB`, not yet code-verified.

**Authoritative external byte map:** `assets/saves/structure.bt` (a 010 Editor binary
template, authored by the maintainer) is the full field-by-field layout of the 32 KB save — banks,
structs, bit-fields, offsets. A human-readable companion, `assets/saves/structure.md`, mirrors it
in prose and is included in the generated Doxygen docs (the `.bt` remains the byte-exact authority). Because it was written **independently of the app's C++**, it is the
*oracle* for testing: per-field offset and golden tests can validate against it rather than against
the code under test (which would be circular). Two real saves live beside it as test fixtures:
`assets/saves/natural-clean/BaseSAV.new.sav` (a fresh file saved right after the new-game screen) and
`assets/saves/natural-clean/BaseSAV.sav` (a progressed save; exact progress not recorded — characterize as needed).
Both are exactly `0x8000` bytes. See `plans/testing.md`.

---

## Save file structure (the bytes)

- **Size:** a Gen 1 save is `0x8000` bytes (32 KB) — `SAV_DATA_SIZE`.
- **Species count:** 151 (`maxPokedex`). Pokédex owned/seen are 0-based flag arrays.
- **Internal species index ≠ Pokédex number (the editor already models both).** Party and box
  records store an **internal species index** (Rhydon = `$01`, Kangaskhan = `$02`, … Bulbasaur =
  `$99`), *not* the Pokédex number. **This is built into the editor from the start:**
  `PokemonDBEntry` (`pse-db/pokemon.h`) carries `ind` (the internal index — the byte the save
  actually holds) alongside an `std::optional<var8> pokedex` (the dex number, when assigned) and a
  `glitch` bool; `PokemonDB` is keyed by species name. So the editor reads/writes species by `ind`
  natively and converts to/from the dex only for display. The internal index has **gaps**, and those
  gaps are exactly the **glitch species** the `glitch` flag marks (MissingNo et al.) — the glitch-mon
  support and the internal index are two sides of the same fact, both already implemented. Moves are
  indexed `$01`–`$A5`. (Full tables live in the species/move DBs and the `.bt` oracle; Data Crystal
  is just an external cross-check. Recorded here as background for *understanding* the existing
  design, not as anything to change — the index system is correct and is not to be "fixed.")
- **16-bit values:** stored as two bytes, `0x12,0x34 <=> 0x1234`; reverse the pair for the other
  endianness. Pad to the full width when formatting — a 16-bit `1` is `0x0001`, not `0x1`
  (`0xCAD` → `0x0CAD`). (Item *prices* are 16-bit; an early bug stored them in 8 bits and
  corrupted any price above 255 — `f19ee18`.)
- **Checksum:** 8-bit. The editor uses the efficient form — **start at `0xFF` (255) and subtract
  each byte of the range in order, letting it underflow/wrap around** — which produces exactly the
  value Gen 1 expects (the game itself uses a slower but equivalent algorithm). Recompute on save.
- **Box checksums (bank 2):** each of boxes 1–6 has its own individual checksum, plus one overall
  bank-2 checksum that *excludes* the individual ones. The game doesn't always use or recalculate
  the box checksums, so only recompute them when the boxes are actually touched.
- **"Boxes formatted" flag + when the game actually wipes the boxes (SOURCE-VERIFIED 2026-06-13
  against `pret/pokered`; full writeup: `reference/box-recovery-research.md`).** `0x284C` holds the
  current box index in its low 7 bits; **bit 7 is `BIT_HAS_CHANGED_BOXES`** — literally *"has the
  player ever changed PC boxes"* (`wCurrentBoxNum`), which the editor exposes as `boxesFormatted`.
  The 12 boxes live at `0x4000` (boxes 1–6 + checksums) and `0x6000` (boxes 7–12 + checksums); the
  active box is cached at `0x30C0`, and that cache is what's edited/loaded regardless of the flag.
  - **Only two routines ever write the 12 boxes:** `ChangeBox` (PC box switch) and
    `ClearAllSRAMBanks`. Normal saves + new game write only `sGameData` (which **excludes** the
    boxes), so **a new game does NOT wipe them** — old box data persists in SRAM.
  - **The in-game box "format" is NOT a real erase.** On the *first* box switch (flag off),
    `ChangeBox` calls `EmptyAllSRAMBoxes`, which per box writes only **2 bytes** — count → `0x00`,
    first species → `0xFF` (list reads empty) — then recomputes checksums. **Every mon record / OT /
    nickname below the header stays physically intact.** The only true full wipe is
    `ClearAllSRAMBanks` (fills all SRAM with `0xFF`), reached **only** via the secret "Clear saved
    data?" screen answered YES.
  - So unformatted boxes very likely hold **real, recoverable Pokémon**, and they survive even the
    in-game box switch (until new deposits overwrite the records or clear-save runs). A **recovery
    feature** is viable: Tier 1 = still-intact + checksum-valid boxes (high confidence); Tier 2 =
    game-emptied boxes whose records survive (heuristic, checksum useless). Keep it quarantined in a
    fail-closed opt-in scanner, never in the main editor. Details + design stance:
    `reference/box-recovery-research.md`. Do not build without the maintainer's go-ahead.
- **Hall of Fame:** records are `0x60` bytes each and start at `0x598`; record *N* lives at
  `0x598 + N*0x60`.
- **Map-view VRAM pointer:** fixed at `0x9800` (`VramBGPtr`, the GB background-tilemap pointer —
  it never changes); the saved map-view VRAM pointer is just set to it.
- **Badges:** stored big-endian, one owned-flag per badge.
- **Font/text values start at 1:** to map a requested character value, offset by 1.
- **Sprite screen position:** pixel-aligned to a 4-pixel offset from the tile grid.
- **DVs / stats are unsigned and can *intentionally* underflow** — editing below certain
  thresholds wraps around by design.
- **A "+3 offset flag"** exists in the data whose purpose is still unknown (documented as such in
  the source — leave it alone unless you learn what it does).

---

## Gameplay rules the editor encodes

These come from the in-app tooltips (the user-facing explanations) and the model logic:

- **Glitch content is available on purpose.** Glitch species (including MissingNo), glitch moves,
  and glitch maps are all selectable in the editor — the data models carry an explicit `glitch`
  flag and the pickers list a "--- Glitch ... ---" section.
- **Trade status:** a Pokémon is "traded" (i.e. *not yours*) when its **OT name + OT ID don't
  match the player's** name + ID. The OT ID is a 4-hex-digit value (each char `0–9`/`A–F`).
  Because of this, editing the player name/ID **cascades** to keep every non-traded mon yours —
  which is exactly why those writes must be atomic/commit-on-finish (see
  `reference/player-name-hang.md` and `reference/ui-patterns.md` → "Commit edits on FINISH").
- **Nickname semantics:** a name counts as a *nickname* only if it isn't the species name in ALL
  CAPS. Nicknames are difficult to change in-game and are **unaffected by evolution**.
- **Level:** 1–100. **Below level 5 can trigger underflow bugs** (a genuine Gen 1 issue) — the
  editor warns about it rather than forbidding it.
- **Catch rate:** unused leftover garbage from when the species was a wild battle encounter. It
  still exists in the data and is editable, but does nothing in normal play.
- **Natures (retroactive):** natures didn't exist until Gen 3. Game Freak later published a
  formula to derive a Gen 1 mon's nature **from its IVs/DVs**; the editor surfaces this as a
  read-only **"future nature."**
- **Shininess (retroactive):** shininess didn't exist in Gen 1. Gen 2 defined a way to read
  shininess from Gen 1 DV data, and that convention has been kept since; the editor exposes a
  **"future shiny"** toggle. **Toggling it rewrites DV bytes**, which is why the checkbox uses
  `onToggled` (real user action) and not `onCheckedChanged` (see `StatsTab.qml`).
- **Status condition:** the sleep status also carries a remaining-turns count.
- **Currency / stack limits:** money max **999,999**; casino coins max **9,999**; item stack max
  **99**.
- **Player starter:** recorded from the starter you chose, but (per the tooltip) probably never
  actually used in gameplay.
- **Rival starter:** chosen at the start of the game; it **determines the rival's whole team** —
  which Pokémon grows alongside him.
- **Name byte budget:** person names are *meant* to be ≤ 7 characters, but there are 10 bytes
  total. The name editor warns at 7, warns again at 10/10 used, and warns when a name **expands
  wider on screen than its byte count suggests** (some single bytes render as multiple tiles).

---

## Place-specific state ("world local")

A handful of save values are tied to *specific maps* rather than to the player or world globally.
The map browser deliberately surfaces extra category icons for exactly these maps
(`MapDetails.qml`'s special-map list):

- **Daycare** (map 72) — the single deposited Pokémon. (Daycare is treated as a likely
  frequently-used "map" even though it isn't the current one.)
- **Vermilion Gym** (map 92) — the two Lt. Surge trash-can switch positions.
- **Cinnabar Gym** (map 166) — the gym quiz's next opponent.
- **Safari Zone** (entrance map 156 plus the zone's sub-maps 217–225) — the Safari game variables
  (steps/captures, etc.). *(Geometry aside: the Safari Zone's sub-maps are **not physically
  consistent** — there is no layout in which every connecting warp lines up spatially, which is why
  the area feels disorienting. Noted by an external mapping project; see "External map references" below.)*

---

## Map geometry & structure (overworld + connected maps)

The editor's Maps feature (DB entries, `WarpData`/`AreaWarps`, `MapConnData`, `SpriteData`,
`AreaTileset`) models the same map system Game Freak shipped. The facts below describe how that
system is laid out. **Provenance:** most of this comes from an external full-game mapping project
(see "External map references" below), *not* from this editor's own code — so treat it as
orienting domain knowledge to confirm against `pret/pokered` and our `MapsDB`, **not** as
code-asserted truth. Where it touches a value the save actually stores, that's called out.

- **Three nested grid units (now resolved).** Gen 1 maps use three sizes, smallest to largest:
  - **tile** = 8×8 px — the smallest unit; what the tileset graphics are stored as.
  - **movement cell** = 16×16 px = **2×2 tiles** — what the player actually walks on ("the player
    moves on a 2×2 tile grid," per Data Crystal). Some sources call this a "square."
  - **block** = **4×4 tiles = 32×32 px = 2×2 movement cells** — the reusable metatile a map is
    assembled from; one map byte = one block index.

  ✅ **Collision resolved.** Two independent authoritative sources — Peter Hajas's parsing article
  and the Data Crystal wiki — both define a **block as 4×4 tiles (32×32 px)**, so that's the truth.
  vjeux's rendering post loosely called the 2×2-tile *movement cell* a "block" (dividing warp coords
  by 2 to reach it); that was his own informal usage, **not** the game's. My earlier "a block is 2×2
  tiles" note was wrong — the maintainer's "not completely sure" instinct was right. **Standardize
  `MapsDB`/`AreaTileset`/`AreaMap` on: tile / movement-cell / block (4×4 tiles).**
- **The 2×2-cell property quirk (Old Man Glitch).** Because movement happens per 2×2-tile cell, the
  game checks the cell's **bottom-left tile** for properties like grass and talking-over — *except*
  it checks the **bottom-right** tile for water, and that inconsistency is the root of the Old Man
  Glitch. (Data Crystal.) Likely the real source of our existing "Sprite screen position:
  pixel-aligned to a 4-pixel offset" and "+3 offset flag (purpose unknown)" notes too — see the
  reconciliation flag in "Object data" below.
- **Map headers = tileset + directional connections.** Each map declares one **tileset** and up to
  four **connections** (north / south / east / west) naming the neighboring map. (In `pret/pokered`
  source form: `map_header PalletTown, PALLET_TOWN, OVERWORLD, NORTH | SOUTH` followed by
  `connection north, Route1, ROUTE_1, 0`.) This is exactly what our `MapConnData` / `MapDBEntry`
  connection fields represent; the whole Kanto overworld is one connected graph reachable by walking
  the connections out from Pallet Town.
- **Tilesets are per-map and there are several.** Beyond `OVERWORLD` there's `PLATEAU` (the route to
  Indigo Plateau), `CAVE`, interiors, etc. Picking the wrong tileset renders a map as garbage —
  corroborates why our `MapSearch::isType("Cave")` / `isType("Outdoor")` type strings matter (60 /
  38 maps match — see `status.md`).
- **Many "rooms" are separate maps stitched at warps.** Buildings, caves, and multi-section
  locations are individual maps connected only by warps, not a single big map. Concrete cases worth
  knowing because they show up in our map list and special-map handling:
  - **S.S. Anne** is split into many separate room-maps.
  - **Sabrina's Gym** (Saffron) is teleport-puzzle sections, **4 teleports each**.
  - **Safari Zone** sub-maps are geometrically inconsistent (see the "world local" note above).
- **Hidden items & Game Corner coins have map coordinates.** Both hidden field items and the Game
  Corner's hidden coins are placed at specific tile positions on their maps (the external project
  draws markers at those spots). Relevant if the editor ever surfaces item/coin locations per map.

### How a map is actually stored & parsed (ROM/disassembly data, not save bytes)

This is the byte-level format Peter Hajas reverse-engineered. It's **`pret/pokered` / ROM data, not
anything in the 32 KB `.sav`** — relevant only if a Maps feature renders or edits map layouts, but
worth recording because it's the authoritative breakdown of the units above.

- **Map layout = a `.blk` "blockfile" of block indices.** Each map's geometry lives in a `.blk`
  file ("blockfile") under pokered's `maps/`. Every byte is **an index into that tileset's
  blockset** — i.e. "draw block #N here" — read left-to-right, top-to-bottom. So the file is tiny:
  Pallet Town is **90 bytes** for a 10-block-wide × 9-block-tall map (and on screen that's 20×18
  player-squares, since each block is 2×2 squares).
- **Dimensions are stored height-then-width, in blocks.** `constants/map_constants.asm` lists each
  map as `mapconst NAME, height, width` (e.g. `PALLET_TOWN, 9, 10`). Note the order: **height
  first.** 9×10 blocks × (4×4 tiles/block) confirms the 90-byte count.
- **Tilesets** live in `gfx/tilesets` — ~**19** of them (`overworld`, `house`, `gym`, `forest`, …),
  each shipped as an image plus a compiled **`.2bpp`** file (newer pokered emits PNGs you regenerate
  with `rgbgfx`, e.g. `rgbgfx -o gfx/tilesets/overworld.2bpp gfx/tilesets/overworld.png`).
- **`.2bpp` = 2 bits per pixel, Game Boy 4-gray.** Values: **`00`=white, `01`=light-gray,
  `10`=dark-gray, `11`=black.** Pixels are packed in 8×8 tiles, read top-left→bottom-right.
  **Gotcha:** a pixel's two bits are **spread across two bytes** — bit *i* of byte 1 pairs with bit
  *i* of byte 2. So bytes `00000000 11111111` decode to eight `01` pixels, **not** `00 00 00 00 11
  11 11 11`. (Easy to get wrong when parsing tile graphics.)
- **Blocksets (`.bst`, a.k.a. "metatiles") are the missing link.** `.blk` bytes are too small to
  index the tileset directly; they index a **blockset** in `gfx/blocksets` (one per tileset, e.g.
  `overworld.bst`). Each **block is 16 bytes = a 4×4 grid of tile indices**, stored contiguously,
  top-left→bottom-right, referenced by position/index. End-to-end render: header → tileset (`.2bpp`)
  → blockset (`.bst`, 16 bytes/block) → map (`.blk`, one block-index per byte).

### Map & tileset byte structures (Data Crystal wiki — solid but wiki-sourced)

The Data Crystal "Pokémon Red and Blue/Notes" page gives the field-by-field ROM structures these
all decode from. **It's a wiki — mostly reliable, but verify against `pret/pokered` before trusting
any single offset.** These are the structures our `MapsDB`/`AreaTileset`/`MapConnData`/`WarpData`/
`SpriteData` mirror.

- **Tileset header (12 bytes each; header table pointer `0xC767`):** bank ID of blocks+tiles
  (1) · pointer to blocks (2) · pointer to tiles (2) · pointer to collision data (2) · up to **3
  "talking-over" tile numbers** (3, unused slots `$FF`) · **grass tile** (1, `$FF` if none) ·
  **animation flag** (1).
  - *Collision data* = `$FF`-terminated list of tile numbers the player **can walk on**.
  - *Talking-over tiles* = tiles you can talk across (PokéCenter/Mart counters).
  - *Grass tile* = drawn above sprites **and** triggers wild "grass" encounters.
  - *Animation flag*: `0` = none; else water animation (rotate tile `$14`); if bit 0 is **reset**,
    flower animation too (overwrites tile `$03` from ROM `$1F19`/`$1F29`/`$1F39`).
- **Map header:** tileset ID (1) · **height = Y size** (1) · **width = X size** (1) · pointer to
  map data (2) · pointer to text pointers (2) · pointer to script (2) · **connection byte** (1) ·
  **11 bytes per connection** · pointer to object data (2). (Header pointer table `0x01AE`, banks
  `0xC23D`. Confirms **height-before-width**.) The *script* pointer runs every overworld frame and
  is where things like one-time Poké Ball / static-encounter XY positions live.
- **Connection bitmask** (the connection byte): `bit3=North, bit2=South, bit1=West, bit0=East`
  (so `$08=N, $04=S, $02=W, $01=E`, OR'd together; `$0F` = all four, `$00` = none → object-data
  pointer follows immediately). This is exactly our `MapConnData` connection mask.
- **Connection data (11 bytes/connection):** connected map ID (1) · ptr to connection strip's
  upper-left block on the *connected* map (2) · same on the *current* map (2) · "bigness" (1) ·
  connected map width (1) · Y alignment (1) · X alignment (1) · window (2). The connection strip is
  **always 3 blocks wide (E/W) or 3 blocks high (N/S)**. Alignment = where the player lands; e.g.
  North Y-align = `(connected_height*2)-1`, and the `*2` recurs everywhere because 1 block = 2
  movement cells. (The page has full per-direction formulas + a worked Saffron City example if we
  ever implement connection editing.)
- **Object data (after the connections):** border block ID (1) · #warps (1) · warps · #signs (1) ·
  signs · #NPCs (1) · NPCs · warp-to points.
  - **Warp (4 bytes):** Y · X · **destination warp-to ID** (which warp-to entry in the *target*
    map) · destination map ID. *(This is the accurate warp model — a warp names a target map + an
    index into that map's warp-to list, not a raw coordinate pair.)*
  - **Sign (3 bytes):** Y · X · text-string ID.
  - **NPC (6/8/7 bytes), type encoded in the text-string ID:** `strID & (1<<6)` → **Trainer**
    (+2 bytes: trainer class + roster ID); `strID & (1<<7)` → **Item** (+1 byte: item ID); else
    **normal person**. Base 6 bytes = picture number · **Y+4** · **X+4** · movement byte 1 ·
    movement byte 2 · text-string ID. Trainers and one-time Pokémon share the layout, split by
    species/class ID: **< 200 = Pokémon, ≥ 200 = Trainer**.
  - **Warp-to point (4 bytes):** event displacement (2) · Y · X — the landing spot a warp's
    "destination warp-to ID" indexes into.
  - ⚠️ **The `+4` on NPC X/Y is very likely our long-standing offset mystery.** Object NPC
    positions are stored as **X+4 / Y+4**. This is almost certainly the same thing as our existing
    "Sprite screen position: pixel-aligned to a 4-pixel offset" and the unexplained "+3 offset flag"
    bullets in "Save file structure (the bytes)." **Reconcile these three notes** — if it's truly a
    flat +4 on sprite coords, the "+3" note may be an off-by-one in our reverse-engineering.
- **Tileset IDs (US R/B), `$00`–`$17`** — note there are **literal duplicate headers** (the editor
  should treat copies as aliases of the original):

  | ID | Tileset | | ID | Tileset |
  |----|---------|-|----|---------|
  | 00 | Outside | | 0C | Museum (copy 2) |
  | 01 | Red's House | | 0D | S.S. Anne |
  | 02 | Pokémon Center | | 0E | Vermilion Port |
  | 03 | Viridian Forest | | 0F | Pokémon Cemetery |
  | 04 | Red's House (copy) | | 10 | Silph Co. |
  | 05 | Gym | | 11 | Cave |
  | 06 | Pokémon Center | | 12 | Celadon Mart |
  | 07 | Gym (copy) | | 13 | Game Freak HQ |
  | 08 | House | | 14 | Lab |
  | 09 | Museum | | 15 | Bike Shop / Cable Center |
  | 0A | Museum (copy) | | 16 | Cinnabar Mansion / Power Plant |
  | 0B | Underground Path | | 17 | Indigo Plateau |

- **Sprite (NPC graphics) loading rules** — explains the classic glitchy-sprite behavior:
  - **Exterior maps** read `MapSpriteSets` (`0x17A64`). Entry `< $F0` → that sprite set directly;
    `≥ $F0` → a **split set** in `SplitMapSpriteSets` (`0x17A89`): split type (1=E/W, 2=N/S),
    split coordinate, set-if-below, set-if-at-or-above. **Sea Route 20** (`$F8`) is a special hard-coded
    split between sets `$01` (west) and `$0A` (east).
  - **Interior maps** load sprite tiles **on demand** per NPC present.
  - Sprites are reloaded on map change and when a textbox closes (font tiles overwrite walking
    sprites); **not** reloaded while crossing a map connection (assumes the set is already correct —
    why Walk-Through-Walls shows glitch sprites), and **not** at all if a map has no NPCs.
- **Handy formulas / constants** (for a future map feature):
  - *Event displacement*: `$C6EF + (map width) + (map width + 6) * (Y offset) + (X offset)`.
  - *Entity picture → ROM address*: `5*$4000 + ($7B27 + 4*(picture_id-1)) % $4000` gives an entry
    of {tile address (2), total tile size (1), bank ID (1)}; then `address = bankID*$4000 + tileAddr % $4000`.
  - *Max tileset in VRAM*: 6 rows of 16 tiles (`$9000`–`$95FF`).

### Sprite graphics encoding (only if we ever render the sprite images)

We store sprite *positions* (`SpriteData`), not graphics, today. But if a future feature renders the
actual overworld character sprites from `pret/pokered`'s exported PNGs, two non-obvious encodings
apply (per the external project):

- **White is the transparent color, and real colors are halved.** The exported PNGs reserve pure
  white `(255,255,255)` for transparency and remap every real color into the `[0–127]` range. To
  display correctly: treat white as fully transparent, and **double** the RGB of every non-white
  pixel (`×2`) back to `[0–255]`.
- **Only the left-facing sprite is stored; right-facing is a horizontal mirror.** To draw a
  character facing right, **flip the left-facing image horizontally** rather than expecting a
  separate asset.

### External map references

- **`pret/pokered`** — the community disassembly of Pokémon Red/Blue. The authoritative source for
  map headers, connections, warps, tilesets, and sprite assets (and already our reference for the
  music symbol table). Relevant paths: map headers `data/maps/headers/` (e.g. `PalletTown.asm`),
  layouts `maps/*.blk`, dimensions `constants/map_constants.asm`, graphics `gfx/tilesets/*.2bpp`,
  metatiles `gfx/blocksets/*.bst`.
- **Peter Hajas — "Parsing Pokémon Red and Blue Maps"** (peterhajas.com/blog/pokemon_rb_map_parsing.html,
  2020-08-03) — **the authoritative byte-format writeup** the "How a map is actually stored & parsed"
  section above is taken from (blockfiles, 2bpp, blocksets). He credits the pret Discord. He also has
  an earlier post on visualizing map *connections*.
- **vjeux, "Pokemon Red/Blue Map" (2023-12-24)** — the full-game map-rendering project this section
  was mined from. It renders the *entire* game including building/cave interiors. Its rendering
  algorithms (recursive connection walk, greedy warp placement via shortest-path, A\* line routing
  with per-tile costs, canvas color remapping) are **map-renderer–specific and do not apply to this
  save editor** — only the Gen 1 structural facts above were extracted.
- **Data Crystal — "Pokémon Red and Blue/Notes"** (datacrystal.tcrf.net; old URL
  datacrystal.romhacking.net is deprecated) — the **field-by-field ROM/RAM structures**: tileset &
  map headers, connection data + formulas, object data (warps/signs/NPCs/warp-to), sprite-set
  loading, and the internal species/move index tables. **Wiki-sourced — solid but verify single
  offsets against `pret/pokered`.** Source of the "Map & tileset byte structures" subsection and the
  internal-species-index note.

---

## Experience & growth rates

Each species has a **growth rate** — the EXP-to-level curve — identified by an internal **Growth
ID** that also appears in the Pokemon data table. Only four are used in Gen 1 (IDs **1 and 2 were
never used**).

Recorded **verbatim** from the original research note (`growth-notes.txt`):

| Name | Growth ID | EXP at level 100 | Formula (n = level) |
|------|-----------|------------------|---------------------|
| Fast        | 4 |   800,000 | Exp = 4 * n^0.6 |
| Medium Fast | 0 | 1,000,000 | Exp = n^3 |
| Medium Slow | 3 | 1,059,860 | Exp = 1.2 * n^3 - 15 * n^2 + 100 * n - 140 |
| Slow        | 5 | 1,250,000 | Exp = 5 * n^0.75 |

> **Open question — verify the Fast/Slow formulas before relying on them (kept as written).**
> This is only a plain-arithmetic observation, not a claim about what's "correct": the Fast and
> Slow *formulas* as written don't reach the level-100 *caps* in the same row. `4 * 100^0.6 ≈ 63`
> and `5 * 100^0.75 ≈ 158`, not 800,000 / 1,250,000. (Medium Fast `n^3` and the Medium Slow
> polynomial both already match their caps and aren't in question.) The commonly-cited Gen 1 cubic
> forms — Fast `(4/5)·n^3`, Slow `(5/4)·n^3` — *do* hit those caps, so the `n^0.6`/`n^0.75` lines
> may have been pasted from a different context, be shorthand for something other than
> total-EXP-at-level-n, or just be off. They're preserved exactly as written; check them against
> wherever they came from (or the program's own EXP code) before treating either form as
> authoritative.

**Editor design for EXP (KISS):** the editor offers a **slider to move EXP between the current and
the next level**; to change whole levels you edit the **level number**, which resets EXP to the
correct range for the new level. (This is what the General-tab Exp slider + the "fine-tune exp
between levels; change the level for whole-level changes" tooltip implement.)

---

## Music: banks, IDs, and the address formula

PokeRed abstracts audio so heavily that the underlying numbers are very hard to recover — this
took **over a year** of trial-and-error (the pokered team didn't answer requests for help),
cross-checked against the compiled disassembly's symbol table. What's known:

- Each track lives in a **bank** and has an **ID**; the address within the bank is
  **`ID * 3 + 0x4000`**. The IDs aren't evenly spaced, so they were recovered by guess-and-check:
  apply the formula, compare against the known bank/address from the symbol table, and adjust.
- **Useful discoveries:** a *slightly* wrong ID points to a slightly wrong address and produces
  **interesting (glitchy) music** — usable as a feature, not just a bug. And a **very short SFX**
  used as map music comes out as effectively **"No Music"** — e.g. `SFX_Snare1_1` (bank `1F`,
  ID `FF`) renders **silent**, which is eerier than Lavender Town.

The recovered track → bank → ID table (IDs are hex):

| Track | Bank | ID | | Track | Bank | ID |
|-------|------|----|-|-------|------|----|
| Music_PalletTown | 2 | BA | | Music_Routes2 | 2 | EF |
| Music_Pokecenter | 2 | BD | | Music_Routes3 | 2 | F3 |
| Music_Gym | 2 | C0 | | Music_Routes4 | 2 | F7 |
| Music_Cities1 | 2 | C3 | | Music_IndigoPlateau | 2 | FB |
| Music_Cities2 | 2 | C7 | | Music_GymLeaderBattle | 8 | EA |
| Music_Celadon | 2 | CA | | Music_TrainerBattle | 8 | ED |
| Music_Cinnabar | 2 | CD | | Music_WildBattle | 8 | F0 |
| Music_Vermilion | 2 | D0 | | Music_FinalBattle | 8 | F3 |
| Music_Lavender | 2 | D4 | | Music_DefeatedTrainer | 8 | F6 |
| Music_SSAnne | 2 | D8 | | Music_DefeatedWildMon | 8 | F9 |
| Music_MeetProfOak | 2 | DB | | Music_DefeatedGymLeader | 8 | FC |
| Music_MeetRival | 2 | DE | | Music_TitleScreen | 1F | C3 |
| Music_MuseumGuy | 2 | E1 | | Music_Credits | 1F | C7 |
| Music_SafariZone | 2 | E5 | | Music_HallOfFame | 1F | CA |
| Music_PkmnHealed | 2 | E8 | | Music_OaksLab | 1F | CD |
| Music_Routes1 | 2 | EB | | Music_JigglypuffSong | 1F | D0 |
| Music_BikeRiding | 1F | D2 | | Music_Dungeon1 | 1F | E0 |
| Music_Surfing | 1F | D6 | | Music_Dungeon2 | 1F | E4 |
| Music_GameCorner | 1F | D9 | | Music_Dungeon3 | 1F | E8 |
| Music_IntroBattle | 1F | DC | | Music_CinnabarMansion | 1F | EC |
| Music_PokemonTower | 1F | F0 | | Music_MeetEvilTrainer | 1F | F6 |
| Music_SilphCo | 1F | F3 | | Music_MeetFemaleTrainer | 1F | F9 |
| | | | | Music_MeetMaleTrainer | 1F | FC |

(This is also encoded in the music database / `music.json`; the formula and the discovery notes
above are the part that only lived in the original research note.)

---

## Randomization rules (playable by design)

The flagship feature's *concrete* constraints live in code; the *spirit* is in
`context/principles.md` → "The Randomization Feature". Two rules are worth recording because they
encode real Gen 1 knowledge:

- **Badges are zeroed on a random new game** — random mid-game progress isn't fun for a fresh
  start — **except Thunder, Cascade, Soul, and Rainbow, which are always left ON.** Those four
  aren't about progress here: they grant **out-of-battle HM use** (the HMs gated behind badges:
  Fly/Surf/Strength/Cut/Flash), so the player can actually move around the world. The gyms can
  still be battled for the first time — the badge just confers the HM permission
  (`playerbasics.cpp`).
- **Maps keep valid warps, wild encounters stay balanced, glitch species are avoided, and an
  HM-capable Pokémon is included** so the randomized save is genuinely *playable*. The map model
  even separates normal maps from a "--- Glitch Maps ---" section and labels the glitchy
  in-between ones. (`MapDBEntryWarpOut`, `MapSearch`, and the randomizer notes in
  `context/principles.md`.)
- **Randomize the towns the player has visited, but always include Pallet Town** (the starting
  town must be flagged visited or the new game makes no sense).
- **Badges and event-completed flags are separate.** Because the randomizer hands out a few badges
  for HM use (above) *without* setting the matching gym's event-completed flag, the player still
  has to actually battle and beat that gym the first time. The app should **tell the player this**
  rather than let the mismatch look like a bug.

---

See `systems/savefile.md` for the code that reads and writes these bytes (the toolset,
iterator, and the expanded object model), `systems/db.md` for where the species/move/item/map
data and their `glitch` flags come from, and `context/principles.md` for the randomizer's intent.
