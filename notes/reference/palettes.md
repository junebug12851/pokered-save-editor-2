# Palettes and "contrast" — including the six glitch palettes

> **Status: IMPLEMENTED and VERIFIED (2026-07-12).** All ten contrast values checked against the **real
> console's palette registers** — zero mismatches (`scripts/emu/verify_palettes.py`).
>
> Twilight, before any of this was looked at: *"There are 4 contrast levels and 6 glitch palettes."*
> **She was exactly right, and here is why.**

## What "contrast" actually is

The save byte at **`0x2609`** — `AreaGeneral::contrast`, which is WRAM **`wMapPalOffset`**.

It is not a brightness dial. It is an **offset the game subtracts from a pointer**:

```asm
LoadGBPal::                     ; home/fade.asm
    ld a, [wMapPalOffset]
    ld b, a
    ld hl, FadePal4
    ld a, l
    sub b                       ; hl = FadePal4 - contrast
    ...
    ld a, [hli] : ldh [rBGP], a
    ld a, [hli] : ldh [rOBP0], a
    ld a, [hli] : ldh [rOBP1], a
```

`FadePal1`…`FadePal8` are **eight contiguous 3-byte palettes** (BGP, OBP0, OBP1) used for fading in and
out. `LoadGBPal` walks *backwards* into that table by `contrast` bytes and takes the next three.

**Land on a multiple of 3 and you get a real palette. Land anywhere else and you read across the seam
between two of them** — three bytes that were never meant to be a palette at all.

## The ten values

| contrast | lands on | `rBGP` | what it looks like |
|---|---|---|---|
| **0** | FadePal4 | `E4` | **Normal** (the identity palette) |
| 1 | 4/5 seam | `E4` | **glitch** — background survives; the *sprite* palettes are wrecked |
| 2 | 4/5 seam | `E4` | **glitch** — same |
| **3** | FadePal3 | `F9` | **Dark** |
| 4 | 3/4 seam | `F8` | **glitch** — midtones crushed |
| 5 | 2/3 seam | `FE` | **glitch** — nearly black |
| **6** | FadePal2 | `FE` | **Very dark** — the *"you need FLASH"* cave palette |
| 7 | 1/2 seam | `FF` | **glitch** |
| 8 | 1/2 seam | `FF` | **glitch** |
| **9** | FadePal1 | `FF` | **Black** |

**Four levels (0, 3, 6, 9). Six glitch palettes (1, 2, 4, 5, 7, 8).** Exactly as Twilight said.

> Note contrast **1 and 2 look normal on the map** — `rBGP` is still `E4`. Their damage is in `rOBP0`/`rOBP1`,
> the **sprite** palettes. So they will only *show* once the player and the NPCs are drawn. That is not a
> bug in the render; it is what the console does.

Past 9 the pointer walks out of the table entirely and the game reads whatever ROM precedes it. We don't
ship the ROM, so `backgroundPalette()` returns **−1** there: unknown, and said so, never invented.

## How the render applies it

The tileset art is drawn in exactly four greys — `255 / 170 / 85 / 0` — and it was captured under the
**identity** palette. So **a pixel's shade *is* its colour index**, which makes the remap exact rather than
approximate:

```
index  = shade(pixel)                    // 255->0, 170->1, 85->2, 0->3
shade' = (rBGP >> (2 * index)) & 3       // what the hardware says that index comes out as
```

That is the whole thing. **We are not simulating "broken" — we apply the byte the console would genuinely be
holding**, which is why the glitch palettes render as the real article rather than an impression of one.

## Verification

`scripts/emu/verify_palettes.py` writes each contrast value into a save (recomputing the checksum — a bad
one is *rejected outright* by the game, so a broken save can never fake a pass), boots the real ROM, and
reads the palette registers the console actually ends up holding (`rBGP $FF47`, `rOBP0 $FF48`, `rOBP1
$FF49`).

> **10 of 10. Zero mismatches.** Including all six glitch palettes.

Unlike the connection structs, this **is** verifiable through generated saves: `rBGP` is a hardware
register, recomputed live by `LoadGBPal` on every map load — it isn't restored from the save snapshot. (See
[`map-connections.md`](map-connections.md) for the case where that *isn't* true, and why.)

Pinned in C++ by `tst_map::palettes_matchTheConsoleForEveryContrastValue` — with the console's own bytes
written into the test, not the disassembly's.

## Where this goes next

- **Sprites.** `rOBP0`/`rOBP1` are already computed and unused. The moment the player and NPCs are drawn,
  contrast 1 and 2 will start showing their real damage.
- **The SGB / colour palettes** (`PAL_ROUTE`, `PAL_PALLET`, … in `constants/palette_constants.asm`) are a
  *different* system — per-town colour on a Super Game Boy. Not this. Not done.
