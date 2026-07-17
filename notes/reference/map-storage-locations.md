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

✅ **The shipped coords are CORRECT — checked, not assumed** (2026-07-17). `import_hidden_items.py`
asserts every ROM row's `(x, y)` against `hiddenItems.json`'s at the **same index**, and all 54 + 12
agree. So the transposition trap above was *avoided* when this data was first brought over; the check
is now permanent, which matters because the arg/storage disagreement makes this the single easiest
thing in the file to get silently backwards.

### 2a. What is BURIED there — imported 2026-07-17

The coord table says *where*; it does not say *what*. The item comes from a **second table**:
`data/events/hidden_events.asm`, a run of `hidden_events_for <MAP>` blocks —

```asm
    hidden_events_for VIRIDIAN_FOREST
    hidden_event  1, 18, HiddenItems, POTION      ; <- (x, y, function, ARGUMENT)
    hidden_event 16, 42, HiddenItems, ANTIDOTE
```

- The **argument is the item id** (`HiddenItems` reads `wHiddenEventFunctionArgument` into
  `wNamedObjectIndex`). For coins the argument is `COIN + <n>` — **the offset IS the amount**
  (10 / 20 / 40; **260 coins buried in total** across the 12).
- ⚠️ **Same trap, second file:** `hidden_event` args are **(x, y, …)** but it stores `db \2` (y) then
  `db \1` (x). Both files agree on *arg* order, which is what makes the join by **(map, x, y)** exact
  — `(x, y)` alone collides across maps.
- `HiddenItems`/`HiddenCoins` are only two of the hidden-event functions; the same table holds bench
  guys, PCs and trash cans. Filter by function or you will import a Poké Center PC as an item.

Importer: `scripts/import_hidden_items.py` (additive-only, `--check`-idempotent) → adds `item` to
`hiddenItems.json` and `coins` to `hiddenCoins.json`. **21 distinct items.** `item` is spelled exactly
as `items.json` spells it ("GREAT BALL"), so it is both readable and a valid ItemsDB key — the
importer **fails** rather than inventing a name that does not resolve.

### 2b. 🐞 …and the items DB had never loaded. Not once.

**The reason this data looked "already brought over" but showed nothing.** Two real bugs, both found
while wiring the import, both fixed + pinned (`tst_db_integrity`, 13/13):

1. **`AbstractHiddenItemDB::load()` used `static bool once`.** A static local in a **base-class**
   method is **one static for the whole hierarchy**, not one per subclass — and `HiddenItemsDB` and
   `HiddenCoinsDB` share that exact method. `db.cpp` constructs **HiddenCoinsDB first**, so it tripped
   the guard and **`HiddenItemsDB::load()` returned early and loaded nothing**. All **54 hidden items
   were an empty store** for as long as the code has existed. Same bug in `deepLink()`.
   **Console-free negative control:** restoring the `static` semantics drops
   `HiddenItemsDB::getStoreSize()` to **0** against an expected 54 — reproduced deliberately, then
   reverted. Fix: per-instance `loaded` / `deepLinked` members.
2. **`HiddenCoinsDB` was missing `DB_AUTOPORT`** while its sibling had it → the db shared library
   never exported `HiddenCoinsDB::inst()`, so nothing outside the dll could link it.

⚠️ **Two lessons, and they generalise well past this DB:**

- **A `static` local in a shared base method is a hierarchy-wide flag.** If two singletons inherit one
  `load()`, only the first one runs. (`qmlRegister()` is *safe* from this only because each subclass
  overrides it — different functions, different statics.)
- **A loop-over-everything test passes vacuously on an empty store.** `allSubDbsLoadAndCount` asserted
  `>= 0` and went green on zero for years; the first cut of
  `everyHiddenPickupResolvesItsMapAndItem` **also passed while the bug was live**, because it iterated
  nothing. It now asserts the store is non-empty *first*. This is the same shape as the
  `emu-venv` gate lesson in [`../status.md`](../status.md): **the check must be able to fail.**
- 📝 `tst_db_coverage_fill.cpp` had **recorded the symptom** — *"the HiddenItems store is empty in the
  test data"* — and filed it as a quirk of the fixtures. It was the bug, in writing, unread.

### 2c. Items and coins are SEPARATE arrays — don't merge the lists

`MapDBEntry` now carries **`toHiddenItems` and `toHiddenCoins` apart**. They used to share one list
(harmless only because the items DB was loading nothing). They must not: they are two different save
bitfields (`0x299C` vs `0x29AA`) with **independent numbering from 0**, so a merged list makes an
entry's `ind` ambiguous. Each `HiddenItemDBEntry` now carries its own `ind` (**== its save bit**) and
`isCoin`.

## 3. Scripts — located only when they test the player's coords

Two mechanisms, and they produce **different shapes**:

| Mechanism | Files | Shape | Extracted |
|---|---|---|---|
| `dbmapcoord` table + `ArePlayerCoordsInArray` | most | a **tile** | **99** |
| raw `ld a, [wYCoord]` / `[wXCoord]` + `cp N` | 17 | a **row / column** | **37** |

**A script's trigger has an EXTENT — but the extent is NOT a box.** A `cp 1` on `wYCoord` in Pallet
Town is not a square; the trigger really is **the entire north row**, and the extractor must record
that. What we *draw* is the second question, and leadership answered it twice on 2026-07-17 — the
second answer is the one that stands:

> ~~*"if its a coord range test then put a box around the whole range"*~~ — **superseded, same day:**
>
> *"The range may span multiple blocks, its important to highlight the blocks separately not the
> range of blocks. So its still important to have only per-block or per-tile boxes depending on what
> it is it just adds the ranged tab onto other tabs the block or tile depending on what it is may
> have."*

So: **the extractor expands a range into the units it covers, and each unit gets an ordinary
fixed-size box** with the ranged script as **one more tab**. Pallet Town's north row is *N boxes,
each tabbed* — not one wide rectangle. **There is no `w`/`h`** (an earlier draft of this file said
there had to be; it was wrong). A range is therefore not a new geometry at all — it is a **spot that
lands on many locations**, the same shape as a shared event flag showing on every map it spans.

⚠️ **The unit belongs to the storage KIND, not the box** — *"use the measurement its suppose to be"*
(leadership, 2026-07-17). The model must let a kind declare its unit rather than assuming one. Which
unit each kind actually uses is settled below, against the cartridge.

## 3a. The units — there are THREE, and the middle one is where everything lives

Prompted by leadership: *"arent some filter flags or coords or whatever for things in meansurement of
blocks … things like tile attributes would be examples of tile measurements"*. Half right, and the
half that's wrong matters — **so this got checked rather than agreed with.**

| Unit | Size | What is measured in it |
|---|---|---|
| **Tile** | 8×8 px | **tile attributes** — collision, ledges, water, counters, bookshelves, warp tiles. ✅ leadership's instinct, exactly right |
| **Half-block** *(the walk grid)* | **16×16 px = 2×2 tiles** | **`wYCoord`/`wXCoord` — and therefore objects (→ filter flags), warps, signs, hidden items + coins, script coord triggers** |
| **Block** | 32×32 px = 4×4 tiles | the `.blk` map data, `wCurMapWidth`/`Height`, the border block (`wMapBackgroundTile`), connections |

**The proofs, all from `pret/pokered`:**

- **`wYCoord` counts HALF-blocks, and the game says so itself** — `engine/gfx/tilesets.asm`:
  ```asm
  ld a, [wYCoord]
  and $1                  ; <- the LOW BIT of the coord ...
  ld [wYBlockCoord], a    ; <- ... IS "which half of the block am I in"
  ```
  and the consumer is literally named **`.adjustForYCoordWithinTileBlock`** (`home/overworld.asm`).
  If `wYCoord` were a block coord, `& 1` would be meaningless. A block is **2×2** of these.
- **Warps ride the player's grid** — `CheckWarpsNoCollision` does `ld a, [wYCoord]` → `ld d, a`, and
  `CheckWarpsNoCollisionLoop` compares the warp's stored Y directly against `d`. Same unit, no
  conversion.
- **Objects and signs, same grid** — `macros/maps.asm`: `object_event` emits `db \2 + 4` / `db \1 + 4`
  (the +4 sprite bias, same grid), `bg_event` emits `db \2, \1, \3`, `warp_event` emits
  `db \2, \1, \4 - 1, \3`.

⭐ **So NOTHING we box is block-scoped, and a block-sized box would be a lie:** it spans 2×2 walk
squares and therefore cannot tell two adjacent warps apart. **The 16×16 boxes already shipping are
correct — the word was wrong, not the size.** 16×16 is a **half-block**; it is not a tile (8×8) and
not a block (32×32). ⚠️ This file and `tiles.md` have both been calling the walk grid "tile level"
(`tiles.md`: *"Gameplay happens at the tile level (you walk in half-blocks = 2×2 tiles)"* — the
parenthesis contradicts the sentence). **Rename, don't resize.**

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
