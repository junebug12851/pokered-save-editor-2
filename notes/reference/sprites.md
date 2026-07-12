# Sprites — the Game Boy's objects, and what Gen 1 builds on top of them

> Twilight: *"Sprites is going to be another deep thing — you need to research the Game Boy and the game
> code, pokered, and document all understanding in notes."* She's right. This is that document.
>
> Every claim here is either read out of `pret/pokered` or **measured off the real console** (the emulator's
> OAM and palette registers). Where it was measured, it says so.

## Part 1 — the hardware (what the Game Boy actually does)

The Game Boy draws **objects** (sprites) from **OAM**: 40 entries, 4 bytes each, at `$FE00`.

| byte | meaning |
|---|---|
| 0 | **Y**, *biased by 16* — an object at screen y=0 is written as 16. `Y=0` hides it. |
| 1 | **X**, *biased by 8* — screen x=0 is written as 8. `X=0` hides it. |
| 2 | **tile id** into the object tile area |
| 3 | **attributes** |

Attribute bits:

| bit | meaning |
|---|---|
| 7 | **priority** — 0: object over BG; 1: object *behind* BG colours 1–3 (but still over colour 0) |
| 6 | **Y-flip** |
| 5 | **X-flip** |
| 4 | **palette** — 0 = `rOBP0`, 1 = `rOBP1` |

Two things matter enormously and are easy to forget:

- **Colour 0 of an object is TRANSPARENT.** Always. That is why the object palettes only ever define three
  usable colours — bits 1-0 of `rOBP0`/`rOBP1` are ignored.
- Only **10 objects per scanline** are drawn. Gen 1 never gets near that in the overworld, but it is why
  sprites disappear in other games.

## Part 2 — Gen 1's overworld sprite system

### The 16 slots

`wSpriteStateData1` / `wSpriteStateData2` — **16 sprite slots**, `$10` bytes each. **Slot 0 is always the
player.** Slots 1–15 are the map's object events (`MAX_OBJECT_EVENTS EQU 16`).

The fields that matter to a renderer (`ram/wram.asm` documents all of them):

| `StateData1` | |
|---|---|
| 0 | picture id — 0 means *the slot is unused* |
| 2 | **image index** — facing + walk-animation frame, in the low nibble; the **VRAM slot** in the high nibble. `$FF` = **off screen, don't draw** |
| 4 | **Y screen position, in pixels** — *"always 4 pixels above grid, which makes sprites appear to be in the centre of a tile"* |
| 6 | X screen position, in pixels |
| 8 | walk-animation frame counter — 4 states |
| 9 | **facing** — `$0` down, `$4` up, `$8` left, `$C` right |

| `StateData2` | |
|---|---|
| 4, 5 | Y, X in 2×2-tile steps — **biased by 4** ("topmost 2×2 tile has value 4") |
| 7 | `$80` when the sprite is standing **in grass** → the grass-priority bit |
| `$E` | **image base offset** — the sprite's VRAM slot; 0 here means the slot is skipped entirely |

### The 4-pixel lift — measured, not assumed

A grid-aligned sprite's Y is `$fc, $0c, $1c, … $7c` — that is `row * 16 - 4`. The disassembly says so twice
(the WRAM comment above, and a comment in `DetectCollisionBetweenSprites` explaining why it adds 4 back).

**Confirmed on the console:** with the player at map (5, 6), OAM entry 0 reads back as screen **(64, 60)** —
x exactly on his tile column, y **4 pixels above** his tile row. And it cross-checks our own geometry
exactly: buffer (176, 188) − screen origin (112, 128) = (64, 60).

### One sprite = four OAM objects

`PrepareOAMData` (`engine/gfx/sprite_oam.asm`) writes **four** OAM entries per sprite — a 2×2 of 8×8 tiles —
using `SpriteFacingAndAnimationTable` (`data/sprites/facings.asm`), indexed by the image index's low nibble
(**facing + animation frame**):

```
.StandingDown: db $00, $01, $02, $03      .NormalOAM:   y  x  attrs
.WalkingDown:  db $80, $81, $82, $83        db 0, 0, 0            ; top left
.StandingUp:   db $04, $05, $06, $07        db 0, 8, 0            ; top right
.WalkingUp:    db $84, $85, $86, $87        db 8, 0, UNDER_GRASS  ; bottom left
.StandingLeft: db $08, $09, $0a, $0b        db 8, 8, UNDER_GRASS  ; bottom right
.WalkingLeft:  db $88, $89, $8a, $8b
                                          .FlippedOAM: the same, X-mirrored, with OAM_XFLIP
```

Three things fall out of that table, and every one of them is a trap if you don't read it:

1. **There is no "right" sprite.** Facing right is **facing left, X-flipped** (`.StandingLeft` + `.FlippedOAM`).
   The art only holds down / up / left.
2. **There is no second walking frame either.** The 4-state walk cycle is
   `stand → walk → stand → walk-MIRRORED`. Frame 3 of *down* and *up* is the **same walking sprite,
   X-flipped** — that is how Gen 1 gets "steps with the other foot" out of one drawing. (For left/right it
   doesn't flip on frame 3, because flipping already means "right".)
3. **The bottom two tiles carry `UNDER_GRASS`.** When the sprite stands in tall grass (`StateData2` field 7
   = `$80`), the priority bit is OR'd into the *lower half only*, so the grass draws over the character's
   legs and he looks like he's *in* it. The top half stays in front.

### The tile id

```
tileId = (imageIndex >> 4) * 12 + <offset from the table>
```

The high nibble of the image index is the sprite's **VRAM slot**, allocated per map from its **sprite set**
(`spriteSet.json`, which the editor already carries). **The player is always slot 0**, so his tiles are
`$00`–`$0B` (standing) and `$80`–`$8B` (walking) — 12 + 12 = **24 tiles = 6 frames × 4**.

That is exactly the shape of `gfx/sprites/red.png`: **16×96 px = six 16×16 frames**, in this order:

| frame | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| | stand **down** | stand **up** | stand **left** | walk **down** | walk **up** | walk **left** |

(Verified by eye against the sheet, and it is exactly what the tile table above demands.)

Sprites `$a` and `$b` are the exception: item balls and boulders have **one face, 4 tiles**, and the second
half of the facing table sends every facing/frame to the same quad.

### Colours

The player's OAM attributes are `0` → **`rOBP0`**. With the normal palette (`rOBP0 = $D0`):

| sprite colour index | via `$D0` | comes out as |
|---|---|---|
| 0 | — | **transparent** (always, for objects) |
| 1 | bits 3-2 = 0 | white |
| 2 | bits 5-4 = 1 | light grey |
| 3 | bits 7-6 = 3 | black |

**This is where the glitch palettes finally bite.** Contrast 1 and 2 leave `rBGP` at `$E4` — the map looks
completely normal — but they wreck `rOBP0`/`rOBP1`. Until sprites are drawn, those two values look like
nothing is wrong. See [`palettes.md`](palettes.md).

## Part 3 — what the SAVE gives us

The sprite state data (`$C100`) is **not** in the saved region — it's rebuilt on map load. What the save
*does* carry, and what the editor already models:

- **`playerCurDir`** (`AreaPlayer`, save `0x27D6`) — raw **bit flags**, not the sprite facing:
  `1 = right, 2 = left, 4 = down, 8 = up` (`PLAYER_DIR_*`). Map it to `SPRITE_FACING_*`
  (`right → $C, left → $8, down → $0, up → $4`).
- **`xCoord` / `yCoord`** — his position, from which the screen position follows.
- The map's **object events** (`MapDBEntry`'s sprites/signs/warps) — for the NPCs, later.

## Part 4 — how we render it

- The player's sprite sits at **buffer pixel `(border*32 + x*16, border*32 + y*16 − 4)`** — the 4-pixel lift
  included, because the console does it.
- Frame = facing (down/up/left) with **right drawn as left, mirrored**; standing frame while the save is at
  rest (a save is never mid-step).
- Colour index 0 → **transparent**; 1/2/3 → through **`rOBP0`**, exactly as the hardware maps them.

### Not done yet (deliberately, and in this order)

1. **NPCs / object events** — the other 15 slots. Their screen position, their sprite-set VRAM slot, the
   `$FF` off-screen rule, and the fact that a *missable* sprite may not be there at all.
2. **The grass priority bit** — draw grass over the sprite's lower half when he's standing in it.
3. **The walk animation** — the 4-state cycle above. A save is always at rest, so this only matters if the
   map screen ever animates.
