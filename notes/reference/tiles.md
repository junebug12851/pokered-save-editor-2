# Tiles, Blocks and Tilesets — what the map is actually made of

_Verified against `pret/pokered` **and** against Twilight's own cartridge (`assets/references/backup.gb`),
2026-07-12. Everything below has a source; nothing here is inferred from a wiki._

This is the reference for the **third layer** of the map emulator. The first two are done:
[`map-connections.md`](map-connections.md) (the border ring) and [`palettes.md`](palettes.md) (the
colours). This one is about **meaning** — what each square of the map *does*.

Read [`gen1-knowledge.md`](gen1-knowledge.md) first for the save offsets; this file assumes them.

---

## 1. The three layers, and why they keep getting confused

| Layer | Size | What it is | Where it lives |
|-------|------|-----------|----------------|
| **Tile** | 8×8 px | One graphic. 96 of them (`MAP_TILESET_SIZE` = `$60`) per tileset. | The tileset's GFX |
| **Block** | 4×4 tiles = 32×32 px | A pre-made arrangement of 16 tiles. **A map is a grid of blocks, never of tiles.** | The tileset's blockset |
| **Tileset** | — | One GFX + one blockset + one collision list + a handful of tile ids | `data/tilesets/tileset_headers.asm` |

Gameplay happens at the **tile** level (you walk in half-blocks = 2×2 tiles), but the map is *stored*
at the **block** level. Every field below belongs to exactly one of these layers, and getting that
wrong is what made the old Maps screen unreadable.

> ⚠️ **`wMapBackgroundTile` is a BLOCK.** The game's own name for it is a lie — pokered calls it a
> "tile", the save editor inherited that, and it is the **block** the 3-block border ring is filled
> with. `MapEngine` already treats it correctly (`MapDBEntry::getBorder()`).

---

## 2. The tileset header — 12 bytes, and the last one is not what we thought

`data/tilesets/tileset_headers.asm`:

```
MACRO tileset
	db BANK(\1_GFX)
	dw \1_Block, \1_GFX, \1_Coll
	db \2, \3, \4 ; counter tiles
	db \5         ; grass tile
	db \6         ; animations (TILEANIM_* value)
ENDM
```

`LoadTilesetHeader` copies the first **11** bytes to `wTilesetBank`… and the **12th** byte goes to
`hTileAnimations` — a different place entirely. That is why the save keeps it somewhere else too.

### 2.1 🔑 There is no "Indoor / Cave / Outdoor" byte. There is an ANIMATION byte.

`AreaTileset::type` reads save offset **`0x3522`**. That is **not** in `sMainData` — it is
**`sTileAnimations`**, the single byte sitting immediately before the checksum at `0x3523`
(`ram/sram.asm`). It is the saved copy of `hTileAnimations`, and its three values are
(`constants/map_data_constants.asm`):

```
	const TILEANIM_NONE          ; 0
	const TILEANIM_WATER         ; 1
	const TILEANIM_WATER_FLOWER  ; 2
```

`UpdateMovingBgTiles` (`home/vcopy.asm`) does exactly this: value 0 returns immediately; value 1
animates the **water** tile (`$14`) and resets the counter before it can reach the flower frame;
value 2 lets the counter reach 21 and also animates the **flower** tile (`$03`).

**And `tileset.json`'s `type` is a verified 1:1 rename of it.** All 24 tilesets checked against the
cartridge's header table — a perfect match, every one:

| `tileset.json` | The ROM | What it means |
|---|---|---|
| **Indoor** | `TILEANIM_NONE` (0) | nothing animates |
| **Cave** | `TILEANIM_WATER` (1) | water animates |
| **Outdoor** | `TILEANIM_WATER_FLOWER` (2) | water **and** flowers animate |

Overworld/Dojo/Gym = Outdoor. Forest/Ship/ShipPort/Cavern/Facility/Plateau = Cave. Everything else =
Indoor. **The data was right all along; only the name was friendly.** No JSON change needed, and the
UI can now say both — the name Twilight gave it *and* what it does.

> 🐞 **This is also a live rendering bug.** `Settings::previewOutdoor` is a **bool**, so it collapses
> Cave into Indoor: a cave tileset renders with **no animated water**, when the console animates it.
> The tri-state is a fidelity fix, not a relabel.

### 2.2 Counter tiles (`wTilesetTalkingOverTiles`, 3 slots, `0x27DE`)

The tiles you can **talk across**. `home/overworld.asm` → `.extendRangeOverCounter`: if the tile in
front of the player is one of these three, the NPC-talking range is extended from short to `$20`
pixels — so you can speak to the clerk on the far side of a **shop desk**. `$FF` = unused slot.

That is the whole of it. Mart/Pokecenter use `$18,$19,$1E`; Gate/Museum/Forest Gate use `$17,$32`;
Gym/Dojo use `$3A`.

### 2.3 Grass tile (`wGrassTile`, `0x27E1`)

One tile id, and it does **three** things:

1. **Wild encounters.** `TryDoWildEncounter` checks the bottom-right tile of the half-block you stand
   in against `wGrassTile`; a match arms `wGrassRate`. This tile *is* the encounter.
2. **Sprite priority.** `home/movement.asm` sets bit 7 of a sprite's flags when it stands on this
   tile, which is what draws the grass **over** the player's feet.
3. Nothing else. `$FF` = the tileset has no grass (every indoor one).

Overworld `$52`, Forest `$20`, Plateau `$45`. Those three, and no others.

---

## 3. Collision: the save only points at it

**There is no "colliding tile" field in the save.** `collPtr` (`0x27DC`, `wTilesetCollisionPtr`) is a
**bank-0 ROM pointer** to the tileset's list of **PASSABLE** tile ids, `$FF`-terminated
(`data/tilesets/collision_tile_ids.asm`). `CheckTilePassable` walks the list; **a tile not in the
list is a wall.**

So "does this collide?" is a **derived** property of every tile on screen — which is exactly what
makes drawing it over the map possible. It is not something the user types in.

### 3.1 ⚠️ The lists are SHARED, and `tileset.json` got three of them wrong

Several tilesets point at the *same* list (`RedsHouse1_Coll::` / `RedsHouse2_Coll::` are two labels on
one list). The v1 importer assumed one list per tileset **in index order** — so it walked the chain
one entry out of step for three of them.

Dumped straight out of the cartridge (bank 0, `$172F` onward), 21 `$FF`-terminated lists, chaining
exactly as the disassembly says:

| Addr | Length | Belongs to |
|------|--------|-----------|
| 5935 | 5 | Underground |
| 5941 | 19 | Overworld |
| 5961 | 9 | Reds House 1 **and Reds House 2** |
| 5971 | 5 | Mart **and Pokecenter** |
| 5977 | 11 | Dojo **and Gym** |
| 5989 | 15 | **Forest** |
| 6005 | 9 | House |
| 6015 | 10 | Forest Gate / Museum / Gate |
| 6026 | 10 | Ship |
| 6037 | 4 | Ship Port |
| 6042 | 7 | Cemetery |
| 6050 | 9 | Interior |
| 6060 | 10 | Cavern |
| 6071 | 0 | *(unused — an empty list really is in the ROM)* |
| 6072 | 7 | Lobby |
| 6080 | 9 | Mansion |
| 6090 | 6 | Lab |
| 6097 | 11 | Club |
| 6109 | 18 | Facility |
| 6128 | 6 | Plateau |

Three entries in `tileset.json` disagreed, and the cartridge wins:

| Tileset | JSON had | Cartridge says |
|---|---|---|
| Mart | 5961 | **5971** |
| Forest | 5971 | **5989** |
| Reds House 2 | 5989 | **5961** |

**This had teeth.** `AreaTileset::loadFromData()` writes `collPtr` into the save, so placing the
player in a **Poké Mart** wrote **Red's-house collision** into the save — wrong walls, wrong doors,
wrong counter, in the real game. Fixed 2026-07-12 with Twilight's go-ahead, and pinned by a test that
re-derives all 24 pointers from the list chain so it can never drift again.

---

## 4. The two fields that are not map data at all

| Save | WRAM | What it really is |
|------|------|-------------------|
| `boulderIndex` `0x29C4` | `wBoulderSpriteIndex` | *"sprite index of the boulder the player is trying to push"* |
| `boulderColl` `0x29C8` | `wTileInFrontOfBoulderAndBoulderCollisionResult` | *"the tile in front of the boulder when trying to push a boulder; also the result of the collision check (`$FF` collision, `$00` no collision)"* |

These are **runtime scratch from the last Strength push**. They are not tileset configuration, they
are not map configuration, and editing them configures nothing — the game overwrites them the next
time you shove a rock. Twilight's own 2020 comment ("*I don't think I've seen them used in a sav
file*") was right.

They must **not** be presented as editable map properties.

---

## 5. Everything else the ROM already tells us about a tile

All per-tileset, all free, all in `data/tilesets/`. This is the material the map overlay is drawn
from.

| Data | File | Note |
|------|------|------|
| **Passable** | `collision_tile_ids.asm` | §3 above. The absence of this is a wall. |
| **Warp** | `warp_tile_ids.asm` | Stepping on it warps. ⚠️ Several lists **fall through** into each other — parse the labels, not the blank lines. |
| **Door** | `door_tile_ids.asm` | A `dbw` table keyed by tileset id, `-1`-terminated; a tileset absent from it has no doors. Note the terminator here is **`0`**, not `-1`. |
| **Ledge** | `ledge_tiles.asm` | `(facing, tile you stand on, ledge tile, input)` — so a ledge knows **which way you jump**. |
| **Water** | `water_tilesets.asm` | Which tilesets contain water; the water tile itself is **`$14`**. |
| **Bookshelf / sign** | `bookshelf_tile_ids.asm` | `(tileset, tile, text)` — the tiles that talk back. |
| **Warp pad / hole** | `warp_pad_hole_tile_ids.asm` | Only Facility (`$20` pad, `$11` hole), Cavern (`$22` hole), Interior (`$55` pad). |
| **Elevation edges** | `pair_collision_tile_ids.asm` | `(tileset, tileA, tileB)`: you may not cross **between** these two. Cavern and Forest only. Land and water lists. |
| **Cut trees** | `cut_tree_blocks.asm` | ⚠️ These are **BLOCK** ids, not tile ids — block-layer, not tile-layer. |
| **Spinner** | `spinner_tiles.asm` | Facility + Gym arrow tiles. |
| **Dungeon** | `dungeon_tilesets.asm` | Forces a warp re-position on load (`LoadTilesetHeader`). |
| **Bike / Escape Rope** | `bike_riding_tilesets.asm`, `escape_rope_tilesets.asm` | Where each is allowed. |

---

## 6. The rule this file exists to enforce

**Every one of these bytes is editable, and not one of them is a raw number.**
(`context/principles.md` → "Every Byte, None of Them Raw".)

Nothing here gets dropped from the UI for being obscure — **including** the two Strength-push scratch
bytes in §4. "It isn't really map data" is a reason to *label it honestly*, never a reason to hide it.
And nothing here is a spin box, because every one of them is a **thing**:

| Field | What it is | So the editor is |
|---|---|---|
| `type` (§2.1) | not a number at all | **Indoor / Cave / Outdoor**, each saying what it animates |
| `grassTile` (§2.3) | a tile | a picker of the tileset's **rendered tiles** |
| counters 1–3 (§2.2) | tiles | the same picker — "the tiles you can talk across" |
| `outOfBoundsBlock` (§1) | a **block** | a picker of the tileset's **rendered blocks** |
| `boulderIndex` (§4) | a **sprite slot** | the map's sprites, by picture and name |
| `boulderColl` (§4) | a tile **and** a yes/no | both, said out loud: the tile, plus **Blocked**/**Clear** |
| `collPtr` etc. (§3) | a ROM pointer | a **named** choice — "the collision list Mart uses" |

Each of those carries a **Custom…** escape hatch, so any byte value is still reachable — a glitch
hunter is a legitimate user. And when the save disagrees with the cartridge we **show** it and never
silently rewrite it, the same doctrine as the music bank ([`glitch-music.md`](glitch-music.md)) and
the glitch palettes ([`palettes.md`](palettes.md)).

The number is an implementation detail of a 1996 cartridge. It is not the user's problem — but the
*byte* is absolutely the user's property.
