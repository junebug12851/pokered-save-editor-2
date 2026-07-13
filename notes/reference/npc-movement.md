# How an NPC actually walks

_Read out of `engine/overworld/movement.asm` (`pret/pokered`), instruction by instruction. This is the
**complete** algorithm, bugs included — not a summary, not an impression. `MapSim` reproduces it._

> **Twilight, 2026-07-13:** *"walking needs to follow the games logic precisely and completely
> accurately, find this in pokered."*

---

## The headline: a STAY sprite is not standing still

It is **turning**.

```asm
CanWalkOntoTile:
    ld a, [hl]         ; MOVEMENTBYTE1
    inc a
    jr z, .impassable  ; if $ff, no movement allowed (however, changing direction is)
```

A `STAY` ($FF) sprite runs the **entire** random-movement path every time it is ready: it rolls a random
direction, and **`TryWalking` writes the new facing *before* it asks whether the step is legal.** Only
then does `CanWalkOntoTile` refuse it. So Professor Oak, standing outside his lab, is picking a random
direction every second or so, turning to face it, failing to move, and setting a new delay.

That is what "STAY" means in this game: **turn, but never step.**

⚠️ A simulation that just *skips* `STAY` sprites (which ours did) is not accurate. It is a still picture.

---

## The state machine

Every frame, for every sprite slot 1–15 (`UpdateNPCSprite`):

### `movementStatus` (`spritestatedata1` field 1)

| | |
|---|---|
| **0** | Uninitialised → `InitializeSpriteStatus` |
| **1** | **Ready** — roll, turn, and try to step |
| **2** | **Delayed** — count down, then become ready |
| **3** | **Walking** — advance the 16-frame step |
| bit 7 set | Face the player (they spoke to you) |

### 0 → `InitializeSpriteStatus`

```
movementStatus = 1
imageIndex     = $FF        ; off screen until it is drawn
yDisplacement  = 8
xDisplacement  = 8
```

### 1 — Ready

**Nothing happens while the player is mid-step** (`wWalkCounter != 0`). The whole town moves in lockstep
with him.

Then `InitializeSpriteScreenPosition` recomputes the sprite's **screen** pixels from its map position
*relative to the player*:

```
yPixels = ((mapY - playerY) * 16) - 4      ; the famous 4-pixel lift
xPixels = ((mapX - playerX) * 16)
```

Then, for `WALK` **and `STAY` alike** (`movementByte1` $FE or $FF):

```
r = Random()
```

**The direction is the top two bits of that byte** (`NPC_MOVEMENT_*`):

| random | direction |
|---|---|
| `$00–$3F` | down |
| `$40–$7F` | up |
| `$80–$BF` | left |
| `$C0–$FF` | right |

…**unless movement byte 2 overrides it**:

| movement byte 2 | what it does |
|---|---|
| `DOWN $D0` / `UP $D1` / `LEFT $D2` / `RIGHT $D3` | forces that direction — the roll is ignored |
| `UP_DOWN $01` | a rolled *left* becomes **up**; a rolled *right* becomes **down** |
| `LEFT_RIGHT $02` | a rolled *down* becomes **left**; a rolled *up* becomes **right** |
| `ANY_DIR $00`, `NONE $FF`, anything else | the roll stands |

⚠️ Note `NONE` ($FF) does **not** stop the roll. A sprite is stopped from *stepping* by movement byte
**1**, not byte 2.

### `TryWalking`

**In this order** — and the order is the whole point:

```
facingDirection = the facing for that direction      ; ⚠️ WRITTEN EVEN IF THE STEP FAILS
yStepVector     = dy   (-1 / 0 / +1)
xStepVector     = dx

if not CanWalkOntoTile():  return failure            ; (it has already set the delay)

mapY += dy
mapX += dx
walkAnimationCounter = 16
movementStatus       = 3
```

### `CanWalkOntoTile`

In order, and any one of them refuses:

1. `movementByte1 < WALK` (scripted) → **always allowed**, stop here.
2. The destination tile is **not in the tileset's passable list** → no.
3. **`movementByte1 == $FF` (STAY) → no.** ⬅ *the turn-but-never-step rule*
4. `yPixels + 4 + dy >= $80` → off the top/bottom of the screen → no.
5. `xPixels + dx >= $90` → off the sides → no.
6. `DetectCollisionBetweenSprites`; if `collisionData` has the chosen direction's bit → no.
7. **The displacement bookkeeping** — and it is *bugged*, on purpose, and we copy it:

   ```
   going down/right (d >= 0):   disp += d;  if disp < 5 → NO       ; ⚠️ the vertical one only
   going up/left    (d == -1):  disp -= 1;  if it underflows → NO
   ```

   ⚠️ **The famous bug.** `yDisplacement` starts at **8**. Walk *up* five times and it is 3. Now try to
   walk *down*: `3 + 1 = 4`, which is `< 5` → **refused**. The sprite is now stuck drifting upward and can
   never come back down. The disassembly says so in as many words:

   > *"bug: these tests against $5 probably were supposed to prevent sprites from walking out too far, but
   > this line makes sprites get stuck whenever they walked upwards 5 steps"*

   And the **horizontal** check has `cp $5` with **no conditional jump after it** — so X is never limited
   at all. Also a bug. Also copied.

**On failure** (`.impassable`):

```
movementStatus = 2                    ; delayed
yStepVector    = 0
xStepVector    = 0
movementDelay  = Random() & $7F       ; ⚠️ 0 means 256 frames, not 0 -- another bug
```

### 3 — Walking (`UpdateSpriteInWalkingAnimation`), once per frame

```
intraAnimFrameCounter++
if intraAnimFrameCounter == 4:
    intraAnimFrameCounter = 0
    animFrameCounter = (animFrameCounter + 1) & 3      ; 4 frames x 4 ticks = a 16-frame step

yPixels += yStepVector
xPixels += xStepVector

walkAnimationCounter--
if walkAnimationCounter != 0:  return

; the step is finished
if movementByte1 >= WALK:                  ; WALK or STAY
    movementDelay  = Random() & $7F
    movementStatus = 2
    yStepVector = xStepVector = 0
else:
    movementStatus = 1
```

### 2 — Delayed (`UpdateSpriteMovementDelay`), once per frame

```
if movementByte1 >= WALK:
    movementDelay--
    if movementDelay != 0:  → NotYetMoving
else:
    movementDelay = 0

movementStatus = 1

NotYetMoving:  animFrameCounter = 0        ; stand still, don't hold a walk pose
```

---

## So THAT is what the "animation scratch" fields are

They are not scratch. They are **the simulation's live state**, and every one of them ticks:

| field | what ticks it |
|---|---|
| `movementStatus` | the state machine above |
| `movementDelay` | counted down every frame while delayed; reset to `Random() & $7F` |
| `walkAnimationCounter` | 16 → 0 across one step |
| `intraAnimFrameCounter` / `animFrameCounter` | 4 ticks per frame, 4 frames per step |
| `yStepVector` / `xStepVector` | −1 / 0 / +1 while stepping, zeroed when not |
| `yPixels` / `xPixels` | advanced by the step vectors every frame |
| `yDisplacement` / `xDisplacement` | the (bugged) wander limit |
| `facingDirection` | written on **every** roll, step or no step |
| `collisionData` | which directions another sprite is blocking |

⚠️ **The game rebuilds all of it from the ROM the moment the map is (re)loaded** — see
[`sprites.md`](sprites.md) Part 6. So it is live, real, editable state that does **not** survive the
player walking back through the door. The UI must say that, with a **!**, rather than pretending
otherwise.

---

## What our simulation deliberately does differently, and why

One deviation, and it is honest:

* The console only updates sprites that are **on screen** (`CheckSpriteAvailability`), because it only
  has the screen's tilemap to hand. We simulate **every sprite on the map**, and we read passability from
  the **map's own block/tile data** rather than the 20×18 screen buffer — which is the same tile, just
  fetched a different way. Otherwise a sprite on the far side of Route 1 would simply never move, which
  would make the feature useless as an *editor*.

Everything else — the roll, the two-bit direction, the byte-2 override, the turn-before-the-check, the
`< 5` displacement bug, the `$7F` delay with its 256-frame zero — is reproduced exactly.
