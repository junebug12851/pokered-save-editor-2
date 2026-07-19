# The map, ANIMATED — what the console actually does, frame by frame

**Verified against the disassembly** (`pret/pokered`, Twilight's clone at `Documents/projects/pokered`).
The map is not a still picture: the Game Boy re-writes tile graphics **in VRAM every frame**, and it does
it from **one byte the save holds**. This is the reference for making our map move the way the console's
does — and it is the spec Phase 3 of [`../plans/map-screen.md`](../plans/map-screen.md) is built from.

> ⚠️ **Nothing here is an impression.** Every number below is read out of the game's own code. If our
> renderer disagrees with this, our renderer is wrong. The oracle (`tst_emu_parity`) can and must check it:
> the animated tiles live in VRAM, so PyBoy can dump them **every frame** and demand a byte-for-byte match —
> the same doctrine that proved the sound engine.

---

## The one byte that drives it

`hTileAnimations` — loaded from the **tileset's animation byte**, which is what the save stores at
**`0x3522`** (`sTileAnimations`; the byte `AreaTileset` calls `type`). The disassembly's own comment:

```
; Controls which tiles are animated.
; 0 = no animations (breaks Surf)
; 1 = water tile $14 is animated
; 2 = water tile $14 and flower tile $03 are animated
hTileAnimations:: db
```

`TILEANIM_NONE = 0`, `TILEANIM_WATER = 1`, `TILEANIM_WATER_FLOWER = 2` — which is exactly the tri-state
`tileset.json` renames as **Indoor / Cave / Outdoor** (a verified 1:1 rename; see
[`tiles.md`](tiles.md)).

**"0 breaks Surf" is not a joke.** Set a water map's animation byte to Indoor and the game's *Surf* code
depends on the animated tile — the app should say that, in words, when someone does it.

## The loop — `UpdateMovingBgTiles` (`home/vcopy.asm`)

Runs **once per frame**, in V-blank. Two counters: `hMovingBGTilesCounter1` (the frame counter) and
`wMovingBGTilesCounter2` (the animation *phase*, which survives across steps).

```
if hTileAnimations == 0: return           ; nothing animates

counter1 += 1
if counter1 <  20: return                 ; 19 frames of doing nothing
if counter1 == 21: goto flower            ; the 21st frame is the flower's

; ── water ── (counter1 == 20)
counter2 = (counter2 + 1) & 7
rotate the 16 bytes of VRAM tile $14:
    counter2 & 4  ?  rlca each byte  (left)
                  :  rrca each byte  (right)
if hTileAnimations bit 0 is SET:          ; value 1 = water only
    counter1 = 0                          ; → the cycle is 20 frames
    return
; else (value 2) fall through: counter1 stays 20, so next frame it is 21

; ── flower ── (counter1 == 21)
counter1 = 0                              ; → the cycle is 21 frames
frame = counter2 & 3
    0, 1 → FlowerTile1
    2    → FlowerTile2
    3    → FlowerTile3
copy that 16-byte tile into VRAM tile $03
```

### What that means in practice

- **The animation ticks once every 20 frames** (water-only tilesets) **or 21** (water+flower) — roughly
  **three times a second** at the DMG's 59.73 Hz. It is slow, and it is *supposed* to be.
- **Water doesn't have frames — it has a ROTATION.** The tile's 16 bytes are **bit-rotated one pixel per
  step**, right for four steps then left for four (`counter2 & 4`), forever. That's the shimmer. There is
  no "water frame 2" to load — there is one tile, being rolled back and forth.
- **The flower has three real frames**, and the third is only reached on `counter2 & 3 == 3` — so the
  sequence over a full 8-step `counter2` cycle is `1,1,2,3,1,1,2,3`. Flower1 shows **twice as long** as the
  other two.
- **The two counters are coupled.** The flower's frame is chosen from the *water's* phase counter — so on a
  water+flower tileset the flower and the water are locked in step, and you cannot animate one without the
  other. (Which is why there is no "flower only" value.)

### Hack values are defined behaviour, not chaos

The code tests `hTileAnimations` twice: `and a` (is it zero?) and, after the water step, `rrca` (is **bit
0** set?). So:

| Byte | What the console does |
|---|---|
| `0` | Nothing animates. Water is dead. **Surf breaks.** |
| `1` | Water only, 20-frame cycle. |
| `2` | Water + flower, 21-frame cycle. |
| **`3`, `5`, `7`… (odd)** | **Behaves exactly like 1** — bit 0 is set, so it resets after the water step. |
| **`4`, `6`, `8`… (even, non-zero)** | **Behaves exactly like 2** — bit 0 is clear, so it falls through to the flower. |

So an "impossible" tileset type byte is **not** garbage: it is water-only or water+flower, decided by its
lowest bit. The editor shows the value the save holds, says which of the two behaviours a real console
would give it, and **never rewrites it**.

---

## The sprites (the other moving thing)

See [`sprites.md`](sprites.md) for the hardware. The traps that bite an animator specifically:

- **There is no right-facing sprite.** Facing right is the left-facing art, **mirrored** (OAM X-flip).
- **There is no second walking frame.** The "other foot" is the *same* frame, flipped — so a two-frame walk
  cycle is one tile drawn twice, once mirrored.
- **A sprite sits 4 px above its tile row** — "which makes sprites appear to be in the centre of a tile"
  (`ram/wram.asm`), and confirmed against the console's own OAM.
- Sprites are drawn through **`rOBP0`/`rOBP1`**, not `rBGP` — which is why contrast **1 and 2** leave the
  map looking perfectly normal and wreck *the people on it*. (See [`palettes.md`](palettes.md).)

---

## What we shipped (2026-07-12, map-screen phase 3) — and the two bugs it found

`MapClock` (`brg.mapClock`) is the heartbeat: it advances **one number** — `MapModel::frame`, the
animation *step* — at the cadence above (20 or 21 console frames, from the save's own byte), and it
**refuses to run on the `offscreen` platform**, which is what every headless run uses. `MapEngine`
renders from `(save, layers, frame)` and never reads a clock. The transport (▶/⏸ · step · the
cadence) is in the status bar, and it is **absent on a map that does not animate** — no button that
lies about what it does.

**The port was already there, and it was wrong in two places.** `TilesetEngine` had implemented the
water and the flower years ago — plausibly, and by invention:

| | It ran | The console runs |
|---|---|---|
| **water offsets** (per step) | `0, 1, 2, 3, 4, 3, 2, 1` | **`0, 1, 2, 3, 2, 1, 0, -1`** |
| **flower frames** (per step) | `2, 3, 1, 1` | **`1, 1, 2, 3`** |

The water had the right *shape* — a ping-pong — but the wrong swing: the console's rotation runs
from **−1 to +3**, not 0 to +4, because the direction flips on `counter2 & 4` while the offset keeps
accumulating. The flower was simply a different order, and it missed that **flower1 shows for twice
as long** as the other two. Both are fixed and pinned by `tst_map_animation` (8 cases), which checks
that every water step is a **pure rotation** of the tile (never a second tile), that the offsets are
the console's, that the flower runs `1,1,2,3`, and that a **hack** animation byte behaves the way the
cartridge behaves.

**Owed: VRAM parity.** `tst_emu_parity` does not yet dump VRAM tiles `$14`/`$03` from the running
cartridge frame by frame. The tables above are read from the disassembly and pinned — but the
project's standard is that **the console judges**, and until that dump exists this one organ is
verified against the source rather than against the silicon.

## Determinism — the rule the tests depend on

Animation and **screenshot/visual-regression tests are natural enemies.** So:

- The renderer takes an explicit **frame number**; nothing reads a wall clock.
- The headless `screenshooter`, `tst_visual_regression` and the `--shot` harness all render **frame 0**
  unless told otherwise. A moving map must never make a test flap.
- The animation clock is a *view* concern. **It never touches the save.**
