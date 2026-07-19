# Per-map script progress (the 97) & missables (the 228) — the Map Storage panel's big pages

Briefed by Twilight 2026-07-16: the map scripts and the missables join the persistent Map Storage
panel — the script as a **dropdown at the top of each map's page** (title + progression description
per step, custom values via "Something else…"), the missables **sorted into their own group** under
the owning map, with unused/glitch/known-issue flags and the missable↔event-flag conflict hooks.

## The save layout (both verified)

| Block | File | WRAM | Size | Model |
|---|---|---|---|---|
| Per-map script progress | **`0x289C`–`0x2915`** | `0xD5F0`+ (`wOaksLabCurScript`…) | **97 values** in `0x7A` bytes (1 byte each + pret's padding skips) | `WorldScripts` (`world.scripts`) |
| Missable visibility | **`0x2852`–`0x2871`** | `0xD5A6` (`wToggleableObjectFlags`) | 256 bits, **228 used** | `WorldMissables` (`world.missables`) |

- **Twilight's counts were exactly right:** 97 `w<Map>CurScript` variables in `ram/wram.asm`, 228
  `TOGGLE_*` constants in `constants/toggle_constants.asm`.
- The script block's layout (sizes + skips) is `scripts.json`'s — v1's import, kept verbatim.
  **Byte-exactness pinned** by `tst_world::scripts_writeExactlyTheirByte` (offsets hardcoded
  independently: ind 0 → `0x289C`, ind 1 → `0x289D`, ind 3 → `0x28A0`, ind 96 → `0x2915`) and
  `missables_writeExactlyTheirBit` (bit 0 → `0x2852` b0, bit 227 → `0x286E` b3).
- ⚠️ **Polarity:** a missable bit **SET = HIDDEN** (pret: *"bit set = toggled off"*). The panel's
  switch shows the intuitive direction — checked = on the map.
- **Corrects an old note:** the sprites research (2026-07-13) placed `wToggleableObjectFlags` at
  `0x28A0` — that byte is actually **Viridian City's script-progress byte**. The flags are at
  `0x2852` (v1 concurs; pinned by the bit-exact test).

## Scripts: how the dropdown gets its meaning

- Each of the 97 entries owns one map (94) or a small map group (3 gates share a byte:
  Route 16 Gate 1F/2F, Power Plant + Route 7 Gate, Route 18 Gate 1F/2F).
- The steps come from `maps.json` `scriptEntries` (116 maps / 458 steps, imported 2026-07-15 from
  the game's own `SCRIPT_*` constants). **Each step now carries a `desc`** —
  `scripts/import_storage_meta.py` writes a **curated description for the story maps** (Pallet's
  intro, the whole 19-step Oak's Lab opening, Route 22's two ambushes, Cerulean's bridge rival,
  Vermilion's dock, the Championship rooms…) and an honest derived one elsewhere (gym
  START/END/POST pattern, EXIT/cutscene/battle patterns) — every option reads like a stage of the
  map's story, which is the *sense of progress* Twilight asked for.
- `AreaMap::curMapScript` (`0x2CE5`, the **live working step** of the map you're on) shows the
  **same descriptions** under the Details panel's "Current script step" combo — one meaning, two
  places. Relationship: on map entry the game copies the map's `w<Map>CurScript` into the dispatch
  path; editing the per-map byte sets where the story *will* resume, the `0x2CE5` byte is where it
  *is* right now.
- ⚠️ **The out-of-range hazard:** script steps dispatch through a per-map pointer table ending in
  `jp hl`, with **no bounds check** — a value past the map's own table reads garbage as a pointer
  and jumps to it (the same mechanism as the event-flag crash research). The panel's custom path
  accepts the full byte range (never refused) and **warns in words** beyond the named steps.

## Missables: the enrichment

`missables.json` (228 entries, v1's import) now carries, per entry
(`scripts/import_storage_meta.py`, additive + `--check`-idempotent):

- **`toggleConst`** — the pret `TOGGLE_*` name (position-matched; both lists are index-ordered).
- **`scriptToggled`** — **121 of 228 are pret's X-marks**: no map script ever calls
  ShowObject/HideObject on them (item balls and static encounters that deactivate through
  `wToggleableObjectList` detection instead). The bit still controls visibility either way; the
  description says so.
- **`oddity`** — pret's own four oddballs (`TOGGLE_SILPH_CO_2F_1`/`_10F_3` "never (de)activated?",
  `TOGGLE_SILPH_CO_7F_8`/`TOGGLE_UNUSED_MAP_F4_1` "sprite doesn't exist") — flagged **amber** in
  the panel: the bit exists and stores, toggling shows nothing.
- **`desc`** — kind-aware plain English (item ball / trainer / static encounter / character, with
  what hidden means for each).
- **`linkedEvents`** — the **14 verified flag↔object links** from the 2026-07-15 script
  cross-reference (`CheckEvent`-before-toggle), each with the flag name + canonical bit index.
  Placeholder "map-specific got-item" pseudo-flags are deliberately filtered out.

**The conflict hooks (v1 of the missable conflict system):** the panel prints each linked flag WITH
ITS LIVE STATE beside the switch — *"Tied to EVENT_FOLLOWED_OAK_INTO_LAB (ON)…"* — so a flag and a
visibility bit that disagree are visible at a glance. This is the *suspected* tier of the
conflicting-flags doctrine (event-flags plan, Phase 11): directional predicates (which combination
actually breaks) need per-object research and console probes, which is Phase 11's machinery; the
links themselves are baked and ready for it.

## Wiring

- `MissablesDB::deepLink()` **was never called at boot** (found + fixed 2026-07-16 — every
  missable's `toMap` link was silently null). Now runs after `MapsDB::deepLink()`.
- `world.h` fully includes `worldscripts.h`/`worldmissables.h`/`worldevents.h` so QML traverses
  `world.scripts` / `world.missables` / `world.events` (the WorldLocal de-opaque precedent).
- The panel's data comes from `MapModel::storagePages()` (one page per script entry + missable-only
  maps, the legacy trio merged in — Safari stays COMBINED), `storageScriptSteps()`,
  `storageMissables()`.
- ⚠️ **Qt Quick trap encoded in the panel:** a `ComboBox` writes `currentIndex` itself whenever its
  model changes, severing a plain binding — page switches then show the wrong step. The panel uses
  a `Binding` element, which re-asserts after internal writes. (Caught by the screenshot review:
  Oak's Lab stored step 18 while the combo displayed "Default".)

## Honesty ledger

- Script values and missable bits are **durable** save data (inside `sMainData`).
- The 121 never-script-toggled missables and the 4 oddities are **flagged, never hidden**.
- Custom script steps beyond the table are **stored as asked** with the crash-risk warning.
- Deeper per-map semantics (which step arms which trigger, exact conflict predicates) belong to the
  scripts-import + Phase 11 work — the descriptions say what is *known*, not guesses dressed up.
