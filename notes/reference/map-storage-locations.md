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
| **In-game trades** | `0x29E3` | **9 of 10** — the trader's own tile | **Exact.** (map id, text id) → trade. 1 unused, **no coords** → General. [`in-game-trades.md`](in-game-trades.md) |
| **Town visited** | `0x29B7` | **YES** — the whole town | **Exact.** bit *i* == **map id** *i*. **11**. A map, not a tile. [`town-visited.md`](town-visited.md) |
| **The rods** | `0x29D4` b3-5 | **YES** — the Fishing Guru's tile | **Exact.** 3 bits, 3 houses. [`world-completed.md`](world-completed.md) |
| **Saffron guards** | `0x29D4` b6 | **YES** — each gate's guard | **Exact**, and **one bit on 4 maps** (shared) |
| **Lapras / starter** | `0x29DA` b0/b3 | **YES** — the giver's tile | Lapras: Silph 7F (1,5). Starter: Oak's Lab **+ read on Red's House 1F** (shared) |
| **Ever met a nurse** | `0x29DA` b2 | ⚠️ **every Poké Center's nurse** | written by the **engine**, not a map script — ~10+ maps, one bit. Placement **undecided** |
| **Started the Elite 4** | `0x29E0` b1 | **NO** — set on **map load** | belongs to Lorelei's Room + the Lobby; **no x/y, no box** |
| Boulder switches, spin tiles, … | various | **YES**, but their own readers | **11 files** — not yet extracted |

**The two GROUP kinds, and they are orthogonal** (leadership, 2026-07-17). Getting these confused is
how a group ends up lying about what it contains:

| Kind | Means | Affords | Members so far |
|---|---|---|---|
| **Shared** *(shipped)* | **ONE bit**, on **several map pages** | the same switch, wherever it's relevant | Silph Co's flags (12 floors) · **Saffron guards** (4 gates) · **the starter** (2 maps) · **ever met a nurse** (every Poké Center) |
| **Alike** *(new)* | **DIFFERENT bits**, the **same kind of thing**, one per place | **view the whole group at once** · **check / uncheck all** | **Towns** (11) · **Trades** (10) · **Hidden items** (54) · **Hidden coins** (12) · **Fishing rods** (3) |

⚠️ **Hidden items and coins are TWO alike groups, never one** (leadership, 2026-07-17) — the same
ruling as §2c, for the same reason: separate save arrays (`0x299C` / `0x29AA`), independent numbering
from 0, so a merged group makes an entry's bit ambiguous.

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

### 2b. …and the items DB had never loaded. Not once.

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
- `tst_db_coverage_fill.cpp` had **recorded the symptom** — *"the HiddenItems store is empty in the
  test data"* — and filed it as a quirk of the fixtures. It was the bug, in writing, unread.

### 2c. Items and coins are SEPARATE arrays — don't merge the lists

`MapDBEntry` now carries **`toHiddenItems` and `toHiddenCoins` apart**. They used to share one list
(harmless only because the items DB was loading nothing). They must not: they are two different save
bitfields (`0x299C` vs `0x29AA`) with **independent numbering from 0**, so a merged list makes an
entry's `ind` ambiguous. Each `HiddenItemDBEntry` now carries its own `ind` (**== its save bit**) and
`isCoin`.

## 2d. The macro traps that hide a THIRD of the game's flag writes (2026-07-17)

Both found by writing `import_map_storage_spots.py` and **checking counts against the source**
rather than trusting a regex that looked right. Both fail *silently* — the parse succeeds, the data
is just quietly incomplete, which is the worst failure mode this project has.

**1. `SetEvent` has EIGHT siblings.** Counted in `pret/pokered/scripts/`:

| Macro | Sites | |
|---|---|---|
| `SetEvent` | 105 | the obvious one |
| `SetEventReuseHL` | 17 | pure codegen (reuses HL) — semantically identical |
| `SetEventAfterBranchReuseHL` | 14 | ditto |
| `SetEventForceReuseHL` | 1 | ditto |
| `ResetEvent` | 12 | |
| `ResetEventReuseHL` | 8 | |
| `ResetEvents` | 8 | **plural** — several flags at once |
| `SetEvents` | 4 | **plural** |
| `SetEventRange` | 7 | **a whole range** of flags |
| `ResetEventRange` | 2 | |

A `\b(SetEvent|ResetEvent)\s` matches **none of the suffixed ones** — the `\s` fails against the
`R` of `ReuseHL` — so **61 of 178 sites vanish without a word**. ⚠️ The 16f-a extractor
(`extract_map_storage_locations.py`) uses exactly that regex, so its *"117 SetEvent/ResetEvent
sites"* is an **undercount**; the importer is the corrected reader.

⚠️ **Ranges do not always name `EVENT_` constants:**
`ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END, 1` names two block
markers that `events.json` has never heard of (it only carries `pretName`s). Resolving them needs
`constants/event_constants.asm` parsed properly: `const_def` (counter := 0) · `const NAME`
(NAME := counter++) · `const_skip N` (counter += N — **the padding gaps**) · `DEF X EQU
const_value[ - 1]` (an alias for wherever the counter is). **510 constants**, and the aliases are
the only way the 9 range sites resolve.

**2. `predef_jump` is a Show/Hide, and demanding `predef ` drops it.** It is `predef` + `ret` — the
tail-call form the game uses when the toggle is the routine's **last act**, which is exactly when
it is the *climax* of a phase. **12 of the 83** Show/Hide sites use it. Pallet Town proves it in
one routine:

```asm
    ld a, TOGGLE_DAISY_SITTING   / predef      HideObject   ; matched
    ld a, TOGGLE_DAISY_WALKING   / predef_jump ShowObject   ; MISSED without (?:_jump)?
```

Daisy would have been recorded as **hidden and never shown again**. Pinned by
`tst_map::phases_tellTheMapsStoryInOrder`.

⚠️ **And the toggle must be captured WITH its verb** (`ld a, TOGGLE_x … predef(_jump)? (Show|Hide)Object`).
Matching `ld a, TOGGLE_x` alone and separately asking *"does this routine contain a Show/Hide?"*
cannot say **which** was shown and which was hidden — and Pallet Town's phase 5 does **one of
each**.

## 2e. What each PHASE of a map does — the story, in order (2026-07-17)

> **Fairy Fox:** *"the event flags not only need to keep track of the map locations that called
> them, they need to keep track of which script phases a map goes through like (First phase flags)
> (second phase flags) like which scripts and locations did what to them turning them on off
> etc... have the flags be aware of there states across scripts"* — and, for filter flags:
> *"i need them to be aware of which are on and off and where and by whom"*.

A flag's **action is never flattened away**: a set and a reset of the same flag are opposite facts,
and a union of them says nothing. So every write keeps its **phase** (the `wCurMapScript` value —
the same number the "Current script" dropdown holds, so the story and the dropdown speak one
language), its **routine**, its **action**, and whether it was reached **via the chain**.

Pallet Town, straight out of the importer, and it reads like the story it is:

```
phase 0  SCRIPT_PALLETTOWN_DEFAULT        turns ON  Oak Appeared In Pallet
phase 1  SCRIPT_PALLETTOWN_OAK_HEY_WAIT   shows     Prof. Oak
phase 5  SCRIPT_PALLETTOWN_DAISY          turns ON  Daisy Walking
                                          turns ON  Pallet After Getting Pokeballs 2
                                          shows     Daisy Walking
                                          hides     Daisy Sitting
```

- **`scriptPhases` is NOT limited to located routines.** A phase that writes its flags from a text
  box has no tile and is still a phase of the map. **28 maps / 61 phases** change the world.
- ⚠️ **Filter actions are recorded as the game's VERB, not the bit.** A missable's bit is
  **set = HIDDEN**, so `ShowObject` *clears* it. "Shown/hidden" is what a person means; the
  inversion is precisely what gets muddled.
- Model: `MapDBEntryFlagWrite` / `MapScriptPhase`; surfaces `MapModel::mapPhases()` and
  `MapModel::flagHistory(ind, isEvent)` — the latter walks **all 249 maps**, because a flag is not
  owned by one (Silph Co's bits span 12 floors).
- ⚠️ **This awareness is for the PANEL LIST, not the tabs** (leadership, 2026-07-17: *"we dont need
  too much ui clutter just have the tabs as i describe them"*). The canvas stays quiet.

## 2g. THE STANDARD — the whole canvas, in one place (leadership, 2026-07-17)

Fairy Fox had to say *"there needs to be proper standardization"* **three times**, and each time a
new cut of `MapBlockHotspot.qml` invented a local rule instead. The standard is written once, here
and at the top of that file, and nothing is allowed to disagree with it.

**Why it kept failing, and it was never the individual bugs:** the canvas had **four** systems —
`MapSprite`, `MapWarp` and `MapSign` each drew, hovered and selected *themselves*, and the tab layer
was bolted on beside them. So the map **answered differently depending on what you pointed at**. You
cannot polish four disagreeing systems into one; every fix moved the inconsistency somewhere else.

| # | The rule | Her words |
|---|---|---|
| **1** | **Everything on a map is the same kind of thing** — a person, the player, a door, a sign, a filter flag, a script trigger, a buried item, an event flag, grass, water. Each is a **spot**, on a **block**, with an **outline** and a **tab**. No category is scenery; none is unreachable. | *"Like anything else they need to be rendered and have a tab"* · *"the npcs scriptable stuff other things they need an outline too"* |
| **2** | **The OUTLINE says what you can DO.** solid+filled = movable (player, people, doors, signs). dashed+hollow = fixed (scripts, filter flags, buried items, event flags, tile traits) — you change what it *does*, never where it *is*. One movable makes the whole **cell** solid. | *"stuff draggable, deletable, insertable… should have a solid fill and box, stuff that allows editing things from fixed locations like scripts and stuff should have a dashed outline and not be filled"* |
| **3** | **The COLOUR says which LAYER it belongs to** — the same swatch the Layers panel paints, so **the panel is the legend**. Never a code to learn off the map: hovering says it in words. | *"im seeing some kind of pink tab it says fisher 2, but why is it pink"* |
| **4** | **Tabs live at the TOP of the block, and ONLY on mouseover.** One horizontal strip: **non-tile first (left) · GAP · tile traits (right)**. The split is the **unit** (walk grid 16×16 vs tile 8×8). | *"the tabs are supposed to be on the top and only on mouseover"* · *"i said the tile traits go right not left i said the non tile traits go left"* |
| **5** | **A tab is a HANDLE, not a label.** Hover → lights its own thing. Click → **selects** it *and* opens its editor. | *"mouse over is important"* |

⚠️ **The "why is it that colour" bug was NOT a colour bug.** *"there seems to be other color tabs
around sprites that are yellow looking and when clicking on them they take you to the sprite details
page"* — yellow **is** the Warps layer, correctly. The fault was that clicking a tab **opened** the
Details panel **without selecting** the thing, so the panel went on showing the previous selection
(the sprite). A door's tab appeared to open a sprite's page. **A selection bug wearing a colour's
clothes** — and worth remembering as a diagnostic shape: when a colour "means the wrong thing",
check what the click actually did before touching the palette.

⚠️ **Three defaults that were never rules, only my inventions**, each of which she had to catch:
*"1 spot = no tabs"* (made the map inconsistent — a two-thing block was live, the water block beside
it dead), *tabs shown permanently* (dotted the whole map with furniture), and *tile traits with
`section: ""`* (which, because the hit areas were `enabled:` only when a section existed, switched
**water and grass completely off** — no hover, no tooltip, no click, on a route that is mostly
water). **If a rule isn't in the table above, it isn't a rule.**

## 2f. The canvas's visual language, and the tab strip (leadership, 2026-07-17)

Five rulings, in the order she gave them. Together they are a **language**, not a set of looks —
each one says what you can *do* with a thing before you touch it.

| | The rule | Why it is not arbitrary |
|---|---|---|
| **Save data is ON** | *"anything related to the save file is on by default"* — and *"even the rom-only tiles like grass and water … because you can change the pokemon in them and also change whats grass in the map state"* | The test is **not** "is it drawn from the ROM?" (nearly every tile is) but **"is there something here the save lets you change?"**. This is a save editor; what the save remembers is the point. |
| **Solid + filled = movable** | *"stuff draggable, deletable, insertable, etc.... should have a solid fill and box"* | Warps, signs, people. |
| **Dashed + unfilled = fixed** | *"stuff that allows editing things from fixed locations like scripts and stuff should have a dashed outline and not be filled"* | A script trigger sits where the cartridge tests coords; a hidden pickup's tile **is** its save bit's identity. You edit what they do, never where they are. |
| **One movable makes the CELL solid** | *"if there are multiple tabs fill and use solid line only if any of the tabs contain one thing that matches the rules above"* | The box is the handle for the **cell**. If anything in it can be picked up, the cell can. A dashed box beside a solid one inside one cell would say two things about one target. |
| **Only movable tabs drag** | *"tabs can be dragged out directly and dragged in snapping in… "* + *"i mean tabs that can be moved not fixed ones"* | Not built — the movable kinds aren't spots yet. |

⚠️ **This RETIRES the old solid-vs-dashed meaning** (solid = shown, dashed = hidden), which spent
the same ink on a different question. A switched-off object now says so with the **⚑** and a fainter
line; outline *style* is reserved for *"can I move this?"*.

**The strip: non-tile tabs LEFT, tile traits RIGHT.** The first cut put both on the left with a gap
between them and was corrected — *"i said the tile traits go right not left i said the non tile
traits go left"*. Opposite sides, so the two families can never read as one strip.

⚠️ **ONE TAB PER TRAIT PER BLOCK.** A block is 4×4 tiles, so a grassy block holds **sixteen** grass
tiles. Filing each as its own spot stacked sixteen identical "Grass" tabs and overflowed into the
next block — an unbroken grey bar down the whole map. Sixteen tabs that all say the same thing and
go to the same place **disambiguate nothing**, which is a tab's only job. The truthful 8×8 highlight
is not lost: `MapEngine::overlay()` already paints every tile at its real size. **The tab is the
handle; the overlay is the highlight.**

⚠️ **A spot's HIGHLIGHT is drawn once, by the block owning its top-left** — even though it is *filed*
on every block it touches (that is the aggregation rule, and it is right for the tabs). Without that
guard Pallet Town's north-row trigger drew its full-width rectangle **ten times**, stacking a
translucent range into a solid bar. Both of these were caught by the **mandatory screenshot review**
and neither was visible in the code.

**The density question, answered with numbers.** Leadership asked *"why does a block have 40
tabs?"* — prompted by me miscounting aloud. **It doesn't.** Pallet Town's busiest block carries
**four** (1 script + its 3 event flags); the row across the top is **ten blocks × four**, not one
block × forty. It spans them because the cartridge's trigger genuinely is the whole row:

```asm
ld a, [wYCoord]
cp 1 ; is player near north exit?   ; <- no X test at all
```

**99 located spots are single tiles** (`dbmapcoord`) with exact places; only **37** are ranges.
Pinned by `tst_map::hotspots_aBlockOnlyCarriesWhatOverlapsIt`. **Describe what the code does, not
what you think you saw** — the same lesson as the screen-box wording correction.

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

**So NOTHING we box is block-scoped, and a block-sized box would be a lie:** it spans 2×2 walk
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

**Consequence not yet handled: the script-step CHAIN.** The trigger routine and the routines it
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
