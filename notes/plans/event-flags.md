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

**Phase 4 — Crash / instability analysis.** Identify individual flags and combinations that crash,
softlock, or glitch. Console-probe (`scripts/emu/`) the load-bearing cases. Produce the per-flag and
per-combo notices (panel-level + on-toggle).

**Phase 5 — Author dossiers for ALL 2,560.** 🟡 First complete pass done. The generator emits
`tmp/event-flags/events_dossiers.json` — **every** bit with friendly name, description, map, group,
classification, and evidence. Taxonomy (leadership-defined): **507 named** (incl. 4 hand-researched from
pret's `; ???`) **+ 30 block-swept** range-group members **+ 2,023 "Placeholder Flag #<hex>"** (no code
presence at all; tied to map, own "Placeholder Flags" group at the bottom — no sign-off needed). A
`CURATED` overrides dict in the generator is the hand-authoring layer (seeded with the 4). Owed:
editorial polish of the named descriptions; the Phase 4 `crash` field; extend `CURATED` as deeper
authoring proceeds.

**Phase 6 — Save-model + console verification.** Confirm v2 reads/writes the 320-byte field byte-exact
(add the model if absent). Console-verify a sample of edits persist on Continue and behave as
documented. Pin with a test.

**Phase 7 — Data home.** Finalize the curated data file — ideally one events JSON (names verbatim from
pret + our curation), else standalone + linked. Baked into `db.qrc`; edits go through project leadership. Write
the importer + self-validation.

**Phase 8 — UI design, then build.** Design the Events panel **in this doc first** (per-map grouping,
flag groups + group toggles, search/filter, classification badges — including a **Placeholder Flags**
group at the bottom of each map — crash notices at panel + toggle level; themed map-screen chassis). No
build until the design is written and approved.

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

## Open questions for project leadership (to settle before the phases that need them)

- **Data home** (Phase 7): one combined events JSON vs. standalone-linked — your lean is "one file if
  it can work". Decided when the schema is drafted.
- **Where the screen lives**: a dedicated **Events** screen (as in v1) vs. a panel on the **Map**
  screen vs. both. Affects Phase 8 only.
- **"Unknown" sign-off**: expect a batch of genuinely-undiscoverable candidates to bring to you near
  the end of Phase 5, with the evidence that every file (incl. assembly) was checked.
