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

**The sprite state data IS saved.** `sSpriteData` (`ram/sram.asm`) is `wSpriteDataEnd − wSpriteDataStart` =
**`$200` bytes** — both 16-slot tables, verbatim — copied out by `SaveSAV` and back in by `LoadSAV`
(`engine/menus/save.asm`). In the `.sav` it lives at **`0x2D2C`** (StateData1) and **`0x2E2C`**
(StateData2).

> ⚠️ An earlier reading of this file said the state data was *"not in the saved region — it's rebuilt on map
> load."* **That was wrong**, and the cartridge is what corrected it. See Part 6.

The rest of the sprite story lives in `sMainData` (base `0x25A3` ↔ `wMainDataStart` `$D2F7`, so
`wram = 0xD2F7 + (sav − 0x25A3)`):

| save | WRAM | what |
|---|---|---|
| `0x278D` | `$D4E1` | `wNumSprites` — NPC count, **excluding** the player |
| `0x2790` | `$D4E4` | `wMapSpriteData` — 16 × (**movement byte 2**, **text id**) |
| `0x27B0` | `$D504` | `wMapSpriteExtraData` — 16 × (**trainer class / item id**, **trainer set id**) |
| `0x287A` | `$D5CE` | `wToggleableObjectList` — 16 × (sprite slot, global toggle index) + `$FF` terminator |
| `0x28A0` | `$D5F4` | **`wToggleableObjectFlags`** — `flag_array $100` (**32 bytes**); *bit set = toggled OFF* |
| `0x29DA` | `$D72E` | `wStatusFlags4` — bit 5 is `BIT_BATTLE_OVER_OR_BLACKOUT` (matters, see Part 6) |

Plus the player's own two:

- **`playerCurDir`** (`AreaPlayer`, save `0x27D6`) — raw **bit flags**, not the sprite facing:
  `1 = right, 2 = left, 4 = down, 8 = up` (`PLAYER_DIR_*`). Map it to `SPRITE_FACING_*`
  (`right → $C, left → $8, down → $0, up → $4`).
- **`xCoord` / `yCoord`** — his position, from which the screen position follows.

### The ROM's object-event format — where the save's values come from

```
object_event  x, y, SPRITE_ID, movement1, movement2, text_id [, item_id] [, class, set]
```

`data/maps/objects/*.asm`, assembled by the `object_event` macro (`macros/data.asm`) into
`sprite, y+4, x+4, movement1, movement2, text_id|flags, …`. Every value in that line has exactly one home
in the save:

| macro arg | goes to | values |
|---|---|---|
| `x`, `y` | StateData2 `4`/`5` (**+4**) | the "topmost/leftmost tile is 4" bias |
| `SPRITE_ID` | StateData1 `0` (`pictureID`) | 1-based index into `sprites.json` |
| **`movement1`** | StateData2 **`6`** (`MOVEMENTBYTE1`) | **`WALK = $FE`**, **`STAY = $FF`** |
| **`movement2`** | **`wMapSpriteData` byte 0** | `ANY_DIR $00` · `UP_DOWN $01` · `LEFT_RIGHT $02` · `DOWN $D0` · `UP $D1` · `LEFT $D2` · `RIGHT $D3` · `NONE $FF` · `BOULDER_MOVEMENT_BYTE_2 $10` |
| `text_id` | `wMapSpriteData` byte 1 | **OR'd** in ROM with `TRAINER ($40)` / `ITEM ($80)` — but the loader does `and $3f`, so **the save holds a 6-bit id and no flags** |
| `item_id` *or* `class`,`set` | `wMapSpriteExtraData` | item → byte 0, byte 1 zeroed; trainer → class, set |

**🔑 `movement1` and `movement2` are two different bytes in two different tables**, and `movement2` is
neither a facing nor a range — it is one byte that means *"how is this sprite allowed to move, or which way
does it stare"*. `maps.json` splits it into **`range`** (for `WALK` sprites: `0`/`1`/`2`) and **`face`** (for
`STAY` sprites: `"None"`/`"Down"`/`"Up"`/`"Left"`/`"Right"`). That split is a fine *curation* — but they are
**the same byte**, and anything that routes `face` into the *animation* facing (StateData1 field `9`) is
wrong. See Part 5.

### Missables are "toggleable objects", and the LIST is not the part that matters

`wToggleableObjectList` (`0x287A`) is **rebuilt from ROM on every map load** by
`MarkTownVisitedAndLoadToggleableObjects` — writing it from an editor accomplishes nothing. The part that
persists and actually decides whether an NPC is on the map is **`wToggleableObjectFlags`** (`0x28A0`, 32
bytes, *bit set = hidden*). **We do not model it at all** — see Part 5.

## Part 4 — how we render it

- The player's sprite sits at **buffer pixel `(border*32 + x*16, border*32 + y*16 − 4)`** — the 4-pixel lift
  included, because the console does it.
- Frame = facing (down/up/left) with **right drawn as left, mirrored**; standing frame while the save is at
  rest (a save is never mid-step).
- Colour index 0 → **transparent**; 1/2/3 → through **`rOBP0`**, exactly as the hardware maps them.

## Part 5 — the four bugs this research found (2026-07-13)

The v2 model is a straight C++ port of v1's `SpriteData.ts`. v1 had **no enums** and its inline comments are
*correct*. The enums are a **v2 invention**, and two of them are **inverted** — so the editor has been
writing the opposite of what it says.

| # | what | v2 has | the truth | proof |
|---|---|---|---|---|
| 1 | `SpriteMobility` | `Moving = 0xFF`, `NotMoving = 0xFE` | **`STAY = $FF`, `WALK = $FE`** | console: Pallet's Oak (`STAY`) reads `$FF`; the Girl + Fisher (`WALK`) read `$FE` |
| 2 | `SpriteData::load(MapDBEntrySprite*)` | `getMove()=="Stay"` → `NotMoving` (`0xFE`) | should write `$FF` | it writes **WALK for a STAY sprite** — every `setTo()` / randomize gets it backwards |
| 3 | `SpriteGrass` | `InGrass = 0x00`, `NotInGrass = 0x80` | **`$80` = IN grass** (`wram.asm`) | `reset()` therefore flags **every blank sprite as standing in grass** |
| 4 | `face` vs `range` | `face` → `faceDir` (StateData1 `9`), `range` → `rangeDirByte` | **both are movement byte 2** | console: Oak's `wMapSpriteData[0]` is `$FF` (= `"face":"None"`); the Girl's is `$00` (= `"range":0`) |

And four things simply **not modelled**:

5. StateData1 `a` **YADJUSTED**, `b` **XADJUSTED**, `c` **COLLISIONDATA**.
6. StateData2 `9` **ORIGFACINGDIRECTION** (backed up by `DisplayTextIDInit`, restored by `CloseTextDisplay`)
   and `d` — a **duplicate `PICTUREID`**.
7. **`wToggleableObjectFlags`** (`0x28A0`) — the flags that actually hide/show a missable NPC.
8. `SpriteMovement` is missing `ANY_DIR ($00)` and `NONE ($FF)`, and its comments say *"I have no idea"*.

## Part 6 — the console settled the big one: **sprite edits DO survive**

Reading `LoadMapHeader` (`home/overworld.asm`, `.loadSpriteData`) is genuinely alarming. On a map load it
**zeroes `$F0` bytes of both sprite tables** for slots 1–15 and reloads `pictureID` / `MAPY` / `MAPX` /
`MOVEMENTBYTE1` / `wMapSpriteData` / `wMapSpriteExtraData` **from the ROM** — unless
`wStatusFlags4` bit 5 (`BIT_BATTLE_OVER_OR_BLACKOUT`) is set, which it isn't in a normal save. Read that
much and you conclude a sprite editor is writing bytes the game erases the moment it boots.

**It doesn't.** `scripts/emu/probe_sprite_persistence.py` boots the real cartridge with three saves and reads
back the console's own WRAM:

| | what the console held after Continue |
|---|---|
| **A** untouched `BaseSAV` | `wNumSprites 3` · slot 1 `pic $03` `(8,5)` `move1 $FF` · slot 2 `pic $0D` `(3,6)` `$FE` · slot 3 `pic $2F` `(11,14)` `$FE` |
| **B** slot 1 re-pictured + moved, a 4th NPC invented, bit **clear** | **every edit held** — `wNumSprites 4`, slot 1 `pic $2E` at `(9,9)`, slot 4 `pic $05` at `(6,11)` |
| **C** the same, bit **set** | identical — the bit changes nothing here |

The reason: `MainMenu`'s `.choseContinue` sets **`BIT_CUR_MAP_LOADED_1`** before jumping to
`SpecialEnterMap`, so the map header is **not** re-read on the continue path. The saved sprite state is what
the player walks into.

**The honest statement, then:** an edited sprite is real and on the map when the save is loaded — and it is
**rebuilt from the ROM the moment the player leaves that map and comes back**. That is not a bug in the
editor; it is what the cartridge does, and it is exactly what Twilight observed years ago in v1 ("*sprite
edits work; outdoor sprites are glitchy*"). The glitchiness has its own cause: the VRAM slot
(`IMAGEBASEOFFSET`) is allocated from the map's **sprite set**, and an outdoor sprite that isn't in that
map's set has no slot to point at. See [`sprite-sets.md`](sprite-sets.md).

**The map screen must say this out loud** — it is precisely the "a derived byte is SHOWN, never silently
synced" doctrine: the editor places the sprite, and tells the user plainly that the game will restore the
original cast on the next map re-entry.

### Not done yet (deliberately, and in this order)

1. **The grass priority bit** — draw grass over the sprite's lower half when he's standing in it.
2. **The walk animation** — the 4-state cycle above. A save is always at rest, so this only matters if the
   map screen ever animates.
3. **`wToggleableObjectFlags`** — a real UI for which NPCs are hidden.
