# The Events screen — plan of record

Briefed by project leadership, 2026-07-15. The save holds **2,560 world-event flags** (`wEventFlags`); v1 had an
**Events** page listing all of them, but most were cryptic (`1cc`, `1cd`, …) with no name, description,
or map. This is the plan to research **every one of the 2,560** and build a proper editor for them.

Research foundation: [`../reference/event-flags.md`](../reference/event-flags.md).

## The brief (project leadership's words, captured)

- **All 2,560 get the full treatment.** Every flag — named, unnamed, unused, temporary, glitchy —
  gets a **proper name, a description, and a map** to associate it to. *"I don't care if it's unused,
  treat them proper."* No shortcuts, no "deep attention only where it pays".
- **Map association is default.** Fit each flag into a **map** unless it really doesn't make sense; if
  not, a well-named **general group**. If a general group gets too big, **split it** into multiple
  non-map groups.
- **Classify every flag:** unused, duplicate, multi-purpose, temporary, and/or crashing/unstable.
- **Flag groups.** Many flags are toggled **in groups**, and **multiple groups can toggle the same
  flag**. Group them under maps with descriptive names, and provide **group toggles**.
- **Crash safety.** Flags or combinations that can crash/softlock/glitch must **show the user a
  notice** — at the panel, and (case by case, where it fits) **on the toggle/checkbox itself**.
- **Exhaustive research is mandatory** — possibly deep per flag, checking there aren't more than 2,560.
  And the other map-screen work (persistent-storage panel, etc.) is **still mandatory afterward** — no
  slacking once the event research is done.
- **Naming fallback:** truly-undiscoverable flags may be named **"Unknown #<hex id>"**, but only after
  exhausting every file **including the raw assembly**, and only with **explicit leadership sign-off**
  (one or a group at a time). See the naming rules in the reference note.
- **Data home (project leadership's preference):** ideally everything in **one event-flags JSON**; if that can't
  work, its own JSON **linked** to the flag map. Settled in Phase 7.

Order (confirmed): **this feature first**, then the persistent-storage map panel.

## Ground truth (already verified — 2026-07-15)

`wEventFlags` = **2,560 bits = 320 bytes**, WRAM `0xD747`–`0xD886`, save-file **`0x29F3`–`0x2B32`**,
ending right before `wGrassRate`. pret names **507**; **2,053** are gaps. Confirmed there are **not
more than 2,560** — the field is `flag_array NUM_EVENTS`, `NUM_EVENTS = $A00 = 2560`, fixed by the ROM.
Full detail + per-region coverage table: the reference note.

## The programme (phases — each finished before the next, per project discipline)

**Phase 0 — Foundation.** ✅ Plan doc + research note + verified anchors; task list; wiring into the
notes system.

**Phase 1 — Canonical bit-map.** ✅ `scripts/import_event_flags.py` → all 2,560 rows (index, byte, bit,
pret name, owning map-region), self-validating to exactly 2,560, every bit attributed to a region.
Output: `tmp/event-flags/event_flags_canonical.{csv,json}`.

**Phase 2 — Usage cross-reference (all 2,560).** ✅ DONE. `scripts/analyze_event_usage.py` scans the
whole tree by `EVENT_*` name, by **range macro** (`Set/ResetEventRange`, resolving boundary constants),
and by raw `wEventFlags + N`. Result: **531 used, 2,029 unused**, 6 defined-but-unused, 70 temporary,
86 multi-map; 9 range ops sweeping 83 bits (**30 gaps revealed as block-swept**, not unused). This is
the exhaustive negative search the naming rule requires before any "Unknown". Detail in the reference
note.

**Phase 3 — Flag groups.** 🟡 First pass. `scripts/generate_event_dossiers.py` derives ~104 groups from
name patterns, **plus 9 range-defined groups** — per leadership, an event **range is itself a flag
group** (its bits are literally toggled together): the Indigo Plateau Elite-Four reset (40 bits) and the
seven gym-trainer sweeps. Range groups are preferred (evidence-based) and give the block-swept gaps a
real group identity + a natural group-toggle. Still to do: shared-flag detection, the missable/gift/
static-Pokémon gates from v1's `eventPokemon.json`, and editorial group names.

**Phase 4 — Crash / instability analysis.** 🟡 Research done; **two claims corrected by the console
(2026-07-16)**. **Mechanism (stands):** flag bits aren't executed, but event flags drive `wCur<Map>Script`
(set from `CheckEvent` chains) dispatched via script-pointer tables ending in `jp hl`, so an impossible
combination *could* resolve a bad pointer. **But:**
- ❌ **"All flags on crashes the game" — UNREPRODUCED.** All 2,560 set: booted healthy, walked into Oak's
  Lab, then **~12 map transitions**, healthy throughout. Not a proof of "never", but it may **not** be
  asserted as fact. **Owed:** the real repro (v1? a specific map/cutscene? a battle?) — now a ~5 s probe.
- ❌ **The Route 22 rival conflict — REFUTED** (ordered if/else masks the 2nd flag). See Phase 11.

So: keep a **bulk-set warning** (the mechanism is real and it's an unreachable state), but word it as
*"unpredictable and unverified"*, **never "this crashes"**. Individual sensible edits are safe. The
softlock/progression categories (key-item gots, missable objects, one-way blockers, range sequences) stand
as **suspected** until adjudicated. Full writeup in the reference note. ⏳ Owed: adjudicate the remaining
5 same-object governors + `route22-rival-armed-but-hidden` on the console.

**Phase 5 — Author dossiers for ALL 2,560.** 🟡 First complete pass done. The generator emits
`tmp/event-flags/events_dossiers.json` — **every** bit with friendly name, description, map, group,
classification, and evidence. Taxonomy (leadership-defined): **507 named** (incl. 4 hand-researched from
pret's `; ???`) **+ 30 block-swept** range-group members **+ 2,023 "Placeholder Flag #<hex>"** (no code
presence at all; tied to map, own "Placeholder Flags" group at the bottom — no sign-off needed). A
`CURATED` overrides dict in the generator is the hand-authoring layer (seeded with the 4). Owed:
editorial polish of the named descriptions; the Phase 4 `crash` field; extend `CURATED` as deeper
authoring proceeds.

**Phase 6 — Save-model verification.** ✅ DONE (2026-07-16) — **and it found a real bug.** The model
exists (`WorldEvents` + `EventsDB`/`events.json`, data-driven per-event `byte`+`bit`).
- ✅ **Byte/bit maths flawless:** all 508 satisfy `byte == 0x29F3 + ind/8`, `bit == ind%8` — **0
  mismatches**, in range, no dupes. `ind` **is** pret's index. **No corruption bug.**
- 🐞 **14 MISLABELLED + 14 phantom entries, all in the Pokémon Tower block** (`0x0EE`–`0x113`, shifted
  ~2 bits): the editor **writes the wrong flag** — *"In Purified Zone"* → `EVENT_BEAT_POKEMONTOWER_5_TRAINER_3`;
  *"Beat Pokemontower 7 Trainer 0"* → **`EVENT_BEAT_GHOST_MAROWAK`**. The phantoms point at placeholder
  bits pret never names, so they do **nothing**. Table + detail in the reference note.
- 📝 The `worldevents.h` comment (*"scattered all over the save"*) is misleading — it is one contiguous
  320-byte array. Truth-in-labelling fix.
- ⏳ Owed: console-verify an event edit persists on Continue (a ~5 s MCP probe); pin with a test
  (`byte==0x29F3+ind/8` for every entry + a byte-diff proving one toggle moves exactly one bit).

**Phase 6b — fix the model (BEFORE any UI).** Per the sprite/tileset/warp precedent the data is fixed
first: **regenerate `events.json` from pret** (`import_event_flags.py` already emits the canonical,
self-validating 2,560 rows) rather than hand-patch v1's list — which also lifts coverage **508 → 2,560**.
⚠️ Needs leadership sign-off (no `.json` edits without it).

**Phase 7 — Data home.** Finalize the curated data file — ideally one events JSON (names verbatim from
pret + our curation), else standalone + linked. Baked into `db.qrc`; edits go through project leadership. Write
the importer + self-validation. ⚠️ **Reconcile with what the DB ALREADY has** (found 2026-07-15):
`projects/db/assets/data/` already ships **`events.json`** (v1's 2,560-entry list — the cryptic one this
feature enriches), **`eventPokemon.json`**, **`missables.json`**, **`hiddenItems.json`**,
**`hiddenCoins.json`**, **`gameCorner.json`**; and **`maps.json`** carries a per-sprite **`missable`**
index (the two Route 22 rivals = missable 34/35) + object coords (tiles). So the data home should
**extend/replace `events.json`** and **read the object↔missable links straight from `maps.json`** rather
than reinvent them (Phase 9 cross-checks against `missables.json`/`maps.json`). No edits to these JSONs
without leadership sign-off.

### Phase 8 — THE DESIGN (for leadership review; no QML until approved)

Events are **NOT a separate panel** — they are the **4th section of each map page** in the existing
persistent-storage panel (`MapStoragePanel.qml`, right dock ▣). That panel is already a **per-map view
over global save state**, and every page reads top-to-bottom:

1. the map's **Script** (`world.scripts`) → 2. its **minigame bytes** (`world.local`) → 3. its **Filter
Flags** (`world.missables`) → **4. its EVENT FLAGS (`world.events`) — NEW.**

It fits the panel's existing logic exactly: "everything the save stores about one specific place."
`world.events` is **already resolved** in the panel (line ~84) but today is only *read* to show a linked
flag's ON/off beside a Filter Flag — there is no events section yet.

**Where each flag goes.** `EventDBEntry::deepLink()` already back-links every event into
`MapDBEntry::toEvents`, so **`map.toEvents` is the section's model** — no new plumbing. Flags with a
curated map land on that map's page (529 associations, preserved verbatim).

**Structure of the section** (one map page):

- **Search/filter** field — a map can carry dozens; the whole space is 2,560.
- **Groups**, from the dossier `group` (e.g. *"Cerulean Gym — trainers"*, *"Celadon City — items"*,
  *"<Map> — story"*), each a collapsible titled group with a **group toggle** (tri-state: all on / all
  off / mixed). This is leadership's "toggled in groups… have group toggles".
- **A flag row** = a `MapSwitch` + friendly **name** + its researched **description** + **badges** for
  classification (*temporary · unused · block-swept · multi-map · vestigial*), and the per-flag
  **caution** line where the dossier has one (progression / range-sequence / temporary).
- **Placeholder Flags group at the BOTTOM, collapsed by default** — the map's reserved/unused bits
  (`Placeholder Flag #<hex>`). Noise for normal use, but still **fully editable**: every value the save
  can hold stays reachable (hack values included, shown never rewritten).
- **NO conflict UI** — Phase 11 is shelved; nothing about conflicts renders.
- **Bulk caution** on a group toggle that sets a large swath: *"puts the save in a state no real
  playthrough can reach — unpredictable and unverified."* **Never "this crashes"** (the console did not
  reproduce it; Phase 4).

**The un-mapped 2,031** (empty `maps[]`, overwhelmingly placeholders) get a **General** page in the map
combo — and since leadership said *"if the general group gets too big, break it into multiple non-map
groups"*, **it is too big**, so General is **split by the flag's pret region** (Silph Co., Mt. Moon,
S.S. Anne, …) as its groups. Those regions are allocation blocks, not maps — which is exactly why they
belong here rather than being forced onto a map page.

**Open for leadership:** (a) does the events section sit *below* Filter Flags, or above? (b) should the
General page be one page with region groups, or one page per region? (c) default state of the groups —
all collapsed, or story-groups open and Placeholders collapsed (my lean)?

### (Superseded wording, kept for context)

The design is an **expansion of persistent storage**: add the event-flag content
(per-map grouping, flag groups + group toggles, search/filter, classification badges — with a
**Placeholder Flags** group at the bottom of each map — and the per-flag softlock cautions).
**No conflict UI** — the conflicting-flags system is **shelved** (Phase 11); nothing about conflicts
renders here. **Bulk-set caution:** an all-on/large-group set puts the save in a state no playthrough can
reach, so warn before it and don't offer a naive global "set everything" — but word it *"unpredictable and
unverified"*, **never "this crashes"** (the console did not reproduce a crash; see Phase 4).
Design in this doc first; no build until approved.

**Phase 9 — Flag ↔ map location research (new, briefed 2026-07-15).** 🟡 In progress.
`scripts/extract_flag_locations.py` → `tmp/event-flags/flag_locations.json`.
- ✅ **Object inventory DONE (exact):** all **223 maps, 918 objects** — each with tile (X, Y), sprite,
  movement, and kind (**334 trainers, 106 item balls, 478 NPCs**). Oak's Lab confirmed: the 3 starter
  Poké Balls + **Oak at (5,2) and (5,10)**. This is the coordinate foundation the hotspots need.
- ✅ **Conditional-object + flag-gating DONE.** Resolved the **missable-object toggle system**
  (`toggle_constants.asm` ‖ `toggleable_objects.asm`, keyed by object const): **226 conditional/missable
  objects** marked with their `TOGGLE_*` + default ON/OFF. The gating flag is read from the script's
  `CheckEvent`-before-toggle: **14 high-confidence flag↔object links**, all verified — Oak's Lab OAK2 ⇒
  `EVENT_OAK_APPEARED_IN_PALLET`, Museum Old Amber ⇒ `GOT_OLD_AMBER`, Mt. Moon Dome Fossil ⇒
  `GOT_DOME_FOSSIL`, Route 12/16 Snorlax, Viridian Giovanni, Blue's House Town Map. Item balls attach to
  their got-item flag. **Oak's Lab fully correct:** 3 starter balls + Oak at (5,2)/(5,10) (the second
  carrying its flag), aides non-conditional.
- 🔜 Remaining Phase-9 polish: extend gating-flag capture beyond the direct `CheckEvent`-before-toggle
  cases (some toggles are script-state driven), hidden-items and trigger-tile/grass events, and the
  final `maps.json`-style shape for the app. The **inventory + conditional + core linkage** are done and
  correct — enough to build the Phase 10 hotspots on.

**Phase 10 — On-canvas flag hotspots (UI, needs Phase 9).** Draw a **clickable box on the map** at each
flag's location; clicking opens the persistent-storage / events panel focused on that flag. The box is
shown **even when the sprite isn't currently present** (Oak's Lab: the three starter Poké Balls; Oak at
(5,2) and (5,10)). A **layer** in the map screen's layer tree, toggleable. Design in this doc first.
This is where the Events feature meets the map-storage panel the leadership originally queued.

- **Outline colour is event-flag-coded (leadership, 2026-07-15).** Each sprite / object / item gets its
  **own outline**; an object **tied to 1+ event flags** gets a **different colour** to signal it is
  flag-governed (not a plain sprite/item). So Phase 9's extraction must record, per object, the **list of
  attached event flags** (and count), and Phase 10 renders the outline colour from that + focuses the
  panel on those flag(s) on click.

## Phase 11 — The conflicting-flags system — 🛑 SHELVED (leadership, 2026-07-16)

> **DECISION: do not build it.** Leadership: *"maybe we should forgo a conflict system… it's too expensive
> to figure out. It was a nice QoL perk — users not interested in advanced things could do something more
> advanced but still quickly and simply — but it honestly may be too expensive."* Call delegated; **agreed
> and shelved.** The evidence backs it:
>
> - **The founding case was a FALSE POSITIVE.** Route 22 — the most "obvious" conflict in the game (two
>   `SPRITE_BLUE` on one tile, one per battle) — is **refuted**. If the flagship is wrong, the static
>   heuristic's hit-rate is unknown and probably poor.
> - **Only `confirmed` may warn** (the rule the refutation forced). So an unadjudicated system shows the
>   user **nothing** — all cost, no visible value.
> - **Adjudication is expensive per conflict**, and does not amortise. Arming *one* cutscene needed the
>   right script step (`wRoute22CurScript`), missable visibility, and the exact trigger coords
>   (`dbmapcoord 29,4`, y,x) — bespoke research per case, over a 2,560-flag space.
> - The remaining 6 rules are all `suspected`; adjudicating each is a fresh bespoke probe.
>
> **KEPT (cheap, and valuable) — this is knowledge, not a feature:**
> `scripts/analyze_flag_contradictions.py` + `tmp/event-flags/conflicts.json` stay as **research output**;
> the **refutation** and its lesson stay in the reference note; `probe_route22_conflict.py` stays as a
> **no-crash regression**; `route22-rival-armed-but-hidden` stays recorded as an unadjudicated lead. None
> of it ships in the UI. **Nothing about conflicts renders in the panel.**
>
> **If it is ever revived,** the spine is already written below — and rule #1 is the one that matters:
> *suspicion is a lead, never a warning.*
>
> ⚠️ Leadership was explicit that **the sorting, research and grouping remain important for everything
> else** — those are NOT shelved. Only the conflict *feature* is.

### (Design retained for reference — not being built)

A first-class system that flags **combinations** of event flags as conflicting, shows them in the UI, and
grows as conflicts are found/confirmed. Born from the Route 22 rival case (two `SPRITE_BLUE` objects at
the **same tile (25,5)**, `ROUTE22_RIVAL1`/`RIVAL2`, one per battle).

> ❌ **…and that founding case is REFUTED (console + source, 2026-07-16) — which is the best thing that
> could have happened to this system.** `Route22DefaultScript` is an **ordered if/else**: with
> `EVENT_1ST_ROUTE22_RIVAL_BATTLE` set it jumps to the first battle and **never consults the 2ND flag** —
> masked, not conflicting. The cartridge agrees: both flags on + both sprites shown → a **normal trainer
> battle** (`probe_route22_conflict.py`, ~5 s). Full write-up: the reference note.
>
> **Three rules fall out of it, and they are the system's spine:**
> 1. **`suspected` is a LEAD, not a risk.** A same-subject cluster / two objects on one tile is a reason
>    to *look*, never a reason to *warn*. **Only `confirmed` renders as a crash warning in the UI.**
> 2. **`refuted` is kept, not deleted** — negative knowledge, so nobody re-raises a settled question.
>    The static generator now *defers* to verdicts (`REFUTED_SUBJECTS`).
> 3. **Adjudicate on the console.** Every suspicion goes to a forge-probe before it is shown to a user.
>
> 🔎 The probe also surfaced a **real** candidate the static pass never would have: **armed-but-hidden**
> (`route22-rival-armed-but-hidden`, suspected/softlock) — flags arm the ambush while the rival object is
> hidden, so the trigger fires, the script advances to 1, and **no battle engages**. A flag ↔ missable
> inconsistency; owed its own probe.

**A conflict is a logical predicate over a flag set, not just "these two conflict."** Each conflict rule:

- `flags`: the flags involved (2+).
- `condition`: the predicate that *is* the conflict — one of **both-on**, **both-off**, **not-both-on**,
  **not-both-off**, **exactly-one**, **at-most-one**, etc. (so a conflict can be "these must not both be
  on", or "these must not both be off", or "exactly one must be on", …).
- `status`: **suspected** (a *lead* from static analysis — same object/tile/script, 1ST/2ND, FIGHT/BEAT;
  **never rendered as a warning**), **confirmed** (reproduced on the real cartridge via a forged save —
  see the emulator-verification standing method; **only this warns the user**), or **refuted** (the
  console/source disproved it — **kept as negative knowledge** so it is never re-raised; the static
  generator defers to it).
- `severity`: crash / softlock / cosmetic.
- `reason`, `map`, and `evidence` (the probe + result when confirmed).

**Data:** a curated `conflicts` dataset (seeded by `analyze_flag_contradictions.py`, promoted to
confirmed by forge-probes). Lives with the events data home (Phase 7).

**UI (mandatory, leadership):** conflicts are shown **in two places** — (1) a **panel at the top** of the
persistent-storage/events UI listing every **active** conflict (its condition currently violated by the
save), and (2) **on the flags themselves** — each flag in an active conflict wears an inline
indicator/badge (colour by severity; suspected vs confirmed distinguished). Toggling a flag re-evaluates
conflicts live. This is separate from, and on top of, the **bulk-set crash guard** (Phase 4).

### ✅ The founding case — Route 22 rival — is CONSOLE-ADJUDICATED: REFUTED (2026-07-16)

The archetype that *started* this whole system was tested on the cartridge the moment the forge could
arm it (scripts + missable filter flags, 2026-07-16), and it came back **not a conflict**. Armed with
both battle flags on and both `SPRITE_BLUE` shown, standing on the (29,4) coord trigger, the ambush
fires and a **normal trainer battle engages** — no crash, no softlock, sane through 960+ frames
(`scripts/emu/probe_route22_conflict.py`; the full mechanics + the forge-vs-cutscene boundary are in
[`../reference/forged-saves.md`](../reference/forged-saves.md)). The reason is in the script itself:
`Route22DefaultScript` checks `EVENT_1ST` **before** `EVENT_2ND` — an ordered if/else, so the second
flag is simply never consulted while the first is set; the stacked sprites only overlap visually.
`tmp/event-flags/conflicts.json` marks it `refuted / severity none` with the evidence.

**Two things this settles for the system:**

1. The **suspected → confirmed/refuted** pipeline works end-to-end, and **`refuted` is a real status**
   the schema must carry (not every static suspicion survives the console). The seed heuristic
   (same-object/same-tile 1ST/2ND ⇒ suspected-crash) **over-fires** on ordered if/else dispatch —
   worth a note in `analyze_flag_contradictions.py` so the panel doesn't cry wolf on every rival.
2. Reproducing it defined **what a forge can drive** (coord triggers, from a booted save) and **what it
   can't** (multi-frame cutscenes need A-mashing + correct spawn geometry, or they stall and a
   settle-only harness misreads "healthy"). Confirming a *crash* conflict will need the same armed-and-
   driven probe shape, not the batch runner.

## Open questions for project leadership (to settle before the phases that need them)

- **Data home** (Phase 7): one combined events JSON vs. standalone-linked — your lean is "one file if
  it can work". Decided when the schema is drafted.
- **Where the screen lives**: a dedicated **Events** screen (as in v1) vs. a panel on the **Map**
  screen vs. both. Affects Phase 8 only.
- **"Unknown" sign-off**: expect a batch of genuinely-undiscoverable candidates to bring to you near
  the end of Phase 5, with the evidence that every file (incl. assembly) was checked.
