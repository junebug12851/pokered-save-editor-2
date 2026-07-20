# Sprite sets — the twelve bytes the game throws away

**Verified against the disassembly** (`pret/pokered`): `engine/overworld/map_sprites.asm`,
`data/maps/sprite_sets.asm`, `constants/sprite_set_constants.asm`, `home/overworld.asm`,
`ram/wram.asm`.

v1 of the editor had a screen called **"Cached sprites"** with a dropdown of *"Static List 1"*,
*"Static List 2 (Lavender Town)"*, *"Dynamic List 1 (Route 2)"* … project leadership's question was the right
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

**So `wSpriteSetID` can only ever hold 0–10.** A split id is resolved *before* anything is stored.
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

## …and the game throws them away

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

**Project leadership's instinct was right: the game does not use what is in the save.** The bytes are real,
they are theirs, and they are editable — but nothing you put there changes a thing in-game. The panel
says exactly that, at the top, rather than implying an edit that does nothing.

## INDOORS THERE IS NO SPRITE SET (2026-07-13)

This is the biggest thing on the page and I missed it for a day. `InitOutsideMapSprites` returns
immediately for an **indoor** map (`cp FIRST_INDOOR_MAP; ret nc` — `FIRST_INDOOR_MAP` = **`$25`**),
and then `InitMapSprites` does something completely different:

```asm
InitMapSprites::
    call InitOutsideMapSprites
    ret c                              ; outdoor maps are already done
; if the map is an inside map (i.e. mapID >= FIRST_INDOOR_MAP)
    ld hl, wSpritePlayerStateData1PictureID
    ld de, wSpritePlayerStateData2PictureID
.copyPictureIDLoop
    ld a, [hl]                         ; ⚠️ the NPC's OWN picture
    ld [de], a
    ...
; falls straight into LoadMapSpriteTilePatterns
```

**Indoors, the game loads each character's own artwork.** The cast *is* the set. There is no lookup,
nothing to be absent from, and **any picture draws correctly in a building** — Gamblers in Red's
house, Lorelei behind the Pokémart counter, whatever you like.

What there *is*, instead, is a hard **capacity**. `LoadMapSpriteTilePatterns` allocates VRAM slots by
**first appearance**, deduplicating repeats (`.alreadyLoaded` reuses the earlier slot), and there are:

| | slots | which pictures |
|---|---|---|
| walking sprites | **10** | picture `< FIRST_STILL_SPRITE` (`$3D`) |
| still, 4-tile sprites | **2** | picture `>= $3D` — Pokéballs, boulders, the fossil, the paper |

The eleventh *distinct* walking picture in a building is the one the console cannot draw. A **second
copy** of a picture already loaded is free.

The save's eleven cached bytes are simply the last *outdoor* map's, carried in, with a set id of 0
(`LoadMapData` zeroed it and nothing wrote it back). **The game ignores them completely indoors.**
That is not save corruption; it is what a console holds too.

## …and outdoors, an out-of-set picture is UNDEFINED, not merely wrong

The other half nobody writes down. `.storeVRAMSlotsLoop` finds an NPC's VRAM slot by searching the
sprite set for its picture id:

```asm
.getPictureIndexLoop
    inc c
    ld a, [de]
    inc de
    cp b                          ; does the picture ID match?
    jr nz, .getPictureIndexLoop   ; ⚠️ NO BOUNDS CHECK. NO TERMINATION.
    inc c
```

**There is no bounds check and no loop counter.** A picture that isn't in the set walks off the end of
`wSpriteSet` and keeps reading WRAM until some byte happens to equal it. Whatever `c` is by then
becomes the VRAM slot, and the sprite draws whatever tiles are sitting at that address.

So "the game would draw it as garbage" is *literally* true, and the garbage depends on the contents of
RAM. It is not a thing we can render faithfully, and we should not try — we mark it and draw the
artwork the user chose.

## What the editor does with all this

`MapModel::vramPictures()` **is the console's routine**, and everything that asks "will this render?"
goes through it — the Characters panel's marks, the canvas's `!`, the picture picker:

* **outdoor** → the map's **ROM** sprite set (`MapSpriteSets[wCurMap]`, splits resolved against the
  player's position). *Not* the save's cached copy, which the game overwrites.
* **indoor** → the cast itself, deduped, first-appearance, capped at 10 walking + 2 still.

⚠️ Before this, we tested membership of the **save's cached set** — a set the console throws away. On
an indoor map that meant we flagged characters that would have drawn perfectly, and on any map an
edit to the cache silently changed our answer. `tst_map_sprites` now pins all three cases.

## Where it lives in the editor

- **Save side:** `AreaLoadedSprites` (`preloadedSprites`) — `loadedSprites[11]`, `loadedSetId`.
- **Data:** `spriteSet.json` (all 22 entries: the ten real sets with their eleven sprite names, and
  the twelve split sets with their dividing lines), `sprites.json` (the picture ids).
- **UI:** `map/SpriteSetPanel.qml`, in the map screen's right-hand dock. It names the sets properly
  (v1's "Static List 4" is **Vermilion**), shows what the game *would* load for this map at the
  player's position, and offers to fill the cache with it.
