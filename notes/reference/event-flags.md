# Event flags — `wEventFlags`, the 2,560 world-event bits

The save's big story-state bitfield: **2,560 flags** the game sets and checks to remember what you
have done — doors you opened, trainers you beat, items you took, cutscenes you watched, NPCs that
should now be gone. v1's editor had an **Events** page listing all 2,560, but most were cryptic
(`1cc`, `1cd`, `1ce`) — no name, no description, no map. This note is the research that gives **every
one of the 2,560** a proper name, description, owning map, flag-group membership, and classification.

Plan of record: [`../plans/event-flags.md`](../plans/event-flags.md). This is a **living** note —
it fills in as Phases 2–5 author the dossiers.

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
(parallel to `toggle_constants.asm`) is the clean way to mark conditional objects — the next Phase 9 step.
(`wMissableObjectFlags` is itself a separate save field worth its own note if it ever gets an editor.)

**Event-flag attachment (drives the Phase 10 outline colour).** Every sprite/object/item gets its own
outline; an object **tied to one or more event flags** gets a **different colour** to signal it is
flag-governed (not a plain sprite/item). An object is flag-attached when a `CheckEvent <flag>`-guarded
`ShowObject`/`HideObject <objconst>` governs its appearance, when it is an **item ball / hidden item**
(its got-item flag), or when the map's script otherwise sets/checks a flag for that object. Phase 9
records, per object, the **list of attached event flags** (and count) so Phase 10 can colour the outline
and the click can open the panel at those flag(s).

## Save-model + UI (Phases 6–8)

- Phase 6: confirm v2 reads/writes the 320-byte field byte-exact (add the model if absent) and
  console-verify a sample of edits persist on Continue.
- Phase 7: the curated data home — ideally **one events JSON** holding names (verbatim from pret) +
  our curation; else a standalone file linked to the flag map. Baked into `db.qrc`.
- Phase 8: the Events UI — designed in the plan doc first, per the map-screen discipline.
