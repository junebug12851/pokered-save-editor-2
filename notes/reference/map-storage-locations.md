# Map storage locations — what has a place on the map, and what does not {#map-storage-locations}

The deep dive behind [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 16f**.

> **Fairy Fox, 2026-07-17:** *"you need to do a deep dive and figure out the coordinates and boxes and
> stuff for the scripts, event flags, filter flags, and more stuff ill add later"*

The Map screen wants to draw a **box** around everything the save remembers about a place, and put that
thing's switches on it as **tabs**. To do that you have to know *where each kind of stored thing lives*
— and the answer is different for every kind, which is why this note exists.

Extractor: `scripts/extract_map_storage_locations.py` → `tmp/event-flags/storage_locations.json`
(git-ignored). Everything below is **counted from `pret/pokered`**, not estimated.

## The one-line answer

| Storage | Save | Has a location? | How exact? |
|---|---|---|---|
| **Filter flags** (missables) | `0x2852` | **YES** — an object's tile | **Exact.** Shipped + drawn (Phase 16c/d) |
| **Hidden items** | `0x299C` | **YES** — its own coord row | **Exact.** bit *i* == row *i*. **54** |
| **Hidden coins** | `0x29AA` | **YES** — its own coord row | **Exact.** bit *i* == row *i*. **12** |
| **Scripts** | `0x289C` (step) | **SOMETIMES** — when they test coords | **99 tiles / 37 ranges**, across **42** files |
| **Event flags** | `0x29F3` | **NO — and this is the finding** | reachable only *through* a script |
| Boulder switches, spin tiles, … | various | **YES**, but their own readers | **11 files** — not yet extracted |

## 1. Event flags have NO location — stop looking for one

**This is the headline, and it kills a whole line of work.** An event flag is a bit the *code* reads.
Nothing about it names a tile. That is exactly why `extract_flag_locations.py` only ever found **14**
object↔event links across 223 maps: it was hunting a relation that **does not exist**.

So: **no event-flag boxes, and no x/y on a flag.** Inventing one would be inventing a fact. Event
flags reach the map only by being *written by a script that has a location* — see §3.

## 2. Hidden items + coins — the cleanest data in the dive

`wObtainedHiddenItemsFlags` (**54** used of `MAX_HIDDEN_ITEMS` = 112) and `wObtainedHiddenCoinsFlags`
(**12** of 16). **Both are inside the saved block** (`wMainDataStart` .. `wMainDataEnd`), and **we
already model both** — `WorldHidden`, file `0x299C` / `0x29AA`. The counts match the ROM exactly
(54 `hidden_item` rows, 12 `hidden_coin` rows), and the offsets reconcile: 112 bits = 14 bytes from
`0x299C` lands precisely on `0x29AA`.

**The link is index identity, not inference** — `HiddenItems` calls `FindHiddenItemOrCoinsIndex`, then
tests bit *n* of the array where *n* is the row's position in `HiddenItemCoords`. So:

> **hidden item's save bit `i`  ==  row `i` of the coord table  ==  a real (map, x, y).**

**66 boxes that need no guessing at all.** ⚠️ These do **not** use `wEventFlags` — a hidden item is its
own kind of storage, not an event. That is the *"more stuff"* she meant, and it was next on her list.

⚠️ **Arg order trap:** the source reads `hidden_item VIRIDIAN_FOREST, 1, 18` = **(map, x, y)** (the
file's own comment says so), but the macro *stores* `db \1, \3, \2` = map, **y, x**. Read the ARGS.
Read the storage order instead and every coordinate comes out transposed.

## 3. Scripts — located only when they test the player's coords

Two mechanisms, and they produce **different shapes**:

| Mechanism | Files | Shape | Extracted |
|---|---|---|---|
| `dbmapcoord` table + `ArePlayerCoordsInArray` | most | a **tile** | **99** |
| raw `ld a, [wYCoord]` / `[wXCoord]` + `cp N` | 17 | a **row / column** | **37** |

**A script box is the EXTENT of its trigger, not a tile** — leadership, 2026-07-17: *"if its a coord
range test then put a box around the whole range."* Pallet Town's `cp 1` on `wYCoord` is not a square;
it is **the entire north row**. So box geometry needs **w/h in tiles**, unlike the fixed 16×16 the
filter-flag boxes use. This is more truthful than a tile would be: the trigger *is* the whole row.

**The worked example** (`scripts/PalletTown.asm`) — Oak pulling you out of the grass, and the best
single argument for the tabs, because one step writes **three kinds** of storage:

```asm
PalletTownDefaultScript:
    CheckEvent EVENT_FOLLOWED_OAK_INTO_LAB   ; an EVENT flag GATES it
    ret nz
    ld a, [wYCoord]
    cp 1 ; is player near north exit?        ; the LOCATION (a row, not a tile)
    ret nz
    SetEvent EVENT_OAK_APPEARED_IN_PALLET    ; writes an EVENT flag
    ld [wPalletTownCurScript], a             ; advances the SCRIPT STEP
PalletTownOakHeyWaitScript:
    ld a, TOGGLE_PALLET_TOWN_OAK
    predef ShowObject                        ; writes a FILTER flag
```

⚠️ **The script is gated by an EVENT flag, not "allowed there by a filter flag"** — the filter flag is
what it *changes*. Direction matters when you draw it.

🚩 **Consequence not yet handled: the script-step CHAIN.** The trigger routine and the routines it
leads to are *different* routines, and the flags are spread across all of them
(`PalletTownDefaultScript` → `OakHeyWait` → `OakWalksToPlayer` → …). The extractor attributes flags
**per routine**, so a trigger currently shows only *its own* writes — **13** located spots write events
by that measure. Following `ld [w<Map>CurScript], a` to union the chain is the next step, and it is
what will make the tabs complete.

## 4. Filter flags → scripts → event flags (her chain, and it is exact)

> *"i think filter flags also point to scripts which change event flags"*

Confirmed, mechanically. `scripts/BillsHouse.asm`:

```asm
    ld a, TOGGLE_BILL_POKEMON        ; the FILTER flag (an object with a tile)
    ld [wToggleableObjectIndex], a
    predef HideObject
    SetEvent EVENT_BILL_SAID_USE_CELL_SEPARATOR   ; the EVENT flag, next line
```

**22 of 224** script files toggle an object *and* write an event flag. So: **object tile → filter flag
→ script → that script's event flags** — the only honest route from a tile to an event flag, and the
basis for event tabs on an object's box.

Counts: **71** `Show/HideObject` sites · **117** `SetEvent`/`ResetEvent` sites · **22** files with both.

## 5. Known gaps — stated, not hidden

- **11 files have coord tables we do not read**: `PokemonMansion3F`, `PokemonTower7F`,
  `RocketHideoutB2F/B3F`, `SeafoamIslands1F/B1F/B2F`, `VictoryRoad1F/2F/3F`, `ViridianGym`. They are
  **other mechanisms** with their own readers — boulder switches, spin tiles, gym floor puzzles — not
  script triggers being missed. Each is its own storage kind and its own future phase.
- **`ViridianGym`** reads `[wYCoord]` into a register and compares later — beyond a regex window.
  Real, but it needs a smarter reader than a pattern match.
- **`data/events/card_key_coords.asm` is UNUSED** — pret's own comment: *"probably supposed to be door
  locations in Silph Co., but they are unused."* Do **not** draw them. The per-map `.GateCoordinates`
  tables (read by each floor's `SetCardKeyDoor*Script`) are the live ones, and those **are** extracted.

## Two parser traps that cost real data

Both silently dropped whole maps on the first runs, and both are the sort of thing that looks like
"the game doesn't do that" rather than "our tool can't see it":

1. **Coord tables are usually LOCAL labels** — `.PlayerCoordsArray:`. A leading dot is not a letter, so
   a `[A-Za-z_]`-anchored regex misses them: **31 of 53** files vanished (Route 24, every Silph Co
   floor, Seafoam, Victory Road).
2. **The colon is OPTIONAL in RGBDS.** `.Route22RivalBattleCoords` (no colon) is a label. Requiring the
   colon lost **Route 22's** rival-battle coords — on the very map that already produced a false
   positive once.

**The standing rule:** extract only what the source **proves**. A spot whose location cannot be
established gets **NO box** — never a guessed one. Static co-location is a lead, never evidence
([`../decisions/rejected.md`](../decisions/rejected.md)).
