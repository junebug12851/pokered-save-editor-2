# Event flags — `wEventFlags`, the 2,560 world-event bits

The save's big story-state bitfield: **2,560 flags** the game sets and checks to remember what you
have done — doors you opened, trainers you beat, items you took, cutscenes you watched, NPCs that
should now be gone. v1's editor had an **Events** page listing all 2,560, but most were cryptic
(`1cc`, `1cd`, `1ce`) — no name, no description, no map. This note is the research that gives **every
one of the 2,560** a proper name, description, owning map, flag-group membership, and classification.

Plan of record: [`../plans/event-flags.md`](../plans/event-flags.md). This is a **living** note —
it fills in as Phases 2–5 author the dossiers.

## WHAT AN EVENT FLAG *IS* — and why it has no place on the map (Fairy Fox's model, source-verified 2026-07-17)

> *"i think event flags are by scripts for scripts, i think filter flags are different. Filter flags are
> meant for maps, event flags are for the code and maps use them in general i think mainly around
> scripts so perhaps an x/y location for event flags is unnecessary but for scripts in an x/y location
> that change event flags, those flags should be tabs on the script box"*
>
> *"i think filter flags also point to scripts which change event flags"*

**They are right, and this settles a question the flag-box work had been circling.** Checked against
`pret/pokered`; the numbers below are counted, not estimated.

**The taxonomy, as the cartridge actually has it:**

| | **Filter flags** (`wMissableObjectFlags`, `0x2852`) | **Event flags** (`wEventFlags`, `0x29F3`) |
|---|---|---|
| Belong to | **a MAP** — one object on one map | **the CODE** — scripts |
| Have an x/y? | **YES**, inherently: `maps.json` gives every object a tile, and `missable` names its bit | **NO.** A flag is a bit the story reads; nothing about it names a tile |
| Reached from the map by | the object standing there | a **script**, which *may* have a location |

**So "an x/y location for event flags is unnecessary" is correct** — and it explains why
`extract_flag_locations.py` only ever found **14** object↔event links across 223 maps. It was looking
for something that does not exist. There is no flag→tile relation to find; there is a flag→**script**
relation, and only *some* scripts have tiles.

**The worked example they gave — Oak pulling you out of the grass — and what it proves.**

> *"if you walk in the grass and prof oak comes out and pulls you into his lab, im pretty sure what you
> stepped on was a script allowed there by a filter flag and the script likely set a lot of stuff
> including more event flags and filter flags"*

`scripts/PalletTown.asm`, verbatim:

```asm
PalletTownDefaultScript:
    CheckEvent EVENT_FOLLOWED_OAK_INTO_LAB   ; <- an EVENT flag gates the whole thing
    ret nz
    ld a, [wYCoord]
    cp 1 ; is player near north exit?        ; <- the LOCATION test
    ret nz
    ...
    SetEvent EVENT_OAK_APPEARED_IN_PALLET    ; <- writes an EVENT flag
    ld a, SCRIPT_PALLETTOWN_OAK_HEY_WAIT
    ld [wPalletTownCurScript], a             ; <- advances the SCRIPT STEP

PalletTownOakHeyWaitScript:
    ...
    ld a, TOGGLE_PALLET_TOWN_OAK
    ld [wToggleableObjectIndex], a
    predef ShowObject                        ; <- writes a FILTER flag (Oak appears)
```

**They are right about the substance:** you step somewhere, a script fires, and it writes *"a lot of
stuff"* — an event flag, a script step, and a filter flag. All three kinds of storage, from one tile.
That is the case for the tabs, made by the cartridge itself.

**One correction, and it matters for the design:** the script is *not* "allowed there by a filter
flag". It is gated by an **event flag** (`EVENT_FOLLOWED_OAK_INTO_LAB`) plus the coordinate test. The
filter flag is what the script **changes**, not what permits it. Direction reversed — the rest holds.

**And it breaks an assumption this plan was about to bake in: a script's location is not always a
TILE.** `cp 1` on `wYCoord` is not a square — it is **the entire north row of Pallet Town**. So a
"script box" cannot be assumed to be one 16×16 outline; some are lines, regions, or conditions that no
extractor can reduce to a coordinate at all. Counted:

| How a script finds out where you are | Files | Shape |
|---|---|---|
| `dbmapcoord` table + `ArePlayerCoordsInArray` | **41** | real **tiles** — a box works |
| **raw** `ld a, [wYCoord]` / `[wXCoord]` + `cp` | **17** (incl. **PalletTown**, **OaksLab**) | a **row/column/region** — a box does **not** work |
| both | 5 | — |

**53 distinct script files have a location of some kind.**

**✅ RESOLVED — leadership, 2026-07-17: _"if its a coord range test then put a box around the whole
range"._** So a script box is **the extent of its trigger**, not a tile:

- a `dbmapcoord` tile → a 1×1 box (as today);
- **`wYCoord == 1` → a box around the WHOLE ROW** — Pallet Town's north edge, all of it;
- a range (`cp` low / `cp` high, or several coords) → a box around the range.

Which is not a compromise — it is more truthful than a tile would be. The trigger *is* the whole row;
drawing one square on it would misrepresent where the game is actually watching. It also means the box
geometry must carry **width/height in tiles**, not the fixed 16×16 the filter-flag boxes use.

⚠️ Still true, and the honest floor: some of the 17 test coords in ways no extractor can reduce to a
range (nested conditions, computed values, checks split across routines). Where the extent cannot be
established **from the source**, the script gets **no box** — never a guessed one.

**The two ways a script gets a location — both real, both counted:**

1. **A script triggered by standing somewhere.** `ArePlayerCoordsInArray` against a `dbmapcoord` table —
   the trigger tiles. **41 script files** have them. e.g. `scripts/AgathasRoom.asm`:
   ```asm
   AgathasRoomDefaultScript:
       ld hl, AgathaEntranceCoords
       call ArePlayerCoordsInArray     ; <- triggers when the player stands on it
   ...
   AgathaEntranceCoords:
       dbmapcoord  4, 10               ; <- the x/y. THIS is a script's location.
   ```
   These are the "script boxes" they mean: a tile that runs code, and the event flags that code writes
   are what belongs on its tabs.

2. **A script reached through a filter-flag object** — their second point, and the mechanism is exact.
   A script toggles an object's filter flag *and* writes event flags in the same breath.
   `scripts/BillsHouse.asm`:
   ```asm
       ld a, TOGGLE_BILL_POKEMON        ; the FILTER flag (an object with a tile)
       ld [wToggleableObjectIndex], a
       predef HideObject
       SetEvent EVENT_BILL_SAID_USE_CELL_SEPARATOR   ; the EVENT flag, next line
   ```
   So an object's tile → its filter flag → the script that toggles it → **that script's event flags**.
   That is a real chain from a map tile to an event flag, and it is the only honest one we have.

**Counted, so nobody plans against a fantasy:**

| Thing | Count |
|---|---|
| `predef Show/HideObject` sites (filter-flag writes) in scripts | **71** |
| `SetEvent`/`ResetEvent` sites in scripts | **117** |
| Script files carrying **both** a toggle and an event write | **22** of 224 |
| Script files with coord-triggered scripts (`ArePlayerCoordsInArray`/`dbmapcoord`) | **41** |

**What this means for the UI** (feeds `../plans/map-screen.md` → Phase 16):

- The **flag box** on an object is a **filter-flag** thing, and that is complete and correct as built.
- **Event flags do NOT get boxes of their own.** They are not on the map; giving them a tile would be
  inventing a fact.
- A **script at a tile** (the 41) can have a box, and *its* event flags are its tabs.
- An object whose script also writes event flags (the 22) can carry those flags as **tabs** — reached
  through the script, not asserted as the flag's own location.

⚠️ **The trap this kills:** "the flag is near the object in the source, so it belongs to the object" is
the same static-co-location reasoning that produced the Route 22 false positive and got the conflicts
system shelved. The chain above is *mechanical* (a toggle and a SetEvent in one routine), not
proximity — but the routine boundary still has to be respected, and anything ambiguous gets **no tab**.

## The anchor facts (verified against `pret/pokered`, 2026-07-15)

| Fact | Value | How verified |
|---|---|---|
| Symbol | `wEventFlags` | `ram/wram.asm`: `wEventFlags:: flag_array NUM_EVENTS` |
| Count | **2,560** flags | `constants/event_constants.asm` final `const_next $A00`; `DEF NUM_EVENTS EQU const_value` → `$A00` = 2560 |
| Size | **320 bytes** | 2560 bits ÷ 8 |
| WRAM | **`0xD747`–`0xD886`** | `wGrassRate` is 320 bytes later at `0xD887`; back-solve |
| Save-file offset | **`0x29F3`–`0x2B32`** | WRAM − `0xAD54` (v2 mapping); cross-checked: `wGrassRate` file `0x2B33` = `0xD887−0xAD54` ✓ |
| Bit order | bit `i` = byte `i÷8`, bit `i%8` | rgbds `flag_array` is little-bit-first |
| Immediately after | `wGrassRate` (wild-encounter table) | the event block ends exactly where [`wild-encounters.md`](wild-encounters.md) begins |

So the whole field sits at **file `0x29F3`, 320 bytes**, ending one byte before the wild-encounter
grass rate. That is a clean, self-consistent anchor — if a future model reads events at a different
offset, it is wrong; read this first.

## What pret already gives us — and what it doesn't

`constants/event_constants.asm` names **507** of the 2,560 via `const` / `const_skip` / `const_next`,
and groups them with `; <City> events` comment blocks. So ~507 arrive with a **real name and an
implied map** for free — v1 simply never absorbed pret's naming, which is why they read as hex.

pret gives us **no descriptions**, no explicit map field, no flag-group data, and no
crash/duplicate/temporary classification. **All of that is ours to research and create.**

The remaining **2,053** bits are gaps pret deliberately skips (`const_skip` inside a city block,
`const_next` jumps between city blocks). Most are genuinely **unused** in the shipped game; a minority
may be **used-but-unnamed** (scripts poking a raw bit). Distinguishing those is a disassembly-wide
cross-reference (Phase 2), never a guess. Per leadership's mandate, **every one of the 2,053 still gets
the full treatment** — a proper name, description, map, group, and classification — not a bulk
"unknown".

## The canonical bit-map (Phase 1 — DONE)

`scripts/import_event_flags.py` parses `event_constants.asm` into the ground-truth skeleton every
dossier hangs on: for all 2,560 bits — `index` (0–2559 = pret `const_value`), `byte_wram`,
`byte_file`, `bit`, pret `name` (or blank), and `section` (owning map-region). It is **self-validating**
(the walk must total exactly `$A00` and every named event must land on pret's assigned index) and
attributes **every** bit — named and gap — to the map-region it falls in (pret allocates each city a
contiguous block via `const_next`; an index belongs to the last block base ≤ it).

Output (research intermediate, git-ignored): `tmp/event-flags/event_flags_canonical.{csv,json}`.

**Coverage: 2,560 total = 507 named + 2,053 gap, across 46 map-regions.** Every gap now has a home
city/route (no orphans). Per-region named/gap counts (from the importer):

| Region | named | gap | | Region | named | gap |
|---|--:|--:|---|---|--:|--:|
| Pallet Town | 14 | 26 | | Route 12 | 10 | 6 |
| Viridian City | 14 | 50 | | Route 13 | 10 | 6 |
| Pewter City | 5 | 43 | | Route 14 | 10 | 6 |
| Cerulean City | 7 | 81 | | Route 15 | 11 | 5 |
| Lavender Town | 22 | 74 | | Route 16 | 10 | 6 |
| Vermilion City | 10 | 38 | | Route 17 | 10 | 6 |
| Celadon City | 21 | 163 | | Route 18 | 3 | 13 |
| Fuchsia City | 12 | 52 | | Route 19 | 10 | 6 |
| Cinnabar Island | 23 | 177 | | Route 20 | 13 | 3 |
| Saffron City | 20 | 108 | | Route 21 | 9 | 7 |
| Route 1 | 1 | 23 | | Route 22 | 5 | 11 |
| Route 2 | 1 | 7 | | Route 23 | 15 | 1 |
| Route 3 | 8 | 8 | | Route 24 | 9 | 7 |
| Route 4 | 2 | 30 | | Route 25 | 15 | 1 |
| Route 6 | 6 | 26 | | Viridian Forest | 3 | 13 |
| Route 8 | 9 | 7 | | Mt. Moon | 14 | 66 |
| Route 9 | 9 | 7 | | S.S. Anne | 22 | 138 |
| Route 10 | 22 | 10 | | Victory Road 3F | 6 | 10 |
| Route 11 | 11 | 5 | | Rocket Hideout | 16 | 112 |
| | | | | Silph Co. | 54 | 218 |
| | | | | Pokémon Mansion | 5 | 123 |
| | | | | Safari Zone | 1 | 63 |
| | | | | Cerulean Cave | 1 | 31 |
| | | | | Indigo Plateau | 10 | 38 |
| | | | | Victory Road 1F | 3 | 157 |
| | | | | Rock Tunnel | 8 | 8 |
| | | | | Seafoam Islands | 7 | 57 |

Note the map-regions are **pret's event allocation blocks**, not the game's map ids — e.g. "Victory
Road 1F/3F" are separate event blocks, and some general/miscellaneous flags will need a **non-map
group** (Phase 5 will carve those out and split any general group that grows too big, per the brief).

## Naming rules (hard rule — project leadership, 2026-07-15)

Every flag gets a **real, discerned name** wherever one can possibly be found, by this precedence:

1. **pret's `EVENT_*` name**, rendered friendly (`EVENT_BEAT_BROCK` → "Beat Brock").
2. **pret placeholder (`; ???`) but referenced in code** → **research it from its usage** (set/check/
   reset sites, surrounding routine) and give it a real name. pret itself hex-names four such events
   (`EVENT_1B8/1BF/67F` = Celadon's dead reset flags; `EVENT_2A7` = "on the Cinnabar Gym map"); all four
   are discernible and named in `CURATED` in `generate_event_dossiers.py`.
3. **No presence in code AT ALL** — not by name, not set/checked/reset, not swept by any range → this is
   a **"Placeholder Flag #<hex>"** (e.g. `Placeholder Flag #1CC`). It is **tied to its map** and filed in
   that map's own **"Placeholder Flags" group at the bottom**. These are the byte-alignment padding /
   reserved bits (the game rounds each map's block to whole bytes and leaves headroom). Because leadership
   defined this name directly, placeholders need **no sign-off** — but the "no code presence at all" test
   is **strict**: any reference whatsoever (even a range sweep) disqualifies a flag from being a
   placeholder.
4. **"Unknown #<hex>" — the true last resort, and gated.** Reserved for a flag that IS referenced in
   code but whose meaning cannot be discerned even after **exhausting every file including the raw
   assembly**. Only then, and **only with explicit project-leadership sign-off** (per-flag or per-group),
   may it be named Unknown. **Currently zero such cases** — every referenced flag has been identified.

**placeholder ≠ used.** A Placeholder Flag has **zero** code presence; a flag that appears in code in any
form is `used` (or block-swept) and gets a researched name, never the placeholder name.

## Classification taxonomy (Phases 2–4 assign these to every bit)

Each of the 2,560 will carry one or more of:

- **used** — the shipped game sets and/or checks it. (sub-note which: set-only, check-only, both)
- **unused** — no references anywhere in the disassembly. Still gets a real name + description
  ("Unused — bit N of byte X; no references in the shipped game").
- **duplicate** — set/checked by more than one place for the same real-world meaning, or shares
  meaning with another flag.
- **multi-purpose** — the same bit is reused for different meanings in different maps/scripts.
- **temporary** — set and cleared within the same script/session (scratch), not durable story state.
- **crash / instability** — setting it (or a combination) causes a crash, softlock, or glitch. Carries
  a **notice** for the UI (panel-level and, where it fits, on the toggle itself).

## Flag groups (Phase 3)

Many flags are toggled **in groups** (all trainers in a gym, all hidden items on a route, a cutscene's
several gates), and **multiple groups can toggle the same flag**. Groups are named descriptively and
filed under their owning map, with a **group toggle**. v1's `eventPokemon.json` (the missable/gift/
static Pokémon gates — Snorlaxes, fossils, Game Corner prize mons, legendaries) is one such cross-map
group to fold in. This section fills during Phase 3.

## Method (how the dossiers get built — Phases 2–5)

1. **Parse** — `import_event_flags.py` gives the skeleton (done).
2. **Cross-reference** — grep the whole `pret/pokered` tree for each `EVENT_*` name **and** raw-bit
   usage; record every set/check/reset site (which map script, which routine). Derives owning map,
   used/unused, temporary, multi-purpose, duplicate.
3. **Groups** — cluster co-toggled flags; note shared toggles.
4. **Crash analysis** — identify individual flags + combinations that crash/softlock/glitch; console-
   probe the load-bearing ones (`scripts/emu/`).
5. **Author** — write name + description + map + group + class for **all 2,560**, chunked by region.

## Results so far — Phases 2, 3, 5 (2026-07-15)

**Phase 2 — usage cross-reference (DONE).** `scripts/analyze_event_usage.py` scans the **whole**
`pret/pokered` tree (every `.asm`) for each flag by `EVENT_*` name, by **range macro**
(`SetEventRange`/`ResetEventRange`, resolving `EVENT_*` and boundary constants like
`INDIGO_PLATEAU_EVENTS_START/END`), and by raw `wEventFlags + N`. Findings:

| Fact | Count |
|---|--:|
| **Used** (has real set/check/reset refs) | **531** |
| **Unused** (no reference anywhere) | **2,029** |
| named-but-**defined-unused** (pret names it, game never touches it) | 6 |
| **temporary** (the game `ResetEvent`s it — scratch, not durable) | 70 |
| **multi-map** (referenced from >1 area) | 86 |
| range ops (`Set/ResetEventRange`) | 9, sweeping **83 bits** |
| **gap** bits revealed as **block-swept** (not truly unused) | **30** |

⚠️ **The range macros were the trap.** `SetEventRange A, B` / `ResetEventRange A, B` **byte-fill the
whole span**, so they sweep **every** bit between the endpoints — including unnamed gaps. The big one:
`HallOfFame.asm` does `ResetEventRange INDIGO_PLATEAU_EVENTS_START, ..._END` — it **clears the entire
Indigo Plateau event block** so the Elite Four can be **re-challenged**, sweeping ~30 otherwise-"unused"
gap bits. Gym `SetEventRange ..._TRAINER_0, ..._6` sweeps mark all a gym's trainers beaten when you beat
the leader. These 30 gaps are classified **block-swept**, not unused.

**Phase 3 — flag groups (first pass).** `scripts/generate_event_dossiers.py` derives **106 groups**
from name patterns (gym trainers, route trainers, items/hidden, boulder/switch puzzles, Team Rocket,
rival, Snorlax, S.S. Anne, per-map story). Largest: Silph Co. trainers (30), Silph Co. story (21),
Lavender/S.S. Anne trainers (16 each). Group toggles + shared-flag detection refine in the UI phase.

**Phase 5 — dossiers for all 2,560 (first complete pass).** The generator emits
`tmp/event-flags/events_dossiers.json` — **every** bit with friendly name, description, map, group,
classification, and the evidence behind it. `0x1CC` (an original mystery) now reads: **"Unknown #1CC,
Celadon City, unused, identity undiscoverable — pending sign-off,"** byte `0x2a1f` bit 4. Named flags
read as full dossiers grounded in their usage.

### The 2,023 no-code-presence bits are Placeholder Flags (named, no sign-off needed)

The exhaustive search **is done** (whole tree, by name + range + raw index). **2,023** gap bits have
**no presence in code at all** → they are **"Placeholder Flag #<hex>"**, each tied to its map and filed
in that map's **Placeholder Flags** group (list: `tmp/event-flags/placeholder_flags.json`). Leadership
defined this name directly, so it is final — no sign-off gate. **Byte-alignment proof:** every per-map
`const_next` base is a multiple of 8 (`$28`, `$68`, `$98`, `$F0`, `$150`, …), i.e. each map's event
block starts on a whole byte, so the tail bits are structural padding — which is *why* they are
unreferenced.

The finalized breakdown of all 2,560: **507 named** (incl. 4 hand-researched from pret's `; ???`) **+
30 block-swept** range-group members **+ 2,023 Placeholder Flags**.

**Owed:** Phase 4 (crash/instability — console-probed; `crash` field is null until then), editorial
polish of the 507 named descriptions, Phase 6 (model verification), Phase 7 (DB data home), Phase 8 (UI).

## Crash / instability / softlock analysis (Phase 4 — research 2026-07-15)

> ⚠️ **Correction (leadership, 2026-07-15):** an earlier draft here claimed "an event flag cannot crash
> the console." **That was too strong** — flags don't execute, but they *drive* executed script pointers
> (mechanism below), so a crash is possible in principle.
>
> **…but the ALL-FLAGS-ON claim did NOT reproduce (console, 2026-07-16). Status: UNREPRODUCED.**
> Leadership reported "turning all flags on crashes the game", and it was recorded here as established.
> The cartridge says otherwise so far. With **every one of the 2,560 flags set** (`emu_boot all_flags`),
> the console: booted healthy in Pallet (888 frames) → walked into **Oak's Lab** (the densest
> `CheckEvent` chain in the game) → then the autopilot walked **~12 map transitions** (Pallet → Route 1
> → Viridian → Route 2 → Pewter → … → the Saffron area) — **healthy overworld throughout, no garbage,
> no hang** (screenshots: `tmp/mcp-shots/`). It only stopped on a *routing* limit ("no path" into Silph),
> not a crash.
>
> **This is not a proof of "never crashes"** — it is a slice: no battle was fought under all-on, and
> ~200 maps and every cutscene remain untested. But the specific, headline claim is **not reproduced**,
> so nothing here may assert it as fact. **Owed:** find the real repro — was it **v1**'s editor? a
> **specific map/cutscene**? a **battle**? Once named, it is a 5-second forge-probe on the new MCP.
> Until then, treat the *mechanism* as real and the *all-on crash* as **unverified**.

**A flag *bit* is never executed as code** (every reference is a `bit n,[hl]` test or a `res`/`set`;
there is no `jp [wEventFlags]`). **But event flags DRIVE executed pointers.** Each map advances its
story by setting `wCur<Map>Script` from a **`CheckEvent` chain**, then **dispatches through a
script-pointer table that ends in `jp hl`** (e.g. `PalletTown.asm` sets `wPalletTownCurScript` off a
stack of `CheckEvent`s; `jp hl` on the resolved pointer is pervasive). So an **impossible flag
combination — above all, *all flags on* — pushes scripts into states and indices that can never occur
in normal play**, resolving a script pointer the game never validates and **`jp hl`-ing into garbage →
crash.** Verified real (leadership) + mechanism grounded in the disassembly.

**Consequences for the editor:**

- **Bulk / mass-set: caution, honestly grounded.** The *mechanism* (an impossible combination driving a
  map script to an unhandled index → executed pointer) is real, so a bulk set is still the riskiest thing
  the panel can offer, and a **warning before any bulk/all/large-group set is retained**. But the warning
  must **not claim "this crashes"** — the console did **not** reproduce it (above). Word it as *"this puts
  the save in a state no real playthrough can reach; behaviour is unpredictable and unverified"* — true,
  and it survives contact with a user who tries it and sees nothing happen. A naive global "set
  everything" still shouldn't be offered.
- **Impossible combinations** (mutually-exclusive story flags both on) are the crash trigger — not any
  single well-formed edit. Individual, sensible edits are almost always safe.
- The **softlock / progression** categories below still apply on top of the crash risk.

**The (non-crash) softlock / progression-break / cosmetic categories:**

- **Progression gates — got-key-item / HM flags** (`GOT_POKEDEX`, `GOT_OAKS_PARCEL`, `GOT_HM01`–`HM05`,
  `GOT_MASTER_BALL`, `GOT_BIKE_VOUCHER`, badges-adjacent): clearing one for something already consumed
  can **strand progress** (the item can't be re-obtained; a gate won't re-open). **Caution.**
- **Missable objects** (the 226 conditional objects, Phase 9): setting a "got/beat" flag **permanently
  hides** its object (you lose that item/battle); clearing re-shows it (can duplicate). Editing these
  changes what's on the map. **Caution.**
- **One-way NPC blockers** (guards, the S.S. Anne/Bill/rival gates): setting/clearing out of order can
  **trap or free** the player unexpectedly. **Caution.**
- **Range-reset sequences** (Elite-Four / Hall-of-Fame clear, Safari): editing individual bits **mid
  sequence** can desync it (block-swept flags). **Caution.**
- **Temporary flags** (70): rewritten during play — an edit **won't stick** as expected. Info, not risk.
- **Everything else** — most flags just change what appears / what an NPC says. **No risk.**

Encoded as a `caution` field on each dossier (`generate_event_dossiers.py`; 127 flags).

**Console test (real cartridge, PyBoy — `scripts/emu/probe_event_flag_crashes.py`, 2026-07-15).** Booted
BaseSAV on Continue with (A) baseline, (B) **all 2,560 flags on**, (C) the Route 22 rival 1ST+2ND+WANTS
set. Result: **all three reach a healthy Pallet Town overworld** and stay healthy 900 frames. So — refining
the finding — **the crash is NOT on the load path**: mass-set flags load fine; the bad `jp hl` only fires
**when the player is on the offending map / triggers the battle** (BaseSAV starts in Pallet, away from the
break). This is consistent with the mechanism and with leadership's report (the crash is seen *in play*).
**Owed:** an input-driven probe that walks the player onto Route 22 and triggers the rival to capture
the crash live (a bigger automation task).

**Contradiction candidates (`scripts/analyze_flag_contradictions.py` → `contradictions.json`).** The
flag *sets* that must not be on together:
- ~~**Same-subject multi-state:** `ROUTE22_RIVAL`~~ — ❌ **REFUTED 2026-07-16 (console + source).** See
  "The Route 22 rival conflict is REFUTED" below. The static heuristic cried wolf; this entry is a
  **false positive** and must not be shown as a crash risk.
- **Same-object governors (one object, >1 flag):** Route 12 & 16 **Snorlax** (`FIGHT_*` + `BEAT_*` — the
  fight-vs-beaten interplay), Cerulean rival, Viridian Giovanni (`BEAT` + `GOT_TM27`), Blue's House Town
  Map (`GOT_POKEDEX` + `GOT_TOWN_MAP`).
These drive the UI's per-combo warnings; the subject-matcher can be broadened (other rival battles) as a
polish pass.

**Forge-onto-any-map PROVEN (2026-07-15).** `probe_route22_conflict.py` + a synthetic save that sets
`wCurMap = ROUTE_22 (0x21)` + player coords + the rival flags, resealed, boots the real ROM **straight
onto Route 22** (verified: `wCurMap 0x21`, on-overworld). The standing forge-a-save method works
end-to-end.

**Coordinate units — RESOLVED from `maps.json` (authoritative, no emulator):** `maps.json` map
`width`/`height` are in **blocks**; object/sprite/sign/warp coords are in **tiles** (= blocks × 2). Proof:
Route 22 is `height: 9` yet has a sign at `y: 11` — impossible unless coords are tiles (0–17). So Route
22 is **40×18 tiles**, the rival at tile **(25, 5) is valid and on-map**, and our Phase 9 object coords
(in tiles) are **correct**. (An emulator read of `wCurMapWidth`=10 mid-boot was stale — disregard it.)
Bonus: `maps.json` already carries **`"missable": 34/35`** on the two Route 22 rivals — the toggle indices
— confirming the missable-object finding and giving Phase 9 the flag↔object link for free.

**Live crash confirmation — blocked by THIS environment, not the logic.** The forge is correct and boots
onto Route 22; driving the player up into the rival should reproduce the crash. But sustained PyBoy runs
here are unreliable (accumulating zombie processes, silent boot stalls, a ~44 s per-call cap). **Resolved
2026-07-16** by the `pokered-dev` MCP server (single-shot, tree-killed, job-based) — the probe now runs in
**~5 seconds**.

### ❌ The Route 22 rival conflict is REFUTED (console + source, 2026-07-16)

The headline suspicion — *both Route 22 rival-battle flags on crashes the battle* — is **wrong**, and this
is the most valuable thing the conflict system has produced so far, because it proves **why suspicion must
be adjudicated before it is shown to a user**.

**The source settles it** (`scripts/Route22.asm`, `Route22DefaultScript`):

```
	CheckEvent EVENT_ROUTE22_RIVAL_WANTS_BATTLE
	ret z                                    ; not armed -> return
	ld hl, .Route22RivalBattleCoords
	call ArePlayerCoordsInArray
	ret nc                                   ; not on a trigger coord -> return
	CheckEvent EVENT_1ST_ROUTE22_RIVAL_BATTLE
	jr nz, Route22FirstRivalBattleScript     ; 1ST set -> FIRST battle, and DONE
	CheckEventReuseA EVENT_2ND_ROUTE22_RIVAL_BATTLE   ; only reached if 1ST is CLEAR
	jp nz, Route22SecondRivalBattleScript
```

It is an **ordered if/else**: with 1ST set the script jumps to the first battle and **never consults 2ND**.
Both flags on cannot collide — the second is simply **masked**. The two stacked `SPRITE_BLUE` objects just
overlap; only one is ever driven.

**The cartridge agrees** (`scripts/emu/probe_route22_conflict.py`, ~5 s):

```
armed-1st    -> HEALTHY trainer battle (mode=2, script=2)
conflict     -> HEALTHY trainer battle (mode=2, script=2)   <- both flags + both sprites shown
ghost        -> NO BATTLE (script=1, ow=True) - stalled/blocked
```

⚠️ **Trigger coords corrected:** the ambush fires from `.Route22RivalBattleCoords` = `dbmapcoord 29,4` /
`29,5` (**`dbmapcoord` stores db y,x**) — **not** the rival's own tile (25,5). The player must stand on the
**upper** trigger (29,4); on (29,5) the rival's rightward walk is blocked and the cutscene softlocks (a bad
forge, not a game bug). Arming it also needs `wRoute22CurScript = DEFAULT` **and** the rival object shown
(missable) — flags alone are not enough.

**A real candidate did fall out of it — `ghost`:** armed (WANTS + 1ST) but the rival object **hidden**
→ the coord trigger fires, the script advances to 1, and **no battle ever engages** (stalled). That is a
genuine **flag ↔ missable inconsistency** (the flags say "ambush armed", the sprite says "nobody here"),
and it is a **suspected softlock** worth its own probe. (It also contradicts the probe docstring's claim
that ghost "engages cleanly" — the run says otherwise; trust the run.)

**The lesson, and it is the point of the whole system:** "two flags for one subject / two objects on one
tile" is **suspicion, not evidence**. Script **dispatch order** can make one flag mask another entirely.
So: `suspected` never renders as a crash warning; only `confirmed` does — and `refuted` entries stay in
the dataset as *negative knowledge* so nobody re-raises them.

## Flag ↔ map location & object association (briefed 2026-07-15)

Project leadership briefed an on-canvas feature: draw a **clickable box on the map** at the location
tied to each flag; clicking it opens the persistent-storage / events panel focused on that flag. The box
shows **even when the sprite isn't currently there** (many objects appear/disappear or move by story
state — e.g. **Oak stands in several different spots**). Worked example, Oak's Lab (`data/maps/objects/
OaksLab.asm` + `scripts/OaksLab.asm`):

- Each map's `def_object_events` gives every object an **(X, Y) tile coordinate** + sprite + its text.
  Oak's Lab: the three **starter Poké Balls** at **(6,3) (7,3) (8,3)** (all tied to the starter-choice
  flags), **Oak at two positions** — **(5,2) DOWN** `TEXT_OAKSLAB_OAK1` and **(5,10) UP**
  `TEXT_OAKSLAB_OAK2` — plus the Pokédex objects, the rival, the aides.
- Objects are shown/hidden by the map script via `predef ShowObject` / `HideObject`, **gated on
  `CheckEvent`** — that is the flag↔object link. So the association is derivable per map by pairing the
  object list (coords) with the script's show/hide-on-event logic.
- **Item balls / hidden items** tie to their "picked up" event; **trigger-tile / grass events** (e.g.
  the offscreen Oak when you first step into the Route 1 grass) have **no clean object coordinate** — they
  attach to the map (or a trigger tile) rather than a sprite, and the box for those is map-level.

The association data (**flag → one or more (map, X, Y) + object/kind**) is a **new extraction** (its own
research phase feeding the UI). Some flags have several boxes (one per Oak spot), some share a box
(a group), some have none on the visible map (story/global). This is Phase 9 in the plan.

**Object structure (per map `<Map>_Object:` block).** `def_warp_events` (doors), `def_bg_events`
(signs), and `def_object_events` — each `object_event X, Y, SPRITE, movement, facing, <text|OPP_*|ITEM>`.
Objects are indexed by `object_const_def` (e.g. `PALLETTOWN_OAK`), which is how the script references
them: `predef ShowObject`/`HideObject <objconst>` guarded by `CheckEvent <flag>` is the **flag↔object**
link. The object's 6th arg tells its **kind**: a plain **text id** (NPC/sign-like), `OPP_<class>`
(**trainer** — has a battle script), or `ITEM` (**item ball** — tied to its got-item flag).

**Object show/hide runs through the missable-object toggle system — a distinct bitfield (verified
2026-07-15).** A map object that appears/disappears is not toggled by `wEventFlags` directly. The chain
is: `constants/toggle_constants.asm` (a **global** `TOGGLE_*` index list — `TOGGLE_PALLET_TOWN_OAK` = 0,
…) → `data/maps/toggleable_objects.asm` (`toggleable_objects_for <MAP>` blocks of
`toggle_object_state <object_id>, ON/OFF`, in the same global order) → the runtime bit lives in
**`wMissableObjectFlags`**, which is **NOT** `wEventFlags`. The map script does `CheckEvent <flag> …
ld a, TOGGLE_… ; ld [wToggleableObjectIndex], a ; predef ShowObject/HideObject` — so an **event flag
gates** the missable toggle, but the visible/hidden state itself is a *missable-object* bit. 33 of the 34
scripts that toggle objects use `TOGGLE_*`. **Consequence for the hotspot:** an object is "conditional"
(appears/disappears) iff it is in the toggle table; the *event flag* that governs it comes from the
script's `CheckEvent`-before-toggle. Resolving `TOGGLE_ → (map, object)` from `toggleable_objects.asm`
(`toggle_object_state <OBJECT_CONST>, ON/OFF`, keyed by the object const) marks conditional objects —
**done** (226 objects). The gating event flag is read from the script's `CheckEvent`-before-toggle
(`extract_flag_locations.py`). (`wMissableObjectFlags` is itself a separate save field worth its own
note if it ever gets an editor.)

**Event-flag attachment (drives the Phase 10 outline colour).** Every sprite/object/item gets its own
outline; an object **tied to one or more event flags** gets a **different colour** to signal it is
flag-governed (not a plain sprite/item). An object is flag-attached when a `CheckEvent <flag>`-guarded
`ShowObject`/`HideObject <objconst>` governs its appearance, when it is an **item ball / hidden item**
(its got-item flag), or when the map's script otherwise sets/checks a flag for that object. Phase 9
records, per object, the **list of attached event flags** (and count) so Phase 10 can colour the outline
and the click can open the panel at those flag(s).

## The v2 save model — verified, and it has a REAL bug (Phase 6, 2026-07-16)

**The model exists** (it is *not* missing): `WorldEvents` (`expanded/world/worldevents.{h,cpp}`) +
`EventsDB` (`projects/db/assets/data/events.json`). It is **data-driven**: `load()`/`save()` walk the DB and
read/write each event at its own **absolute `byte` + `bit`** (`toolset->getBit(event->getByte(), 1,
event->getBit())`).

**✅ The byte/bit maths is FLAWLESS — no corruption.** Every one of the 508 entries satisfies
`byte == 0x29F3 + ind/8` and `bit == ind % 8` — **0 mismatches**, all in range, no duplicates. `ind` **is**
pret's event index, and the offsets agree exactly with the cartridge-verified anchor (`Followed Oak Into
Lab` ind 0 → byte 10739 = `0x29F3` bit 0; `Beat Articuno` ind 2522 → byte 11054 = `0x2B2E` bit 2). Contrast
the tileset `collPtr` disaster: **there is no wrong-byte bug here.**

The header comment (*"these bits have to be gotten all over the place"* / *"scattered all over the
save"*) is **misleading legacy wording** — they are one **contiguous 320-byte array**; the DB merely stores
each bit's absolute address. (Leadership 2026-07-16: the v1 event data *"was always a mess to me… but I
never disputed it was contiguous bytes"* — so this is confused wording, not a disputed model.) Fix the
comment.

### The bug: 14 mislabelled + 14 phantom entries, all in the POKÉMON TOWER block

Comparing all 508 v1 names against pret's names **at the same index**: **480 match, 14 are MISLABELLED,
and 14 more sit on indices pret does not name at all.** They cluster in one place — Pokémon Tower
(`0x0EE`–`0x113`) — where v1's list is **shifted ~2 bits** against the truth:

| index | v2/v1 calls it | the bit REALLY is |
|---|---|---|
| `0x0F1` | Beat Pokemontower 3 Trainer 0 | **`EVENT_BEAT_POKEMON_TOWER_RIVAL`** |
| `0x0F3` | Beat Pokemontower 3 Trainer 2 | `EVENT_BEAT_POKEMONTOWER_3_TRAINER_0` |
| `0x0FB` | Beat Pokemontower 4 Trainer 2 | `EVENT_BEAT_POKEMONTOWER_4_TRAINER_0` |
| `0x104` / `0x105` | Beat Pokemontower 5 Trainer 2 / 3 | `..._5_TRAINER_0` / `..._5_TRAINER_1` |
| `0x107` | **In Purified Zone** | **`EVENT_BEAT_POKEMONTOWER_5_TRAINER_3`** |
| `0x109` | Beat Pokemontower 6 Trainer 0 | **`EVENT_IN_PURIFIED_ZONE`** |
| `0x10B` | Beat Pokemontower 6 Trainer 2 | `EVENT_BEAT_POKEMONTOWER_6_TRAINER_0` |
| `0x111` | Beat Pokemontower 7 Trainer 0 | **`EVENT_BEAT_GHOST_MAROWAK`** |
| `0x113` | Beat Pokemontower 7 Trainer 2 | `EVENT_BEAT_POKEMONTOWER_7_TRAINER_0` |

**Consequence: the editor writes the WRONG FLAG.** Ticking *"In Purified Zone"* marks a Tower trainer
beaten; ticking *"Beat Pokemontower 7 Trainer 0"* sets **`EVENT_BEAT_GHOST_MAROWAK`** (story-critical — the
Marowak ghost). And the **14 phantom** entries (`0x0EE`, `0x0EF`, `0x0F2`, `0x0F9`, `0x0FA`, `0x102`,
`0x103`, `0x10A`, …) point at **placeholder bits pret never names**, so toggling them does **nothing**.
(4 of the 14 "mislabels" are benign style only — `Unnamed 1BF` vs `EVENT_1BF`, same bit; `0x677` is simply
unnamed in v1 but is really `EVENT_ENTERED_ROCKET_HIDEOUT`.)

### ✅ Console-verified: event edits are LIVE on Continue (2026-07-16)

Forged a save with three flags set, booted the real ROM, and read the console's **own WRAM** — all three
survive the load, at exactly the byte/bit the model computes (`wram = 0xD747 + ind/8`, `bit = ind%8`):

| flag | ind | WRAM | console byte | bit | |
|---|--:|---|---|--:|---|
| `EVENT_GOT_TOWN_MAP` | 24 | `0xD74A` | `0x07` = `0000 0111` | 0 | ✅ set |
| `EVENT_BEAT_BROCK` | 119 | `0xD755` | `0xC4` = `1100 0100` | 7 | ✅ set |
| `EVENT_BEAT_GHOST_MAROWAK` | 273 | `0xD769` | `0x8E` = `1000 1110` | 1 | ✅ set |

So the **file `0x29F3` ↔ WRAM `0xD747`** mapping (`wram = file + 0xAD54`) is right on real hardware, and
event flags are **durable** — no `EnterMap` rewrite, unlike the transient `wStatusFlags*` bytes. Note
ind **273 = `0x111`** is the very bit v1 called "Beat Pokemontower 7 Trainer 0": the console confirms it
is `EVENT_BEAT_GHOST_MAROWAK`.

**Pinned by tests** (`tst_world`): `events_everyEntryIsAtItsCanonicalBit` (coverage **2560** + every
entry at `0x29F3 + ind/8`, bit `ind%8`, computed independently of the json) and
`events_writeExactlyTheirBit` (flip one flag → byte-diff the whole 32 KB → **exactly one byte moves**).
⚠️ Neither catches the *mislabel* class (v1's byte/bit were already correct — the NAMES were wrong); that
is guarded by **generation**: `import_events_db.py --check` reports any drift from pret.

**The fix is Phase 7, not a hand-patch:** `ind` is pret's index and `import_event_flags.py` already emits
the canonical, self-validating 2,560-row truth — so **regenerate `events.json` from pret** rather than
nudge v1's hand-made list. That also lifts coverage from **508 → 2,560** (the other 2,052 are unreachable
today). ⚠️ Data change — needs leadership sign-off before writing any `.json`.

## Save-model + UI (Phases 6–8)

- Phase 6: confirm v2 reads/writes the 320-byte field byte-exact (add the model if absent) and
  console-verify a sample of edits persist on Continue.
- Phase 7: the curated data home — ideally **one events JSON** holding names (verbatim from pret) +
  our curation; else a standalone file linked to the flag map. Baked into `db.qrc`.
- Phase 8: the Events UI — designed in the plan doc first, per the map-screen discipline.
