# Sprite sets — the twelve bytes the game throws away

**Verified against the disassembly** (`pret/pokered`): `engine/overworld/map_sprites.asm`,
`data/maps/sprite_sets.asm`, `constants/sprite_set_constants.asm`, `home/overworld.asm`,
`ram/wram.asm`.

v1 of the editor had a screen called **"Cached sprites"** with a dropdown of *"Static List 1"*,
*"Static List 2 (Lavender Town)"*, *"Dynamic List 1 (Route 2)"* … Twilight's question was the right
one — *what even is this, and does the game use it?* Here is the answer, and it is a good one.

---

## What a sprite set is

The Game Boy can only hold a handful of overworld sprite tile-patterns in VRAM at a time. Gen 1's
answer is the **sprite set**: **eleven sprite pictures — nine that walk and two that don't**
(`SPRITE_SET_LENGTH EQU 9 + 2`). Every **outdoor** map names one, and every NPC on that map must be
drawn from it. That is why the same handful of faces recur across a region.

There are **ten** of them, and the game's own source names them
(`constants/sprite_set_constants.asm`):

| id | name | id | name |
|---|---|---|---|
| 1 | `SPRITESET_PALLET_VIRIDIAN` | 6 | `SPRITESET_INDIGO` |
| 2 | `SPRITESET_PEWTER_CERULEAN` | 7 | `SPRITESET_SAFFRON` |
| 3 | `SPRITESET_LAVENDER` | 8 | `SPRITESET_SILENCE_BRIDGE` |
| 4 | `SPRITESET_VERMILION` | 9 | `SPRITESET_CYCLING_ROAD` |
| 5 | `SPRITESET_CELADON` | 10 | `SPRITESET_FUCHSIA` |

`MapSpriteSets` (`data/maps/sprite_sets.asm`) is a flat table: one byte per outdoor map.

### The split sets — a table entry, never a saved value

Twelve maps are too big for one set, so their table entry is a **split id**, `$F1`–`$FC`
(`SPLITSET_ROUTE_2`, `_5`, `_6`, `_7`, `_8`, `_10`, `_11`, `_12`, `_15`, `_16`, `_18`, `_20`). Each
names a **dividing line** (`EAST_WEST` or `NORTH_SOUTH`, and a coordinate) and the two real sets on
either side of it. `GetSplitMapSpriteSetID` compares the player's `wXCoord`/`wYCoord` to the line and
returns **one of the ten real ids**.

🔴 **So `wSpriteSetID` can only ever hold 0–10.** A split id is resolved *before* anything is stored.
Our own `AreaLoadedSprites::loadSpriteSet` got this wrong until 2026-07-13 — it stored `entry->ind`,
so "place the player on Route 2" wrote **`$F1`** into a byte a console never writes anything but
`1`–`10` into. Fixed via `SpriteSetDBEntry::getResolvedSet(x, y)`; pinned by `tst_sprite_set`.

## What the save actually holds

`ram/wram.asm`, right after the connection headers and just before `wMapBackgroundTile`:

```
; sprite set for the current map (11 sprite picture ID's)
wSpriteSet::   ds SPRITE_SET_LENGTH     ; save 0x2649 .. 0x2653
; sprite set ID for the current map
wSpriteSetID:: db                       ; save 0x2654
```

Twelve bytes, inside the saved region — which is why v1 had a screen for them, and why we have a
panel.

## 🔴 …and the game throws them away

`InitOutsideMapSprites` (`engine/overworld/map_sprites.asm`) uses `wSpriteSetID` as a **cache key**:

```
ld a, [wSpriteSetID]
cp b                        ; has the sprite set ID changed?
jr z, .skipLoadingSpriteSet ; if not, don't load it again
```

That is the *entire* purpose of the byte: skip a VRAM copy you have already done. And **every map
load zeroes it first** (`LoadMapData`, `home/overworld.asm`):

```
xor a
...
ld [wSpriteSetID], a
call LoadTextBoxTilePatterns
call LoadMapHeader
farcall InitMapSprites       ; -> InitOutsideMapSprites: re-reads the set from MapSpriteSets
```

Pressing **CONTINUE** runs `SpecialEnterMap → EnterMap → LoadMapData`. So the twelve bytes in your
save are **recomputed from the map you are standing on before the game ever reads them**.

**Twilight's instinct was right: the game does not use what is in the save.** The bytes are real,
they are hers, and they are editable — but nothing you put there changes a thing in-game. The panel
says exactly that, at the top, rather than implying an edit that does nothing.

### The one interesting consequence

`InitOutsideMapSprites` returns immediately for an **indoor** map (`cp FIRST_INDOOR_MAP; ret nc`), so
indoors the eleven pictures are simply **left alone** — a save made in a building carries the sprite
set of the last *outdoor* map you were on, with a set id of 0 (because `LoadMapData` zeroed it and
nothing wrote it back). That is not save corruption; it is what a console holds too, and the panel
says so instead of calling it a mismatch.

## Where it lives in the editor

- **Save side:** `AreaLoadedSprites` (`preloadedSprites`) — `loadedSprites[11]`, `loadedSetId`.
- **Data:** `spriteSet.json` (all 22 entries: the ten real sets with their eleven sprite names, and
  the twelve split sets with their dividing lines), `sprites.json` (the picture ids).
- **UI:** `map/SpriteSetPanel.qml`, in the map screen's right-hand dock. It names the sets properly
  (v1's "Static List 4" is **Vermilion**), shows what the game *would* load for this map at the
  player's position, and offers to fill the cache with it.
