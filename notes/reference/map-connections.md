# Map connections — the strips, in full

> **Status: IMPLEMENTED and VERIFIED (2026-07-12).**
> **All 78 connections in the game reproduced from `maps.json`, byte-for-byte against the compiled structs
> in the real cartridge — zero mismatches.** The resulting border ring is byte-identical to the console's
> `wOverworldMap` (`tst_emu_parity::theBorderRing_carriesTheConnectedMaps`).
>
> Twilight's warning, and she was right: *"connection strips are notoriously complicated and error prone,
> they have layers of complexity, the algorithms are very complicated."* The maps must render so accurately
> that **glitches render accurately**. So nothing here is reasoned-about-and-hoped: every claim below is
> checked against the cartridge (`scripts/emu/verify_connections.py`) and the console's RAM
> (`tst_emu_parity`; see [`emulator-verification.md`](emulator-verification.md)).
>
> **She was also right that her own formulas might be wrong. They are — see "the length" below.**

## What a connection actually is

A map's 3-block border ring is **not** decoration and it is **not** just the border block. The game fills
the ring with the border block and then **bleeds the neighbouring maps' edges over the top of it**. Pallet
Town's ring is really *Route 1's bottom three rows* and *Route 21's top three rows*. That is why you can see
into the next route before you walk there, and why the walk across is seamless: you are already standing on
its blocks.

Each map header declares up to four connections:

```asm
map_header PalletTown, PALLET_TOWN, OVERWORLD, NORTH | SOUTH
connection north, Route1,  ROUTE_1,  0
connection south, Route21, ROUTE_21, 0
```

The last argument is the **offset** — how far the neighbour is shifted along the shared edge, in blocks. It
is **signed**, and it is the source of nearly all the complexity. Real values in the game run from **−27 to
+27**.

## The `connection` macro — the whole thing

`macros/scripts/maps.asm`. It is a *pure compile-time function* of the direction, the two maps' dimensions,
and the offset. It emits an 11-byte struct that `LoadMapHeader` copies into WRAM
(`wNorth/South/West/EastConnectionHeader`, `map_connection_struct`):

| bytes | field | meaning |
|---|---|---|
| 1 | `ConnectedMap` | the neighbour's map id |
| 2 | `ConnectionStripSrc` | **a ROM pointer into the neighbour's BLOCK data** |
| 2 | `ConnectionStripDest` | a pointer into `wOverworldMap` — where the strip lands in our ring |
| 1 | `ConnectionStripLength` | N/S: blocks per row. E/W: **number of rows** |
| 1 | `ConnectedMapWidth` | the neighbour's width — the src row stride |
| 1 | `ConnectedMapYAlignment` | the player's Y when he crosses |
| 1 | `ConnectedMapXAlignment` | the player's X when he crosses |
| 2 | `ConnectedMapViewPointer` | the neighbour's view pointer, for when he crosses |

### The clamp — layer one

```
_src = 0
_tgt = offset + 3
if _tgt < 2:            ; i.e. offset < -1
    _src = -_tgt
    _tgt = 0
```

Two numbers come out of one. **`_tgt`** is where the strip lands in *our* ring; **`_src`** is how far into
the *neighbour* the strip starts. A negative offset means the neighbour is shifted such that our ring would
have to start *before* the buffer — so instead of writing out of bounds, the game slides the **source**
along and pins the destination to 0. That is the whole trick, and it is why the two numbers are not
symmetric.

Note `_src` can be **−1** (`offset == -2`): the strip then starts one block *before* the neighbour's row.
No shipped map does this, but the arithmetic permits it and a hacked/glitch header could.

### The four directions

With `W`,`H` = the **neighbour's** size and `curW`,`curH` = **ours**:

| | `_blk` (src, from the neighbour's `_Blocks`) | `_map` (dest, from `wOverworldMap`) | `_win` (their view ptr) | `_y` | `_x` | `_len` |
|---|---|---|---|---|---|---|
| **north** | `W*(H-3) + _src` | `_tgt` | `(W+6)*H + 1` | `H*2 - 1` | `-2*offset` | `min(curW + 3 - offset, W)` |
| **south** | `_src` | `(curW+6)*(curH+3) + _tgt` | `W + 7` | `0` | `-2*offset` | `min(curW + 3 - offset, W)` |
| **west** | `W*_src + W - 3` | `(curW+6)*_tgt` | `(W+6)*2 - 6` | `-2*offset` | `W*2 - 1` | `min(curH + 3 - offset, H)` |
| **east** | `W*_src` | `(curW+6)*_tgt + curW + 3` | `W + 7` | `-2*offset` | `0` | `min(curH + 3 - offset, H)` |

and the emitted **length is `_len - _src`**.

Read the `_blk` column: north takes the neighbour's **last 3 rows** (`W*(H-3)`), south its **first** row,
west its **last 3 columns** (`+W-3`), east its **first**. The border is 3 blocks, and this is why.

### The length — layer two, and where implementations go wrong

`_len = min(curW + 3 - offset, W)`. **Both maps can limit it**: our width plus the border, shifted by the
offset — *or* the neighbour's own width, whichever runs out first. Then `_src` is subtracted, because the
strip already started that far in.

> ⚠️ **Our existing `MapDBEntryConnect::stripSize()` gets this wrong.** It branches on `fromWidth < toWidth`,
> which is *not* the macro's test (`curW + 3 - offset` vs `toW`). They differ whenever `curW < toW` but
> `curW + 3 - offset >= toW`. The **`flag`** field in `maps.json` (21 connections carry it) is consulted
> *only* in the SOUTH and EAST branches of that function and is, as far as the disassembly is concerned, a
> **patch for that divergence** — the real macro has no such concept. The correct implementation needs no
> flag at all.

## What `maps.json` kept — and how to recover the offset

`maps.json` does **not** store the raw offset. It stores the **post-clamp pair**:

```
stripMove   == _tgt - 3
stripOffset == _src
```

Which is invertible, because a non-zero `_src` can only have come from the clamp:

```
offset = (stripOffset != 0) ? (-stripOffset - 3) : stripMove
```

Checked against the real headers:

| header | real offset | `maps.json` | recovered |
|---|---|---|---|
| Route 4 → south → Route 3 | **−25** | `move=-3, off=22` | `-22-3 = -25` ✓ |
| Route 11 → east → Route 12 | **−27** | `move=-3, off=24` | `-24-3 = -27` ✓ |
| Route 2 → north → Pewter City | **−5** | `move=-3, off=2` | `-2-3 = -5` ✓ |
| Viridian City → north → Route 2 | **+5** | `move=5, off=0` | `5` ✓ |

So everything the macro needs is recoverable from data we already ship. **Nothing has to be invented.**

## Editing a connection — the human model (2026-07-15, briefed by Twilight)

Everything above is what the *game* reads. This is what a **person** should be handed, and it is the
whole reason v1's Map page was error-prone: it exposed the 11-byte struct raw (Map ID, UL Corner,
Strip Src, Strip Dest, Strip Width, X/Y Align) and asked a human to keep nine derived numbers
self-consistent by hand.

**A connection has exactly TWO human-meaningful inputs:**

1. **Which map** is the neighbour (`ConnectedMap` / our `mapPtr`).
2. **The offset** — one signed number (**−27…+27**), how far the neighbour slides along the shared edge.

**The other nine bytes are all derived** by the compile-time `connection` macro from (direction, both
maps' sizes, offset): `stripSrc`, `stripDst`, `stripWidth`, `width`, `xAlign`, `yAlign`, `viewPtr`, and
the post-clamp `_src`/`_tgt` pair. So the editor's job is the **derived-byte doctrine** applied to
connections: the user sets **map id + offset**, and we recompute the nine — exactly as `MapEngine` already
does, and exactly the invert-then-recompute round-trip proven in "What `maps.json` kept" above. Raw-byte
editing stays available (break-sync), for power users and for reproducing glitch connections.

**There is no rotation in the save, and there cannot be.** A north connection *always* reads the
neighbour's bottom 3 rows, a west connection its right 3 columns, etc. — the neighbour's block data is
read in its natural orientation; no byte anywhere expresses a rotated map. The meaningful equivalent a
person might want is **re-homing a connection to a different edge** (change the direction N↔S↔E↔W), which
just moves which flag bit is set + which 11-byte slot is written and recomputes the block for the new
direction. So "attach this neighbour to another side" is a real, representable action; "rotate the
neighbour" is not. The neighbour must always render in its natural orientation.

### The save footprint of an edit (what bytes a connection edit touches)

Verified against `AreaMap::load`/`save` (`expanded/area/areamap.cpp`):

| what | bytes |
|---|---|
| **Enable flags** | one byte at **`0x261C`**, bits **0=East, 1=West, 2=South, 3=North** |
| **North block** | 11 bytes at **`0x261D`** |
| **South block** | 11 bytes at **`0x2628`** |
| **West block** | 11 bytes at **`0x2633`** |
| **East block** | 11 bytes at **`0x263E`** |

Each 11-byte block is `mapPtr(1) · stripSrc(2, LE) · stripDst(2, LE) · stripWidth(1) · width(1) ·
yAlign(1) · xAlign(1) · viewPtr(2, LE)` (`MapConnData::save`). **Add** = set the flag bit + write the 11
bytes of that slot. **Delete** = clear the flag bit (byte fidelity: leave the stale 11 bytes as they lie
unless a value is deliberately written). **Move/offset** = rewrite (a subset of) the 11 bytes of the one
slot. Nothing outside the touched slot + the flags byte moves — the same fidelity rule as every other
object edit.

> ⚠️ Field-name gotcha carried from v1's struct: our `MapConnData::stripWidth` is the game's
> `ConnectionStripLength` (blocks-per-row for N/S, **rows** for E/W), and our `width` is the game's
> `ConnectedMapWidth` (the source row stride). Two different fields; do not conflate them in the UI.

## The copy itself (`LoadTileBlockMap`, `home/overworld.asm`)

The ring is filled with the border block **first**, then each present connection is copied over it:

- **`LoadNorthSouthConnectionsTileMap`** — 3 rows (`MAP_BORDER`). Each row copies
  `ConnectionStripLength` bytes, then advances **src by `ConnectedMapWidth`** (the neighbour's row stride)
  and **dest by `curW + 6`** (our buffer's stride).
- **`LoadEastWestConnectionsTileMap`** — `ConnectionStripLength` **rows** (that is the loop count here, not
  a width). Each row copies 3 bytes (`MAP_BORDER`), then advances src and dest by the same two strides.

So N/S length is a *width*, E/W length is a *height*. Same field, two meanings, depending on direction.

## Crossing the boundary (`CheckMapConnections`)

When the player walks off the edge (`wXCoord == $FF`, or `== wCurrentMapWidth2`, etc.), the game:

1. sets `wCurMap` to the connected map,
2. sets the player's coordinate on the new map from the **alignment** bytes (`_x`/`_y`), adding his position
   along the shared edge to the other axis,
3. takes the **connected map's view pointer** (`_win`) and walks it down by `(newCoord / 2)` rows —
   `stride = connectedMapWidth + 6` — to land on the right row,
4. `LoadMapHeader`, `LoadTileBlockMap` — and now *that* map's ring is filled from *its* neighbours.

## The banks — and why glitches are hard

`ConnectionStripSrc` is a **16-bit ROM address in the current map's bank**: `LoadTileBlockMap` calls
`SwitchToMapRomBank` (the **current** map's bank) before each strip copy, and then reads the *neighbour's*
blocks through it. If a neighbour's block data lived in a different bank, the game would read **whatever is
at that address in the current map's bank** — which is exactly the kind of thing that produces glitch maps.

> **Being verified:** whether every connected pair shares a bank in the real game
> (`verify_connections.py` reports any that don't).

This is the crux of rendering glitches accurately: the strip source is a **pointer into a ROM address
space**, not an index into a tidy per-map array. A strip can legitimately be asked to read *past the end* of
the neighbour's block data (a short map, a large offset, `_src = -1`) and the console will happily hand it
the bytes that follow — the next map's blocks, or something that isn't block data at all.

**To reproduce that faithfully we must model the address space, not the array.** We can: `maps.json` gives
every map a `bank` and a `dataPtr`, and we ship every map's blocks, so a per-bank image of the block regions
can be reconstructed exactly. Reads that land *outside* any known block region cannot be reproduced without
the ROM itself — and we will not ship the ROM. Those must be surfaced honestly (drawn as unknown), never
guessed.

## Verification — and one dead end worth remembering

`scripts/emu/verify_connections.py` finds **each map's header in the ROM** by its 10-byte signature
(tileset, height, width and three pointers — unique), reads the connection structs the macro compiled in
(N, S, W, E order), recomputes all 11 bytes from `maps.json`, and compares every field.

**Result: 78/78 connections, zero mismatches.** Every `stripSrc`, `stripDest`, `stripLength`,
`connectedMapWidth`, `yAlign`, `xAlign` and `viewPointer` in the game.

> **The dead end (worth knowing):** the first attempt drove the *emulator* — build a save that places the
> player on each map, boot, read the structs from WRAM. It cannot work, and the reason is interesting: **a
> Gen 1 save is a full WRAM snapshot.** It stores the live map header *and* the four connection structs, so
> the game does not re-read them on CONTINUE — it just restores them. (That is also *why* our `Area*`
> classes model all that live state.) Placing the player elsewhere by save-editing would mean writing the
> very structs we were trying to verify. The ROM has no such problem: it is the source the game itself
> reads from.
>
> The emulator still verifies the **result** — the ring those structs produce — in `tst_emu_parity`.

### What the verification also established

- **66 of 78 connections are cross-bank, and that is fine.** `SwitchToMapRomBank` is called with the
  **connected** map's id (`ld a, [wNorthConnectedMap]` immediately before it), so the strip reads in the
  *neighbour's* bank, not ours.
- **No strip in the shipped game reads outside the neighbour's own block array.** So per-map arrays would
  have sufficed — but we read through the address space anyway (below), because a hand-edited or glitched
  offset *can* run off the end, and it should then overrun exactly where the console overruns.

## How it is implemented

- **`BlocksDB::blockAt(bank, addr)`** — every map's blocks put back where they actually live in the
  cartridge (its `bank`, at its `dataPtr`). A strip source is then read as the console reads it: a
  **pointer**, not an array index. An address inside a region we hold gives the true byte *even if it
  belongs to a different map*. An address in ROM we don't hold (headers, scripts, text — we don't ship the
  cartridge) returns **−1**: unknown, and said so, never guessed.
- **`MapEngine::connectionOf()`** — the macro, exactly. No `flag`, no `fromWidth < toWidth` shortcut.
- **`MapEngine::applyConnections()`** — border block first, then N, S, W, E over the top, with the two
  different loop shapes.

## Still open — a DB question for Twilight

`MapDBEntryConnect::stripSize()` is wrong (above) and `maps.json`'s **`flag`** field exists only to patch
it. `MapEngine` does not use either — it recomputes from the macro — so nothing is broken today. But the DB
still carries a wrong formula and a field the real game has no concept of. Fixing that touches curated data
and a public DB API, so it is **hers to call**, not something to do unasked.
