# Town "visited" flags — eleven bits, one trap, and a name list that is wrong {#town-visited}

The research behind [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 18**.

> **Fairy Fox, 2026-07-17:** *"go ahead and do towns that way too, there are 11 towns i think im
> talking specifically about the persistent storage that says you have visited the town … have a
> visited checkbox near the top of persistent storage for places that take a visited one."*

**Her count is exact: `NUM_CITY_MAPS` = 11.**

This note carries **two findings that outrank the feature**: the flag is **re-set on load** for the
town you are standing in (§3), and **`fly.json` — the name list this data has always been drawn
through — is wrong for 6 of the 11 towns** (§4). The second one shipped in v1.

## 1. The one-line answer

| | |
|---|---|
| **Save field** | `wTownVisitedFlag` — `flag_array NUM_CITY_MAPS`, **2 bytes**, file **`0x29B7`–`0x29B8`** (WRAM `$D70B`–`$D70C`) |
| **Count** | **11**. Bits 11–15 are spare |
| **Bit math** | **bit `i` == map id `i`.** Not "a list order" — the map id itself (§2) |
| **Home** | `SECTION "Main Data"` — inside the saved block, immediately before `wSafariSteps` (`0x29B9`) |
| **What it's FOR** | **Fly.** `BuildFlyLocationsList` walks these bits to build the Fly menu. Nothing else reads them |
| **Already modelled?** | **Yes** — `WorldTowns`, `0x29B7`, `townCount = 11`. Offset and count both right |
| **Durable?** | **Mostly — but NOT for the town you're saved in** (§3). Console-verified |

**Address triangulated three ways**: derived from `wSafariSteps` = `0x29B9` (verified in
[`gym-safari-state.md`](gym-safari-state.md)) minus the 2-byte array; v2's `WorldTowns` reads
`0x29B7`; **v1 read `0x29B7`** too. All agree.

## 2. The bit IS the map id — proven, not inferred

`engine/overworld/toggleable_objects.asm`, the whole routine:

```asm
MarkTownVisitedAndLoadToggleableObjects::
	ld a, [wCurMap]
	cp FIRST_ROUTE_MAP
	jr nc, .notInTown       ; only maps BELOW the first route are towns
	ld c, a                 ; <- the flag index IS wCurMap
	ld b, FLAG_SET
	ld hl, wTownVisitedFlag ; mark town as visited (for flying)
	predef FlagActionPredef
.notInTown
```

`ld c, a` where `a = wCurMap`, straight into `FlagAction`. **The flag index is the map id, with no
translation.** Same `FlagAction` as the event and trade flags: byte `0x29B7 + i/8`, mask
`1 << (i%8)`.

So the eleven, in **their real bit order** (`constants/map_constants.asm`, and `NUM_CITY_MAPS` is
literally `const_value` after this block):

| Bit / map id | Town |
|---:|---|
| **0** | Pallet Town |
| **1** | Viridian City |
| **2** | Pewter City |
| **3** | Cerulean City |
| **4** | **Lavender Town** |
| **5** | **Vermilion City** |
| **6** | Celadon City |
| **7** | **Fuchsia City** |
| **8** | **Cinnabar Island** |
| **9** | **Indigo Plateau** |
| **10** | **Saffron City** |

✅ **Corroborated by a second, independent ROM table.** `data/maps/town_map_entries.asm`'s
`ExternalMapEntries` — the game's own town-map name list — is in exactly this order
(`PalletTownName, ViridianCityName, PewterCityName, CeruleanCityName, LavenderTownName,
VermilionCityName, CeladonCityName, FuchsiaCityName, CinnabarIslandName, IndigoPlateauName,
SaffronCityName`). Two tables, one order, no ambiguity.

## 3. ⚠️ THE TRAP — the town you are saved in RE-MARKS ITSELF on Continue

**This is the finding, and it is the one that changes the UI.** `MarkTownVisitedAndLoadToggleableObjects`
is the **first line of `LoadMapHeader`** — and `LoadMapHeader`'s famous early-out is **eleven lines
later**:

```asm
LoadMapHeader::
	farcall MarkTownVisitedAndLoadToggleableObjects   ; <- line 2006. Runs.
	ld a, [wCurMapTileset]
	ld [wUnusedCurMapTilesetCopy], a
	ld a, [wCurMap]
	call SwitchToMapRomBank
	ld a, [wCurMapTileset]
	ld b, a
	res BIT_NO_PREVIOUS_MAP, a
	ld [wCurMapTileset], a
	ldh [hPreviousTileset], a
	bit BIT_NO_PREVIOUS_MAP, b
	ret nz                                            ; <- line 2017. THE LINCHPIN BAILS HERE.
```

**The linchpin does not protect this flag, because the flag is written before it.** Every other
per-map thing we have researched — warps, signs, wild tables — survives a Continue *because*
`LoadMainData` sets `BIT_NO_PREVIOUS_MAP` and `LoadMapHeader` returns before touching them
([`warps.md`](warps.md)). `MarkTownVisited` sits **above** that return. It always runs.

**The consequence, stated as a user would hit it:**

> **You cannot un-visit the town you are standing in.** Clear Pallet Town's bit on a save that is
> sitting in Pallet Town, and the console sets it straight back on the next Continue — before the
> first frame. The edit is written to the file correctly and is then undone by the game.

⚠️ **But only when you're OUTDOORS in the town.** The guard is `cp FIRST_ROUTE_MAP` / `jr nc` — it
fires only when `wCurMap` is one of the 11 city maps. A save made **inside Red's house** has
`wCurMap` = the house's id, which is far above `FIRST_ROUTE_MAP`, so **nothing is re-marked**. Same
town, different answer, entirely because of which map the save is parked on.

**So the other ten bits are durable and the eleventh is conditional** — which is precisely the
`!`/amber-flag shape this project already has for `wPlayerDirection` and the `wStatusFlags3` family.
The UI must mark **the current map's own town row** as rewritten-on-load, dynamically. It is the
first field we have where *which* row wears the `!` depends on the save.

**This is a source read and it is load-bearing**, so it is not to be trusted until the cartridge
says so — the sprite-persistence pass was wrong in exactly this way. `scripts/emu/probe_town_visited.py`,
three questions:

1. Clear the bit for the town the save sits in → Continue → **is it back?** (predict: **yes**)
2. Clear a *different* town's bit → Continue → **is it still clear?** (predict: **yes** — the control)
3. Save **inside a building** in that town, clear the bit → Continue → **still clear?** (predict:
   **yes** — proving the `FIRST_ROUTE_MAP` guard, not just the routine)

Findings land here.

## 4. `fly.json` IS WRONG — 6 of the 11 towns, and v1 shipped it

**Found while looking for a name to put on a checkbox.** `projects/db/assets/data/fly.json` is the
list the town bits have always been drawn through, and its order matches **neither** the map ids
**nor** `ExternalMapEntries`:

| Bit | `fly.json` says | **The cartridge says** | |
|---:|---|---|---|
| 0 | Pallet Town | Pallet Town | ✅ |
| 1 | Viridian City | Viridian City | ✅ |
| 2 | Pewter City | Pewter City | ✅ |
| 3 | Cerulean City | Cerulean City | ✅ |
| **4** | **Vermilion City** | **Lavender Town** | ❌ |
| **5** | **Lavender Town** | **Vermilion City** | ❌ |
| 6 | Celadon City | Celadon City | ✅ |
| **7** | **Saffron City** | **Fuchsia City** | ❌ |
| **8** | **Fuchsia City** | **Cinnabar Island** | ❌ |
| **9** | **Cinnabar Island** | **Indigo Plateau** | ❌ |
| **10** | **Indigo Plateau** | **Saffron City** | ❌ |

Two separate faults: **4 and 5 are swapped** (Lavender/Vermilion), and **7–10 are rotated** (Saffron
hoisted to 7, shoving Fuchsia, Cinnabar and Indigo each down one).

**It shipped in v1, and here is the code that did it** —
`src/app/screens/world-towns/world-towns.component.ts`:

```ts
get towns() { return this.gd.file("fly").data; }              // iterate fly.json, in fly.json's order
getTown(index: number)  { return ...world.towns.visitedTowns[index]; }   // index = the SAVE BIT
```

The template walked `fly.json` and handed each entry's **list position** to `visitedTowns[]`. So in
v1, **ticking "Vermilion City" set Lavender Town's bit**, and ticking "Saffron City" set Fuchsia's.
Six of eleven checkboxes did the wrong thing, quietly, correctly-looking.

**In v2 the bug is latent, not live** — nothing displays the town flags yet, and
`FlyDBEntry::deepLink()` resolves `toMap` **by name** (`MapsDB::inst()->getIndAt(name)`), which is
right and therefore unaffected. It is only `FlyDBEntry::ind` — the field a bit-index consumer would
reach for — that is wrong. **We were about to build exactly that consumer.**

> **The lesson, and it is the v1-carryover lesson wearing new clothes:** *v1 is a data-rich
> reference, not an oracle.* Its data files came over wholesale, and this one has been wrong since
> 2018 without a single test noticing — because **nothing in the suite ever asserted `fly.json`'s
> `ind` against the game.** A list of the right eleven names in the wrong order is the hardest kind
> of wrong to see: every name is real, every name is a town, the count is exact.
>
> **The generalisation:** an index that means "a position in the save" must be **checked against the
> save's own source**, not merely be plausible. Same shape as the hidden-item `(x, y)` transposition
> trap — which was avoided *only* because someone checked.

### What to do about it — leadership's call, not mine

Per the standing rule (*ask before changing data in any `.json`; fix the code that uses the data
instead*), **`fly.json` is untouched.** Two honest routes, and they are not equivalent:

1. **Fix `fly.json`'s `ind`** to the map ids. Correct at the source; but `ind` is also a store index,
   so anything reading it positionally must be re-checked.
2. **Don't use `fly.json` for this at all.** The town bits are map ids, so the name comes from
   **`MapsDB` at map id `i`** — provably right, needs no data change, and removes a redundant source
   of truth. `fly.json` then keeps whatever job it actually has.

**Route 2 is what the Visited UI is built on** — it cannot be wrong, because it asks the same
question the console asks (`wCurMap`). Route 1 is still owed as a **data decision**, because a wrong
`ind` sitting in a shipped DB is a landmine for the next consumer.

Also spotted, and **not** fixed (it is not this feature, and it is hers to rule on):
**`WorldTowns::randomize()`'s comments contradict its code.** It says *"except for Indigo and
Saffron"*, then `for(i = 1; i < 10; i++)` — which **includes Indigo (9)** — and skips `i == 7`
labelled *"Not saffron"*, but **7 is Fuchsia**; Saffron is 10, excluded only by the loop bound. So
it actually excludes **Fuchsia and Saffron** and randomises **Indigo**. Written against
`fly.json`'s order, which is exactly where that order leads.

## 5. The UI — leadership's "alike" group

> *"You can group the visited data in a new kind of group one that groups alike things instead of
> shared data between maps. Alike things that are grouped can allow the whole group to be viewed
> with a click. In this case a click can allow you to view all of the towns together which also
> allows you to do another thing groups can do check and uncheck all."*

A **second kind of group**, orthogonal to the one already shipped:

| Group kind | What it means | Example |
|---|---|---|
| **Shared** *(exists)* | **One save bit** appearing on **several map pages** — the same bit, many homes | Silph Co's flags, on all 12 floors |
| **Alike** *(new)* | **Different save bits** that are **the same kind of thing**, one per place | The 11 towns' visited bits |

**What "alike" affords, and neither is cosmetic:**

- **View the whole group with one click** — all 11 towns together on one view, instead of hopping
  eleven map pages to see who's flyable.
- **Check all / uncheck all** — which is the actual job: "unlock Fly everywhere" is one gesture, not
  eleven.

**v1 had this half-built and it corroborates her memory** — `world-towns.component.ts` carries a
`toggleAllTowns()` (mirroring town 0's state onto all of them). The idea is not new; the *group* is.

**Placement:** the per-town **Visited** checkbox goes **near the top** of that town's Map Storage
page — above the map script, since "have I even been here" precedes anything the map remembers.

⚠️ **Only 11 of ~141 storage pages take one.** The checkbox appears **only** on the eleven city
maps — *"for places that take a visited one"*. A route or a building has no bit and must not grow a
dead switch; clutter is a bug.

**The alike group is deliberately built as a general mechanism, not a towns feature** — *"We
should do this for other data but i cant think of any now."* **Candidate second member: the ten
in-game trades** ([`in-game-trades.md`](in-game-trades.md)) — different bits, same kind of thing, one
per place, and they would want exactly the same "show me all ten" and "clear them all" affordances.
**Offered, not assumed** — un-briefed, per the standing rule.

## 6. Traps, collected

1. **The current town re-marks itself on Continue** (§3) — the linchpin runs *after* it.
2. **…but only if the save is OUTDOORS in that town** (§3) — `cp FIRST_ROUTE_MAP` gates it.
3. **`fly.json`'s order is wrong for 6 of 11** (§4) — and v1 shipped the mislabelled checkboxes.
4. **The bit is the map id, not a list position** (§2) — the whole class of bug above comes from
   forgetting this once, in 2018.
5. **`randomize()`'s comment lies about which towns it skips** (§4).
6. **Only Fly reads these bits** (§1) — a "visited" bit is not story progress and does not gate
   anything else. Say so, or people will expect it to.

---

**See also:** [`in-game-trades.md`](in-game-trades.md) (the other flag array on this panel; the
durable one) · [`warps.md`](warps.md) (the `BIT_NO_PREVIOUS_MAP` linchpin this flag escapes) ·
[`map-storage-locations.md`](map-storage-locations.md) · [`gym-safari-state.md`](gym-safari-state.md)
(`wSafariSteps`, the neighbour that anchors the address).
