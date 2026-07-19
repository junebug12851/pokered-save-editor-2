# Map states — the per-map progression blueprints (researched 2026-07-17)

Briefed by Fairy Fox 2026-07-17: every map has one or more *default states* — where the NPCs
stand, which filter flags are on, which event flags are set, what the script byte holds — and the
map's script largely determines its **progress**. This is the research behind the shipped
blueprints in `projects/db/assets/data/map-states/` (98 files + `_index.json`), the generator
`scripts/extract_map_states.py`, and the story layer `scripts/data/map_states_curated.json`.
Plan (incl. the model/UI phases and the "map script" → "map state" rename):
[`plans/map-states.md`](../plans/map-states.md).

## The model — what a "map state" actually is

**A map's `SCRIPT_<MAP>_*` values are not all states of the world.** They split into:

- **Resting values** — what the byte *sits at* between play sessions: `DEFAULT`, `NOOP`, the
  post-cutscene watchers. A real save is (almost) always found in one of these. **147** of the
  game's 381 table values are resting.
- **Transient values** — mid-cutscene steps the engine passes through frame-by-frame (Oak
  walking to the player, a rival marching over). Real, storable, never refused — but loading a
  save inside one resumes mid-cutscene. **234** values.

**And a progression STAGE is more than the byte.** Every gym rests at script 0 both before and
after its leader — the stage lives in the flags and the badge. The blueprint therefore models
stages as **script byte + absolute event-flag set + absolute missable visibility + badges**, with
the byte's own raw table shipped separately (`scriptValues`) for the dropdown.

The one clean counter-example proving the byte matters too: **S.S. Anne 2F's rival battle sets NO
event flag** — the byte parking on `NOOP (4)` is the *only* durable record the encounter leaves.

## The blueprint files

One JSON per scripted map (98 — every `scripts/<Map>.asm` with a `def_script_pointers` table).
Per file: `map`/`mapInd`/`scriptSlot` (the `WorldScripts` byte at `0x289C`+slot), `entry` (the
default landing spot — **the first `warpIn` coordinate**, per the brief), `progression`
(`start`/`end`/`order`/`branches`/`messy`), `scriptValues` (the raw table, each value classified
resting/transient with its outgoing edges), and `states`:

- Resting stages carry ids `"1"`, `"2"`, … with **letters for genuine branches** (`"2a"`/`"2b"` —
  Mt. Moon B2F's fossil choice is the exemplar). Transient cutscene steps ride as `"N.k"`
  (the dots are the cutscene *leaving* stage N).
- Each stage's `save` block is **ABSOLUTE**: the script value, every event set (`owned: false`
  marks context flags another map's story writes), every owned-but-unset event explicitly
  `cleared`, the complete visibility list for the map's own missables, badges. Selecting a state
  from the list = writing this block. **Rolling backward = writing the earlier stage's absolute
  block** (deltas are not invertible once branches exist).
- `delta` on each stage is the roll-forward increment from the previous stage (events set/
  cleared, missables shown/hidden, script from→to, badges gained).

**34 maps are hand-curated** (the story maps: Pallet/Oak's Lab/Blue's House/Viridian/Pewter +
gym, all 8 gyms, Cerulean, Route 22's double ambush, Bill, Vermilion + S.S. Anne 2F, Mt. Moon
B2F, the Pokémon Tower trio, Rocket Hideout B4F, Silph 7F/11F, both Snorlax routes, the four
Elite rooms, Champion's Room, Hall of Fame, Safari gate). The other 64 carry an honest
**auto-derived skeleton**: an initial state plus, where the map's scripts write durable facts, a
"scripted events completed (derived)" state — flagged `derived: true`, never dressed up as
curation.

## What the deep dive found (the traps, each one load-bearing)

- **Script writes hide behind helper setters.** Silph Co (and friends) advance with
  `ld a, SCRIPT_X` / `jp SilphCo7FSetCurScript` — a regex for `ld [w*CurScript], a` alone loses
  whole maps' edges. The extractor runs a register-`a` line simulation per routine and follows
  jumps/calls into any routine that stores the caller's `a` into a CurScript var.
- **Fallthrough carries the effects.** `PewterGymBrockPostBattle` ends without `ret` — every
  world-write (TM34, the badge, the cleanup) lives in `PewterGymScriptReceiveTM34`, the label it
  falls into. Routine bodies must be closed over fallthrough or gyms "do nothing".
- **Three pointer-table routines live in the engine, not the map file:**
  `CheckFightingMapTrainers` (resting), `DisplayEnemyTrainerTextAndStartBattle` and
  `EndTrainerBattle` (transient, hand back to 0). Classification for these is knowledge, not
  parse — their bodies aren't in the file.
- **Text handlers write durable flags.** Bill's S.S. Ticket, Daisy's Town Map, every gym TM —
  set from `text_asm` bodies, not script routines. The owned-flag universe must be file-wide.
- **Resting vs transient is a guard question.** A value that advances only behind a *story*
  guard (CheckEvent, coord tests, talk gating) RESTS there; one that advances behind a pure
  *animation* wait (scripted-NPC movement, walk counters, battle-end polls) is transient.
  Value 0 is always listable (every byte inits to 0); if it auto-plays a cutscene on entry
  (Champion's Room) it wears `autoRunsOnEntry` instead.
- **Cross-map writes are everywhere, and they are the story's connective tissue.** Oak's Lab
  arms Route 22's first ambush; Viridian Gym arms the second; Pewter Gym *disarms* the first if
  skipped; Pokémon Tower 7F populates Mr. Fuji's house and toggles two Saffron townsfolk; the
  Hall of Fame hides Cerulean's cave guard. A stage's `save` block carries the cross-map
  *events*; cross-map *missables* (another map's bits) are `saveNotes` — the blueprint never
  writes another map's visibility silently.
- **The Hall of Fame WIPES the Elite Four on purpose.** Value 2 resets all ten run flags
  (door one-shots, beat flags, `EVENT_BEAT_CHAMPION_RIVAL`, the door lock) so the gauntlet
  re-arms, then saves. A post-game save showing the E4 "unbeaten" is the machine working.
- **E4 exit corridors are DERIVED tiles.** Each room recomputes the block behind the boss from
  the beat flag on entry (`0x24` blocked / `0x05` clear) — nothing to store, everything to not
  store. Entering Lorelei's room also sets `startedElite4` (`0x29E0` b1, see
  [`world-completed.md`](world-completed.md)).
- **The fossil pair is the game's first true branch.** `TOGGLE_MT_MOON_B2F_FOSSIL_1` is the
  **Dome** ball, `_2` the **Helix**; taking one sets its `EVENT_GOT_*_FOSSIL`, the Super Nerd
  battle follows, and value 5 hides the other ball. Either branch ends with both hidden —
  `2a`/`2b` in the blueprint, and the id scheme's reason to exist.
- **18 "maps with scripts" were aliases.** `import_map_scripts.py` counted 116 because pret's
  `map_header_pointers.asm` points unused map ids at real maps' headers — the Unused Map XX
  entries share a real map's script file. 98 real files is the correct universe.
- **`missables.json` map names mix spellings** ("Mt Moon B2F" vs maps.json's "Mt. Moon 3") —
  joins must accept `name` OR `modernName` or the fossil floor silently loses its missables.
- **Oak's street sprite is re-hidden by the ENGINE** (`engine/overworld/auto_movement.asm`),
  not by any script — the only Show of `TOGGLE_PALLET_TOWN_OAK` in scripts/ is never paired
  with a Hide there. A blueprint read only off `scripts/` would leave Oak standing in Pallet
  forever.

## Generation & validation

`python scripts/extract_map_states.py` (`--check` for CI-style drift detection, `--digest` for
the curation fact-dump to `tmp/map-states-digest.json`). It builds on
`import_map_storage_spots.py`'s proven parser (macro grammar, event-macro expansion including
the Range forms, `predef_jump` toggles). Every curated reference must resolve — event pretNames
against `events.json`, toggles against `missables.json` *and* against the map that owns them,
script values against the map's own table, badges against the eight — or generation **fails**;
curation cannot invent a flag. Stage-id uniqueness, advance targets and progression order are
validated across all 98 files.

## Matching is BEST-EFFORT (amended 2026-07-19, leadership; evidence rule same day)

The original model answered "" (custom) for a save matching no stage exactly. **Retired.** The
app always determines a stage, in this order (`mapmodel_states.cpp`, pinned by `tst_map_states`):

1. **A transient the byte names** — a byte parked mid-cutscene is the literal current state.
2. **The LATER of**: the latest resting stage matching *exactly*, and the latest resting stage
   with **delta evidence** — any giveaway that is NEW at that stage present in the save: an
   **owned** event it newly sets (*"if stage 3 flags are set we're in stage 3"* — owned only,
   because a context flag is global story and would pin every map at its final stage on any
   late-game save), a **filter-flag** visibility it newly flips (and the save agrees), or the
   byte newly resting on its script value.
3. **A synthesized `"s<value>"` raw step the byte names** — every script value NO stage carries
   is surfaced as one ("if a state is not a script then a script needs to be in a state"), so
   the state menu covers the whole table; the do-nothing sentinel reads *"Idle (script
   parked)"*, never bare "Noop" (a parked script is not a story position, which is also why
   this is *demoted below the flag evidence*). Applying one writes only the step byte.
4. Else the best-scoring resting stage (+1 matching fact / −1 mismatch, latest wins ties).

The raw step/byte controls remain the power path beside the favored state picker — on BOTH the
Details panel and the World panel ("Current state step" + "Something else…").

## What it means for our code

- **"Map script" should read "Map state"** everywhere on the map screen (Twilight's call in the
  brief) — the byte is one field *of* a state, and the UI concept is the state. Phase MS-3.
- The blueprints are data-only today. The model surface (`MapStatesDB`, `applyState`,
  `rollForward`/`rollBack`, change-map construction landing on `entry`) is **Phase MS-2/MS-4**,
  and the constructed story-map states owe a **console probe pass (MS-6)** before any UI ships
  on them — the forge (`emu_make_map_save`) can boot each stage directly.
- The randomizer gets legal whole-states (MS-5) instead of raw bytes.
