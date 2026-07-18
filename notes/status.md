# Project Status

_Current state only._ For the chronological history of what changed each session and why, see
[`sessions/`](sessions/README.md) (one file per day). For root-cause mechanics see
[`reference/qt-patterns.md`](reference/qt-patterns.md) and [`decisions/`](decisions/architecture.md). For the
commit-by-commit changelog see [`version.md`](version.md).

**Version:** `0.43.1-alpha` — on `dev`, **awaiting leadership's in-app review, then "ship"**. (Previous
release: `0.16.6-alpha`, shipped 2026-07-11.) Single source of truth: repo-root `VERSION`; see
[`reference/versioning.md`](reference/versioning.md). Full `ctest` green (**91/91**);
`tst_db_integrity` now 15 (two new pins: `everyFlyDestinationSitsAtItsMapId`,
`everyTradeResolvesAndSitsAtItsBit`).

### 🎨 THE LAYERS REWORK — one ink table, the Continue view, tab-drag, storage scroll (2026-07-18, `0.43.1-alpha`)

Twilight's live pass (*"buggy… highly inconsistent… pieces feel like there shoved in half baked"*)
plus her mid-session follow-up (tab-drag, snappier, click-off closes panels), delivered as one
rework. Full story: [`sessions/2026-07/2026-07-18.md`](sessions/2026-07/2026-07-18.md).

- ⭐ **`MapEngine::ink(key)` is the ONE colour table** — panel swatches, canvas properties, every
  spot (model-attached ink; tile traits wear their overlay's swatch) and the object components all
  read it. Three disagreeing palettes retired; People purple, Screen box red, selection white
  everywhere. **Three new Layers rows** (Hidden pickups / Script triggers / Event flags, default
  ON) so every canvas ink is IN the panel.
- ⭐ **The Continue-load view:** a filter-flag-hidden sprite's artwork is not drawn (its box stays;
  tab-select shows a ghost). 🐞 **Silhouettes had NEVER rendered** (`visible: false` source → empty
  layer texture → MultiEffect drew nothing — her "they outline black"); fixed, verifiable only on
  hardware (shaders don't run offscreen).
- **Z-order:** boxes (z 0) under objects (z 1); the hovered block's strip flips above (z 3).
  **Hover highlights** on all draggables + they hand the cell's tabs the withdraw signal.
  **Tab-drag** moves the thing itself (canvas proxy-drag; byte-exact commits). **Click-off closes
  panels** structurally (groundClicked fires only past every panel/tab/popup early-return).
- 🐞 **Map Storage scroll root cause:** `scrollToHighlight` mapped the MISSABLE section regardless
  of target (one hardcoded id) + a one-shot timer racing the layout. Fixed (target + a 450 ms
  settle window re-applying on content growth); pages open at the top. "Cut trees" → **"Cuttable
  trees"**. Snappier tab animations (60 ms).
- ⏳ **Owed: Twilight's live pass** — silhouette colours, hover/drag feel, click-off behaviour.

### 🗺️🧭 MAP STATES — BUILT: model + rename + UI, on the blueprints (2026-07-17, `0.43.0-alpha`)

The second half of Fairy Fox's brief, delivered same-day on her go-ahead (ground rules: map change
**seamless** — *"as though the map has always been loaded"*, no prompts; cross-map shared globals
written **naturally**; transients **shown**). On top of the 98 blueprints:

- **Model:** `MapStatesDB` (blueprints in `db.qrc`, typed stages) + `mapmodel_states.cpp` —
  `stateList` / `currentStateId` (exact match or an honest "custom") / `applyState` (script byte +
  events + own missables + badge universe, nothing else) / branch-aware `rollForward`/`rollBack` /
  `changeMapConstructed` (`Area::setTo(map,x,y)` new deterministic overload — the whole Area block
  from ROM, player on the blueprint's first-warp entry, live step resuming the map's stored
  progression, `wLastMap` aimed for the `$FF` doors, edited-warnings reset).
- **Rename (leadership's call):** "Map script" → **"Map state"** on the Map Storage panel;
  "Current script step" → **"Current state step"**; run-on-load wording matched.
- **UI:** the Details panel's **Progression state** picker ("1. Name" / "2a." branches / italic
  mid-cutscene transients / flagged derived / a Custom row) + **◀ ▶ roll buttons** + stage story
  text; MapPicker's **"Construct the map on change"** switch (ON by default; OFF = the old
  one-byte power path, notice intact). Screenshot-reviewed: panel, picker popup, storage heading.
- **Pinned:** new **`tst_map_states` 8/8** — every DB reference in range (98 blueprints / 34
  curated), apply moves ONLY state-region bytes (whole-32KB byte-diff), rolls round-trip
  1→2→3→2→1, a gym stage moves its badge PAIR (0x2602+0x29D6) and no other badge, construction
  lands on the entry spot with a coherent header. Full suite **92/92**; `tst_qml_screens` 28/28.
- ⭐ **A finding the tests forced:** BaseSAV's Pallet Town sits genuinely BETWEEN stages 2 and 3
  (Daisy walks, Town Map given, Poké Balls never collected) — the matcher answers **"" (custom)**
  for it, which is the honesty doctrine working, not a gap.
- ⏳ **Owed:** Twilight's live pass (the picker feel, a seamless map change, a roll on a gym);
  MS-5 (randomizer hook) and MS-6 (console probes of constructed states) are the open phases.

### 🗺️🧭 MAP STATES — per-map progression blueprints, researched + shipped as data (2026-07-17, `0.42.13-alpha`)

Fairy Fox's brief, the research half delivered end-to-end: every scripted map now has a
**progression blueprint** — the researched default states (script byte + event flags + missables +
badges, ABSOLUTE per stage), the triggers between them, start/end/branches, and a designated entry
spot (first warp) — so the app can roll a map back/forth one stage at a time, construct a proper
destination map on map-change, and feed the randomizer legal whole-states.

- **Data:** `projects/db/assets/data/map-states/` — **98 blueprints + `_index.json`** (98 = every
  real `def_script_pointers` file; the other 18 of the old "116" were unused-map aliases).
  **196 resting stages**; **34 story maps hand-curated** (Pallet/Oak's Lab arc, all 8 gyms, Route
  22's double ambush, Mt. Moon's fossil **branch** (`2a`/`2b` — the id scheme's reason), Tower,
  Silph, Snorlaxes, the E4 chain, Champion, **Hall of Fame's deliberate E4 wipe**, Safari gate).
- **The model** (the headline finding): `SCRIPT_*` values split **resting (147) vs transient
  (234)** — and a stage is MORE than the byte (gyms rest at 0 on both sides of the leader;
  S.S. Anne 2F's rival leaves *only* the byte as its record). Stage naming: `1, 2, 2a, 2b, 3`;
  cutscene steps ride as `N.k`.
- **Tooling:** `scripts/extract_map_states.py` (register-`a` simulation for helper-setter script
  writes, fallthrough closure — Pewter's whole victory lives in a fallthrough body — engine-owned
  table routines, file-wide owned-flag universe, `--check` idempotent, every curated name
  validated against events/missables/scripts vocabularies) + the story layer
  `scripts/data/map_states_curated.json`. Digest for future curation: `--digest`.
- **Docs:** [`reference/map-states.md`](reference/map-states.md) (the traps, each load-bearing),
  [`plans/map-states.md`](plans/map-states.md) (phases MS-2..6: `MapStatesDB` + model surface,
  the **"map script" → "map state" rename**, the state-picker UI, randomizer hook, console
  verification via the forge), decision entry in `decisions/architecture.md` (a NEW format,
  argued for — pret has no data format for this; it exists only as executable asm).
- ⏳ **Owed:** Twilight's pass on the plan's open questions (map-change default state, cross-map
  context flags on roll, transients in the dropdown), then MS-2 onward. MS-6 console probes gate
  any UI. Data-only change — no app code touched; suite untouched (91/91 stands).

### 🤝🏙️ MAP STORAGE grows: trades, town-visited, milestones, fossil + a General page (2026-07-17, `0.42.12-alpha`)

Fairy Fox's day-long brief, delivered end-to-end (Phase 17 in [`plans/map-screen.md`](plans/map-screen.md)).
Research → 4 console probes → data/model → MapModel surface → panel + canvas, all committed and green.

- **Four new Map Storage sections + a General page.** In-game trades (all 10, on the trader's tile;
  Cinnabar Lab Trade Room shows both; the unused CHIKUCHIKU → **General**), the town **Visited** checkbox
  (11 city maps; the current town wears a *dynamic* amber-! because it re-marks itself on Continue),
  the **Milestones** (rods/Lapras/starter/nurse/guards/`startedElite4`), and the **Fossil** (two bytes,
  shown but **not** synced). Trade tabs on the canvas too.
- **⭐ The two GROUP kinds** (leadership): **shared** (one bit, many maps — Saffron guards, starter,
  nurse) vs **alike** (different bits, same kind — towns, trades, rods, **and hidden items + coins as
  TWO separate groups**). Alike groups afford **view-all** + **check/uncheck-all**.
- 🐞 **Four v1-carryover / data bugs fixed, all the same shape** (right bits, wrong words, never
  asked the game): **`fly.json` was wrong for 6 of 11 towns** (v1 shipped mislabelled Fly checkboxes
  since 2018 — fixed + pinned, negative-controlled); **`defeatedLorelei` is `BIT_STARTED_ELITE_4`**
  (arms an Elite-4 *wipe*, not a victory — renamed, byte-identical); `wElite4Flags` b0 is dead
  (`BIT_UNUSED_BEAT_ELITE_4`); the fossil's two bytes are **independent** (console-proven).
- 🐞 **Two bugs the mandatory screenshot review caught before ship:** the "view all towns" list
  rendered in hash order (QSet → now sorted), and trade houses / towns / the fossil room had **no
  selectable page at all** until added to the picker (same hole the event-flags pass closed).
- 🐺 **A mistake, owned:** the first cut of the trades DB **duplicated an existing `TradesDB` and
  overwrote the shipped `trades.json`** (reusing `textId` for a different type — would have silently
  zeroed every dialog set). Reverted, then redone **additively** on the existing DB (0 existing fields
  changed, proven). The lesson — *search the concept, not the upstream spelling* — is the one I'd just
  written into `in-game-trades.md` §6 and failed to apply to the DB.
- **New research notes:** [`in-game-trades.md`](reference/in-game-trades.md),
  [`town-visited.md`](reference/town-visited.md), [`world-completed.md`](reference/world-completed.md),
  [`fossil-revival.md`](reference/fossil-revival.md) — the last completes v1→v2 map-page transference.
  Probes: `probe_in_game_trades.py`, `probe_town_visited.py`, `probe_world_completed.py`, `probe_fossil.py`.
- ⏳ **Owed: Twilight's live pass** — the group check-all/view-all in motion, the Milestones/Fossil
  sections' scroll-feel (offscreen automation can't scroll to them), and the fossil independence.

### 📷 THE CAMERA JUMP — her description WAS the diagnosis (2026-07-17, `0.42.9-alpha`)

> *"The camera seem to mess around anytime something is committed to the data … other times the
> camera seems to reset to the default position when you first open it."*

Both halves exact, and together they name the bug precisely. `frameOnPlayer()` set `framed = true`
but **never set `framedMap`** — so the opening frame left the canvas at `framed: true,
framedMap: -1`, **confirmed by reading both off the running app**. `changed()` fires on *every* data
commit, and the first one compared `mapInd (23) !== framedMap (-1)`, concluded *"a different map"*,
and re-framed to the opening shot. Hence **"sometimes"**: only the FIRST commit throws the camera,
because it sets `framedMap` on its way past. Fixed; `framedMap` now reads **23** on open.

> ⭐ **Two lessons, and the second is the one that keeps costing:**
> - **"Sometimes" is a clue, not noise.** It meant *once per map load*, which is exactly what a
>   one-shot guard initialised too late produces. Taking the word literally found it in minutes.
> - 🐺 **Use the app's own truth, not arithmetic.** Twilight: *"why dont you use the actual
>   screenshotting tools of the mcp server instead of calculating your own"*. I had "found" a
>   cell-outline bug by computing where a block should be on screen and cropping there. There was no
>   bug: `app_get` shows `hoverBlockX = 8` and `at.blockX = 8` **agree** — the status bar reports MAP
>   coords (5,7), the hover BUFFER coords (8,10), and 5+3=8 is the 3-block border ring. My converter
>   was the defect. Ask the app; it knows.

### 🧭 ONE SYSTEM FOR THE MAP — the standardization (2026-07-17, `0.42.6-alpha`)

Second live pass: *"still buggy and glitchy… some sprites have no box, some do… mousing over things
nothing comes up… clicking water doesnt even bring up wild mons… **theres no standardixation**"*.

**The fault was structural, and three rounds of polish could never have reached it.** The canvas had
**four** systems: `MapSprite`, `MapWarp` and `MapSign` each drew, hovered and selected themselves —
and the tab layer was bolted on beside them. So the map answered *differently depending on what you
pointed at*: a flag-governed sprite had a box and a tab, a plain one had neither, water had no tab,
and hovering most of the map did nothing. **Four disagreeing systems cannot be polished into one.**

- ⭐ **There is ONE list now.** `MapCanvas.storageBlocks` merges the ROM's spots (flags, scripts,
  hidden pickups, tile traits) with the SAVE's objects (people, doors, signs) into one per-block
  model. Every one is a spot with a **kind, a destination and a tab**. The object components still
  draw + drag themselves (`hilite: false`); what they gain is **being reachable**.
  ⚠️ This list **depends on `revision`** and must — the old "ROM-derived, nothing you edit changes
  it" invariant died the moment the save's own objects joined it.
- 🐞 **Water and grass were literally switched OFF.** Tile traits carried `section: ""`, and both the
  tab and the block's hit area were `enabled:` **only when a section existed** — so one empty string
  cost hover, tooltip *and* click across most of a water route. **Every kind now has a real
  destination**: grass/water → the **Wild Pokémon** panel (that is what they *are*), traits → the
  **Tileset** panel, objects → **Details**. Pinned: *no spot may be inert*.
- 🐞 **"1 spot = no tabs" was MY rule, not hers**, and it is gone. It made the map inconsistent — a
  two-thing block was interactive, the water block beside it was dead. **Every block with anything
  gets tabs.**
- 🐞 **Clicking a tab selected the block underneath it.** The ground's `TapHandler` acts *before* any
  item's MouseArea (handlers all fire first, and `DragThreshold` takes no grab) — **the third time
  this exact rule has cost a bug**. The ground now asks `overTab` and stands down. The block's own
  area is `Qt.NoButton`, hover-only, `z: -1` — a full-map grid that ate presses would have broken
  dragging everywhere. [`reference/qt-patterns.md`](reference/qt-patterns.md) (top).
- ✅ **Hover does something**: hovering a tab **lights its own thing** on the map; the cell tints.

⏳ **Owed:** her live pass. And an honest worry: with a tab per block, a water route is now **dotted
with tabs** — uniform, as asked, but possibly too much furniture. That is a look-at-it call.

### 🐺 THE LIVE PASS FOUND WHAT 91 GREEN TESTS DIDN'T (2026-07-17, `0.42.5-alpha`)

Twilight opened it and, in about two minutes: *"none of this works actually"*. She was right about
every item. Kept in full, because the **shape** of the miss matters more than the fixes.

- 🐞 **Map Storage opened COMPLETELY BLANK, on every map.** `MapStoragePanel.qml` used `FlatToggle`
  — not a type that file can see — and **an unresolved type takes the whole component down
  silently**. The suite was **91/91 green** because a dock panel lives behind a **Loader that only
  builds when the dock is opened**, so its QML was never compiled by anything.
  **Fixed** (`MapSwitch`, the panel's own idiom) **and the hole is closed**:
  `tst_qml_screens::everyMapPanelCompiles` now compiles every Map panel + canvas item on its own —
  **negative-controlled** (put `FlatToggle` back → *"failed to COMPILE -- it would open blank"*).
- 🐞 **The tooltips were unreadable.** *"a very dark and extremely large tooltip filled with tons of
  text… way too much reading"* — a 5 px square was hanging the name + state + full description +
  caution + an instruction off itself. Now **one short line** ("Potion — hidden pickup"). A tooltip
  on a dot is a **label, not a page**.
- 🐞 **Hover did nothing.** A strip of coloured dots means nothing until you can tell which is
  which. Hovering a tab now **lights its own box on the map** and the tab lifts — which is also the
  *"everything overlapping is accessible under mouseover"* rule made real.
- 🐞 **Sprites didn't read as movable.** Warps + signs already ship the solid+filled language
  (`#66`-alpha chips); the sprite — the *most* draggable thing — didn't. Now washed `#26` when
  selected (light, because it is the one movable object with **artwork** worth seeing through).
- ⭐ **A proper bench exists now:** `assets/saves/forged-maps/Route12.sav` — **console-authored**,
  walked to through a hijacked door by the real ROM, so every byte of the Area block is the
  engine's own. **10 sprites · 2 signs · 4 warps · a hidden item at (2,63) · the Snorlax**, and a
  10×54 water route. Pallet Town was too quiet to review on. See that folder's README.

> **The lesson, and it is the `emu-venv` lesson again in a new costume: a check must be able to
> fail.** A test that loads a *screen* does not test what the screen **refuses** to load. If a thing
> is built lazily, something must build it eagerly, or "green" only ever meant "never asked".
> Full write-up: [`reference/qt-patterns.md`](reference/qt-patterns.md) (top).

### 💎 HIDDEN ITEMS (16f-b) — the data landed, and the DB that never loaded it (2026-07-17, `0.42.2-alpha`)

Phase 1 of Fairy Fox's hidden-items brief. Research + traps + lessons:
[`reference/map-storage-locations.md`](reference/map-storage-locations.md) → §2a/2b/2c.

- **The counts are final: 54 items, 12 coins.** Answering *"unless there's more"* — there are not.
  `MAX_HIDDEN_ITEMS` is 112 but only 54 rows exist; both counts match `WorldHidden` exactly.
- **What's buried is now imported** (leadership-approved): `import_hidden_items.py` joins pret's
  `hidden_events.asm` onto `hidden_item_coords.asm` → **21 distinct items**, and for coins **how many
  you're picking up** (10/20/40, **260 total**). Additive-only — **0 existing fields changed** across
  all 66 rows, proven semantically, not eyeballed.
- 🐞 **…and `HiddenItemsDB` had NEVER loaded a single row.** `AbstractHiddenItemDB::load()` used
  `static bool once` — and **a static local in a base-class method is ONE static for the whole
  hierarchy**, not one per subclass. `db.cpp` builds **HiddenCoinsDB first**, so it tripped the guard
  and the items DB returned early. All 54 hidden items were an empty store for as long as the code has
  existed. Same in `deepLink()`. **Negative-controlled:** restore the old semantics and the count drops
  to **0 of 54** on demand. Second bug: **`HiddenCoinsDB` was missing `DB_AUTOPORT`** → never exported.
- **Items and coins are now separate per-map lists.** They are different save arrays (`0x299C` /
  `0x29AA`) with **independent numbering**, so one shared list made an entry's bit ambiguous. Each
  entry carries its own `ind` (**== its save bit**) and `isCoin`.
- **Pinned:** `tst_db_integrity` **13/13**; full `ctest` **91/91**.

> ⚠️ **The lesson that outlives this DB: a test that loops over everything passes VACUOUSLY on
> nothing.** `allSubDbsLoadAndCount` asserted `>= 0` and went green on an empty store for years — and
> the first cut of the new resolve test **also passed while the bug was live**, because it iterated
> zero entries. It now asserts non-empty *first*. Same shape as the `emu-venv` gate: **a check must be
> able to fail.** And `tst_db_coverage_fill.cpp` had *written the symptom down* — *"the HiddenItems
> store is empty"* — and filed it as a quirk of the fixtures. It was the bug, in writing, unread.

⏳ **Owed (the rest of her brief, phased — restructure first, her call):** the script coord-trigger +
event-flag extraction (16f-b part 2) · `flagHotspots()` → **`blockHotspots()`** (locations owning spot
lists; must admit **tile traits**) · hidden items/coins **in the Map Storage panel** under their maps ·
the **tab strip** + interaction model (hover-reaches-all · click-priority-is-layer-order ·
click-**and-drag** from a tab).

### 🟢 CI IS GREEN — first time since 0.29.0-alpha (2026-07-17, `0.41.9-alpha`)

`tests` **success** (linux-asan + windows) · `lint` **success** (0 gated findings / 150 TUs) · local
`ctest` **91/91** · local `dtest.ps1 asan` **0 ASan/UBSan findings**. Story:
[`sessions/2026-07/2026-07-17.md`](sessions/2026-07/2026-07-17.md).

**The remote had been red from 0.29.0-alpha to 0.41.7-alpha and local green never saw it.**
`mapengine.cpp` used **`QImage::flipped()`** (**Qt 6.9+**) to draw the player facing right; the kit here
is **6.11** → compiled, rendered, passed. **Every remote build pins Qt 6.8.3** (`tests`, `lint`, `pages`,
**`release`**) → `no member named 'flipped' in 'QImage'`. The sharp end: **`release.yml` builds the same
`appcore` and hadn't run since `v0.16.6-alpha`, which predates the break — the next "ship" would have
died at the release build, after the merge to `main`.** Fixed (`0.41.8-alpha`) with a
`QT_VERSION_CHECK(6,9,0)` guard; `tst_map` **27 passed on BOTH paths** (fallback force-compiled, since
6.11 never compiles it).

**Then the unblocked gates found 4 real, pre-existing bugs** (all fixed, `0.41.9-alpha`):

- **heap-use-after-free** in the music parser — `addrOf()` held a `const QString&` **into `pending`**
  while `place()` appended to `pending` and reallocated it. 7 ASan reports from 5 tests, **one** cause.
- **null deref** — `MapModel::mapInd()`'s bare `map->curMap` (the class null-checks `map` everywhere
  else, which is *why* the analyzer could prove it).
- **OOB read** — the `SgbCity` index was missing its `>= 0`, behind a redundant test that hid the gap.
- an unnecessary copy.

**Why nothing local caught them: BOTH safety nets were down.** ASan doesn't run on the llvm-mingw kit,
so the container is its only home — **and the container had been unbuildable since `pse-audio` landed**
(image had `-m qtcharts`, so configure died on *"Could NOT find Qt6Multimedia"*). Meanwhile the remote
couldn't compile. Container fixed: `qtmultimedia qtshadertools` (matching `tests.yml`) + **`libpulse0`**
(`libQt6Multimedia.so` has a hard `DT_NEEDED` on `libpulse.so.0`; GitHub's runner ships it, a clean
container doesn't).

- ✅ **The container is FIXED — 91/91 standard, 91/91 asan, 0 sanitizer findings, 0 fontconfig errors**
  (2026-07-17). ⚠️ **And the diagnosis above it was backwards** — kept here as the correction:
  - It was **3 failures, not 13**, and the **fontconfig class was already gone** (0 occurrences; the
    24× count was stale from the months the image was broken).
  - The three were **not** "the container lacks the ROM + PyBoy". **The exact opposite**:
    `run-tests.sh` rsynced the host's `tmp/` — **Windows `emu-venv` and all** — *into* the container,
    and the skip-gate only checked `QFile::exists("tmp/emu-venv/Scripts/python.exe")`. On Linux that
    answered **true**, so the gate never fired and the test tried to exec a **Windows PE binary**;
    WSL's binfmt interop caught it and failed sideways (`<3>WSL ERROR: UtilGetPpid:1330 ...`),
    which read like a code fault. CI was green precisely *because* a fresh checkout has no `tmp/`.
  - Fixed **twice, deliberately**: the gate is now **runnability-based, not existence-based**
    (`tests/helpers/emuvenv.h` — shared by all three; *"does this interpreter run?"* is the only
    question that matters, and it's platform-correct: `Scripts/python.exe` vs `bin/python3`), **and**
    `run-tests.sh` excludes `tmp/` (host scratch has no business in the container). Proven
    independently: re-inject the Windows venv and the test now **SKIPs with a message that explains
    itself** instead of going red.
  - **The lesson, and it generalises:** *a file existing is not a capability being available.* An
    availability gate must test the capability, or it will pass in exactly the environment it was
    written to protect.
- ✅ **DECIDED (leadership, 2026-07-17): ONE Qt everywhere — `6.11.0` in all five build files**
  (`tests.yml` ×2, `lint.yml`, `pages.yml`, `release.yml`, `docker/Dockerfile`), matching the kit.
  The dev-newer-than-ship gap caused the 12 invisible-red releases and would have recurred on the next
  6.9+ API; rather than hold the app back on 6.8.3, the shipping toolchain moved **up**. Pinned by a
  ⚠️ block atop `tests.yml` + a cross-reference in each file.
  - ⚠️ **The Windows jobs need a git-pinned aqtinstall, and must keep it.** Qt re-laid-out the
    **Windows** repo per-arch at 6.11 (`qt6_6110/qt6_6110_llvm_mingw/`, no parent `Updates.xml`);
    released aqt (3.3.0) still nests once → 404 → the job dies in 35 s. Fixed upstream by
    **[aqtinstall#1000](https://github.com/miurahr/aqtinstall/pull/1000)** (merged 2026-03-24,
    **unreleased** — 3.3.0 is from *June 2025*), sourced via the action's `aqtsource` input, **pinned
    to the merge commit** for reproducibility. Linux unaffected. **Delete those two lines when
    aqtinstall releases past 3.3.0.**
  - 🐺 **A wrong conclusion was caught by leadership, not by me.** I wrote this up as a hard *ceiling*
    ("Windows can have at most 6.10.x") off one lazy step — *PyPI's newest is 3.3.0, therefore no fix
    exists* — and put three options on top of it. Twilight: *"i find it hard to believe the community
    has no solution for this moving forward."* The fix had been merged **four months** earlier.
    Lessons (**"latest release can't" ≠ "can't"** · an ecosystem with no answer is a **smell** · don't
    promote an unchecked constraint into someone else's decision):
    [`decisions/rejected.md`](decisions/rejected.md).
  - ⏳ **Owed: the branch CI run on the patched config** — `feature/qt-6.11-alignment`. Verified
    locally (patched aqt in a throwaway venv lists `win64_llvm_mingw` @ 6.11.0 + all three modules),
    but *local proof of a remote claim* is the exact blind spot that started this, so it is **not
    green until the remote says so.**
  - Writeup: [`reference/qt-patterns.md`](reference/qt-patterns.md) (top); lookup rows in
    [`reference/fix-patterns.md`](reference/fix-patterns.md).

### 🎫 EVENT FLAGS — all 2,560 researched, data regenerated, model fixed (2026-07-16, `0.41.6-alpha`)

The **Events** feature (v1's cryptic `1cc`/`1cd` page) is researched end-to-end and the data + model are
**fixed and pinned**. Research: [`reference/event-flags.md`](reference/event-flags.md); plan:
[`plans/event-flags.md`](plans/event-flags.md).

- **The field:** `wEventFlags` = **ONE contiguous 320-byte bitfield**, save `0x29F3`–`0x2B32` (WRAM
  `0xD747`–`0xD886`), ending exactly where `wGrassRate` begins. `NUM_EVENTS = $A00 = 2560`, fixed by ROM
  — so there are **not** more. Bit `i` = byte `0x29F3 + i/8`, bit `i%8`. **Console-verified live.**
- **All 2,560 have a dossier** — name, description, map, group, classification — generated from pret
  (`import_event_flags.py` → `analyze_event_usage.py` → `generate_event_dossiers.py`). **537 named**
  (incl. 4 researched out of pret's `; ???`) + **30 block-swept** + **2,023 "Placeholder Flag #<hex>"**
  (zero code presence; byte-alignment padding — every per-map block base is ÷8).
- 🐞 **The editor was writing the WRONG FLAG, now fixed.** `events.json` is **regenerated from pret**
  (508 → **2,560**; `import_events_db.py`, self-validating): **14 mislabels** in the Pokémon Tower block
  (shifted ~2 bits — *"Beat Pokemontower 7 Trainer 0"* actually set **`EVENT_BEAT_GHOST_MAROWAK`*) and
  **14 phantoms** (pointing at bits the game never uses) are gone. `eventCount` 508 → **2560** in
  `worldevents.h` (it MUST match the DB store size or `load()` overruns the array).
  ✅ The byte/bit maths was **always flawless** (0 mismatches) — a labelling fault, never corruption. ✅
  All **529 curated map lists preserved verbatim** (they only *looked* invalid: they use `modernName`,
  which `MapsDB` indexes — no bug).
- **Pinned:** `tst_world::events_everyEntryIsAtItsCanonicalBit` (coverage 2560 + every entry at
  `0x29F3+ind/8`) and `::events_writeExactlyTheirBit` (one toggle → **exactly one byte** moves).
  ⚠️ Neither catches the *mislabel* class (names, not offsets) — that is guarded by **generation**
  (`import_events_db.py --check`).
- 🛑 **The conflicting-flags system is SHELVED** (leadership 2026-07-16, agreed) — its founding case was
  a **false positive**, only `confirmed` may warn, and adjudication is bespoke per case. Knowledge kept,
  **no conflict UI ships**. [`decisions/rejected.md`](decisions/rejected.md).
- ❌ **"All flags on crashes" — UNREPRODUCED** on the cartridge (all 2,560 set: healthy boot → Oak's Lab
  → ~12 map transitions). The *mechanism* is real; the claim is not proven. Bulk-set warning stays but
  must read *"unpredictable and unverified"*, **never "this crashes"**.
- ✅ **The UI is BUILT** (`0.41.7-alpha`). The **Event flags** section is the 4th section of every map
  page in `MapStoragePanel` — **above** Filter Flags, **nothing collapsed** (a page you scroll), a
  **group toggle** per group, and rows that say what a flag *does*: *"Turned on in Oak's Lab, and read
  back by Pallet Town"* (named from the usage cross-reference — not "story flag"), plus classification
  chips (**temporary · does nothing · never used · swept in a group**), cautions, and the raw
  `byte 0x29F3 · bit 0` kept quiet but visible. **Shared groups** show a multi-map flag on **every** map
  it spans, labelled and naming the others (Silph Co's bits on all 12 floors). **Placeholder Flags**
  last. **No conflict UI** (shelved). `MapModel::storageEvents()` reads `MapDBEntry::toEvents`.
- 🐞 **…and a third of the game was unreachable.** `storagePages()` only made a page for maps with a
  script entry or missables, so **Celadon City, Lavender Town, Route 1, Indigo Plateau, Fuchsia City,
  Mt. Moon B1F, five S.S. Anne decks + ~20 more had NO page** — their flags could not be edited at all.
  Fixed: every map an event is filed on is selectable (**108 → 141 pages**).
- **Heavy-page check:** worst is **Silph Co 7F, 227 rows** (median 16) — renders fine fully expanded
  (6055 px scroll, no hang). **No virtualisation needed.**
- ⏳ **Owed:** Twilight's **live pass** (scroll/toggle feel — a still PNG can't judge it). And the
  **on-canvas hotspot layer** (Phase 10 — a clickable box on the map opening the panel at that flag;
  Oak's two spots) is **NOT built**: the research is done (918 objects, 226 conditional, the flag↔object
  links) but it needs its own brief/design pass first.

### ⚔️ ROUTE 22 RIVAL CONFLICT — console-adjudicated REFUTED (2026-07-16)

The founding case of the Phase 11 conflict system, settled on the cartridge — the "hard thing" the
scripts + missables were needed for. The forge gained **total trigger control** (`scripts` +
`filter_flags` in `forge_save.forge()`, wired through `emu_boot`/`emu_forge_save`), which let
`scripts/emu/probe_route22_conflict.py` **arm the ambush and drive it into the battle**: both battle
flags on + both `SPRITE_BLUE` shown, player on the (29,4) trigger → coord trigger fires, rival walks
over, **normal trainer battle engages, no crash** (sane 960+ frames). `Route22DefaultScript` checks
1ST before 2ND (ordered if/else), so the second flag is never consulted. `conflicts.json`:
`suspected → refuted`. Lessons (in [`reference/forged-saves.md`](reference/forged-saves.md)): a forge
drives coord triggers but not cutscenes without correct geometry + A-mashing; a **settle-only harness
misreads the rival's text box as "healthy"** (why 2026-07-15 was inconclusive). Plan verdict:
[`plans/event-flags.md`](plans/event-flags.md) → Phase 11.

### 🗺️📜 MAP SCRIPTS + MISSABLES on the Map Storage panel — BUILT (2026-07-16, `0.41.0-alpha`)

Twilight's brief, delivered: every map with storage is a page (~100). Top of each page: **the map's
script** (the 97 `w<Map>CurScript` bytes at `0x289C`, `WorldScripts`) as a dropdown whose steps read
like a story (458 descriptions — curated beats for the story maps), custom values via "Something
else…" with the **out-of-range jp-hl crash warning** (stored, never refused). Below: the legacy
gym/Safari bytes, then **the missables group** (228 bits at `0x2852`, bit set = HIDDEN;
`WorldMissables`) — "on the map" switches with descriptions, pret's 4 oddities amber-flagged, 121
never-script-toggled marked, and the **14 verified flag↔object links shown with live flag state**
(the suspected-tier conflict surface; predicates await event-flags Phase 11). Details panel's
curMapScript combo shares the descriptions. Fixed en route: `MissablesDB` never deep-linked at boot;
the old `0x28A0` missables-offset claim corrected (that's Viridian's script byte); a ComboBox
model-reset binding trap (caught twice by the screenshot review). Everything:
[`reference/map-scripts-missables.md`](reference/map-scripts-missables.md). Green: `tst_world` 19/19
(two new byte-exact keystones), `tst_qml_screens` 16/16. ⏳ **Owed: Twilight's live pass.**

> **Connections live-review fixes (0.39.5-alpha):** the added-connection dead-state bug (interactive
> strip now save-based, not DB-based), neighbour maps animate, and the smart grouped picker (★ default +
> opposite-edge fits + sizes). Owed: the map **ring** rendering the save's connections, and the
> **living/wandering-NPCs** brief (both tracked in `plans/map-screen.md`).

> **Releases are MANUAL.** Commit and push to `dev` freely, but `main` only moves when Twilight says
> **"ship"**. Green is necessary, not sufficient. See [`reference/git-workflow.md`](reference/git-workflow.md).

### 🐛 KNOWN, UNFIXED — the wallpaper attribution overlaps the Credits cards (found 2026-07-17)

Caught by the mandated screenshot review while adding the aqtinstall credit. **Pre-existing, not from
that change**, and **not fixed** — flagged for Twilight because the fix is hers to choose.

**What:** on the **About / Credits** modal, `CreditWork` — the faint wallpaper attribution (*"Basic
Pokemons Colors" by yoshiyaki (CC-BY-NC-ND 3.0)* + URL) — is `anchors.bottom`/`.right` on the Page and
declared **after** the `ListView`, so it floats on top while the cards scroll **underneath it**. Card
text runs straight through it (clearly visible over the *Data Sources* note and the `aqtinstall` link +
MIT line). On **Home** the same component is fine — it floats over artwork, not over live text.

**Why it wasn't just fixed:** two standing rules point away from a unilateral change. *"UI/UX decisions
are a design decision — do not independently change QML appearance"*, and this text is
**licence-mandated** (CC-BY-NC-ND requires visible attribution), so where it goes and how legible it is
has legal weight, not just aesthetic. An overlap is a bug; **the remedy is a design + licensing call.**

**Options, for her:** give the ListView a bottom margin/footer inset so content can never reach it ·
move the attribution into the *Wallpapers* card it describes · give it a solid backing plate · pin it
to the scrolling footer rather than the Page. **Not started.**

## Current state (read this first)

### 🧭 THE AUTOPILOT'S PROGRESSION LAYER — "progress normally when asked", 16/16 (2026-07-16)

Same-day follow-on brief, delivered in full: **natural drop-in is the default** (`emu_goto` boots
ONE MAP OUT and walks in for real — `wLastMap` and the entry state authored by the walk; verified:
Mt Moon entered from Route 4), the **Saffron guards** are handled (`wStatusFlags1` bit 6, set +
reported when a gate is on the route), **elevators are ridden** (car door warps re-aimed live —
the floor menu's own technique; Celadon Mart 1F→5F), **Surf works by poke** (⭐ research finding:
`wWalkBikeSurfState`=2 before stepping onto water — Pallet→Route 21 crossed), **cut trees are
cleared** (our own `cutTreeBlocks` block+screen poke; Vermilion Gym's mandatory tree), **spinner
mazes avoided + slides settled** (coords imported from pret's scripts; across Rocket Hideout B2F),
**bike auto-provisioned** (Route 16→17), and **`emu_battle('sweep')` wins on request** — Brock
beaten on demand (flag cleared live, talked, swept). New levers: `emu_set_flag` / `emu_give_item` /
`emu_move_sprite`. Every intervention reported (`prep`/events), every one opt-out. Two more traps
now structural: **coords update at step START** (`wWalkCounter`==0 is the real step-over signal)
and **BaseSAV's progression hides mechanisms** (reset what you test). `probe_autopilot.py` **16/16**.
Docs: [`reference/dev-mcp.md`](reference/dev-mcp.md) → "The progression layer".

### 🧭 THE DEV AUTOPILOT — pathfinding + auto-navigation over MCP, briefed + built + 7/7 (2026-07-16)

Twilight briefed "describe the destination and the server takes you there" — and the rest of
comprehensive high-level control: find a battle, execute it a certain way, talk to a **moving** NPC,
long hauls (Mt. Moon end-to-end, town-to-town), one-call multi-step runs for the game AND the app.
Built: `scripts/emu/navigate.py` (A\* + Dijkstra-over-portals off our own shipped map data),
`scripts/emu/autopilot.py` (the WRAM-verified executor in the session child), MCP verbs `emu_goto` /
`emu_walk_to` / `emu_talk_to` / `emu_battle` / `emu_hunt_encounter` / `emu_dismiss` / `emu_play` +
`app_flow`. **Console-verified 7/7** (`probe_autopilot.py`) incl. **Mt Moon 1F → B2F** and the long
haul **Pallet → Pewter through Viridian Forest** + a hunt won with `move:1`. Four real traps found
and fixed structurally (double-step holds; `wCurMap`-first transitions; solid warp squares;
post-clamp connection offsets). Plan: [`plans/dev-autopilot.md`](plans/dev-autopilot.md); reference:
[`reference/dev-mcp.md`](reference/dev-mcp.md). Also: "Filter Flags" casing fixed on the Map Storage
panel. ⏳ Owed: Twilight's live pass; HM routing/bike/spinners/elevators are stated v1 limits.

### 🗺️ FORGED SAVES AT ANY MAP — console-authored, briefed + built + verified (2026-07-16)

Twilight briefed the **total custom state resume**: the MCP server generates a proper save at any
map/position/flags in one call. Design: **the game authors the state itself** —
`scripts/emu/forge_map_save.py` walks the real console to the target through a hijacked door
(edge-hop for the 8 warpless routes, columns pre-filtered by our own collision data) and dumps the
settled WRAM in `SaveSAVtoSRAM`'s own layout. Found + encoded a real Gen 1 mechanism on the way: the
**same-tileset warp trap** (`LoadTilesetHeader` skips arrival positioning — outdoor targets are
reached from inside Red's house). `relocate()` keeps block coords + view pointer in sync;
`forge()` refuses the chimera. **Console-verified across map classes** (Viridian, Pewter, Lavender,
Rock Tunnel, Viridian Forest, Routes 22 + 3); `tst_flag_scenarios` **5/5 in 2.7 s** (was 542 s of
hangs) — `route22-control` healthy on the real base; the suspected rival conflict is
**inconclusive-unconfirmed** (terrain-blocked walk + hidden objects; needs the scripts import).
Everything: [`reference/forged-saves.md`](reference/forged-saves.md). MCP: `emu_make_map_save`,
`emu_boot(map_id=…)`. ⏳ Owed: the proper **default map library** once scripts are imported.

### 🔧 THE PYBOY HANG SOLVED + THE DEV MCP SERVER — the new standing dev transport (2026-07-16)

The "spawning tons of processes and hanging" disaster is **root-caused, reproduced, and fixed at every
layer**. The chain: a **map-id-only forged save is a chimera** (new map's id, old map's Area block —
the linchpin makes Continue trust it) that **hard-crashes ~100 frames in**; the crashed CPU executes
`STOP`, the clocks halt, and **PyBoy's `tick()` never returns** (a core spins forever); killing the
child leaked the interpreter (the venv `python.exe` is a **launcher** — kill the **tree**); and the
time-capped interactive shell spiralled it. **2026-07-15's "forge-onto-any-map PROVEN" is overturned**
— same-map forges (flags/coords/pokes) are safe; the consistent cross-map forge (write the target
map's whole Area block) is a future briefed phase. Full correction:
[`reference/emulator-verification.md`](reference/emulator-verification.md).

**Built on it:** `tst_flag_scenarios` wired into CMake + **verified 5/5 on the real ROM** (one
scenario per process; a wedge is recorded as a **"hang" verdict**, tree-killed, batch continues);
`scripts/emu/forge_save.py` (the shared forge + reseal); `scripts/emu/drive_session.py` (interactive
PyBoy as an owned child REPL); and **the dev MCP server** — `scripts/mcp/` — Twilight's brief: ONE
standardized transport for the whole dev loop (build/test/app-drive/screenshots-returned-inline/
PyBoy install-update-drive/process hygiene), everything a background job with hard tree-kill
timeouts, background-by-default + explicit `app_foreground`. Self-tested end-to-end. Reference:
[`reference/dev-mcp.md`](reference/dev-mcp.md). ⏳ Owed: registering it in Twilight's Claude config
(`scripts/mcp/README.md`), her live pass, and the cross-map forge phase.

**The debug harness got its screws tightened too (0.40.5-alpha):** the "distorted screenshots" were
**mid-transition grabs** — navigation verbs now settle before replying, `shot` waits out transitions
(+ optional `settle` ms); **trap #1 is fixed at the source** (`screen` refuses a duplicate push,
`"already":true`; `title` also returns the screen NAME). `tst_qml_screens` 16/16.

### 🧭 MAP STORAGE panel (gym/safari minigame bytes) — BUILT (2026-07-15, `0.40.4-alpha`)

Twilight briefed a **Map Storage panel** (right dock) for six global-but-map-specific save bytes —
Vermilion trashcan switch 1/2, Cinnabar "next wrong answer", Safari game‑over, Safari balls, steps
left. The accurate model (hers): **global state variables that are clearly map‑specific** — stored once
save‑wide in the global **Main‑Data** block (persistent, not RAM scratch; not a per‑map Area slot),
each belonging to one map, so the panel files each global byte under its owning map. Research + verified
offsets + the big‑endian `wSafariSteps` trap + the `wGymTrashCanIndex`‑is‑a‑decoy trap:
[`reference/gym-safari-state.md`](reference/gym-safari-state.md). ✅ **Console‑verified**
(`scripts/emu/probe_gym_safari_state.py`): **5 of 6 survive Continue** (addresses + big‑endian pinned);
**`wSafariZoneGameOver` zeroed on load** (OverworldLoop → SafariZoneCheck every frame) → shown but
**marked temporary**.

**Built:** `MapStoragePanel.qml` (right dock, ▣ **primary‑filled** rail icon), a map combo listing only
maps with storage (current pre‑selected), three per‑map pages (Vermilion Gym · Cinnabar Gym · combined
Safari Zone), full byte range / hack values shown, each page stating its armed window. Model was already
correct (`WorldLocal`); pinned byte‑exact by **`tst_world`** (`local_writesExactlyItsBytes`,
`local_roundTrip` — 17/17). `world.h` now fully includes `worldlocal.h` (de‑opaque, so QML traverses
`world.local`). `MapRailButton` gained a `primary` (filled‑at‑rest) style. Design: Phase 15 in
[`plans/map-screen.md`](plans/map-screen.md). Green: `tst_qml_screens` 16/16, `tst_world` 17/17.
Panel reviewed live (Vermilion page: values 4/7 = BaseSAV baseline, clean layout). ⏳ **Owed:
Twilight's live pass** (drag/scroll/combo/temp‑flag can't be still‑reviewed).

### 🧱 LAYERS: "Components" → "Tiles"; the two "Warps" told apart, not merged (2026-07-15, `0.40.3-alpha`)

Twilight caught the two-"Warps" confusion. There really are two different things: the tile-meaning
group's **warp tile-trait** (which tile *graphics* are warp-capable on this tileset — doors, stairs,
cave mouths, warp pads; from `warp_tile_ids.asm`, read by `IsWarpTileInFrontOfPlayer` to know a tile
CAN warp) and Game View's **Warps** (the save's warp *list* — coords/destinations, the draggable
objects). So the fix is **disambiguation, not deletion**:

- The group is **renamed "Tiles"** (it's the tileset's tile meanings).
- The warp tile-trait **stays**, relabelled **"Warp tiles"** so it reads apart from the object "Warps".
- **Doors stay** — a door is a passable tile you cross to reach a warp, a real tileset trait.
- **Defaults (Twilight):** every **Game View** layer ON except **Draw area** (player, people, warps,
  signs, screen box); every **Tiles** overlay OFF. Warps show by default as the Game View **object**
  layer (now ON by default — it used to be off only because the warp tile-trait was standing in for it).
  - ⚠️ **AMENDED 2026-07-17 (`0.42.1-alpha`): the Screen box is OFF by default** — *"disable camera
    view box by default … the outline around the player that would be exactly the gameboy screen
    view"*. The map now opens without an outline trailing the player. **Exactly one bit changed
    (`ViewScreenBox`)** — the **Draw area was already off and was NOT touched** (Twilight's own
    earlier call, `3a22f84`; it has never been in the default mask). Pinned by `tst_map_layers` 12/12.
    - 🐺 **A wording bug worth remembering:** the first cut of this said *"both player-riding boxes
      now start off"* — true about the **state**, false about the **change**, and it read as though a
      second layer had been disabled unasked. Twilight caught it. **When describing an edit, describe
      what the edit DID, not what the world now looks like** — the two diverge exactly when something
      was already in the desired state, which is precisely when a false claim is most alarming.

⚠️ **History note:** an intermediate commit (`0.40.2`) briefly *removed* the warp tile-trait; Twilight
corrected that ("it has to come back") once the two-warps distinction was clear. It's back.
`tst_map_layers` 12/12 (nine tile overlays incl. Warp tiles; Game-View defaults pinned), `tst_map`
27/27. The layer-name mapping is verified **not** swapped.

### 🌿 WILD POKÉMON panel — BUILT (2026-07-15, Phase 8, `0.40.1-alpha`)

Twilight briefed the **Wild Pokémon** panel; it's in the map screen's left dock. Two sections (grass,
water), each an **Enable** switch + an **encounter-chance** slider (Low↔High) + the ten slots drawn like
the Pokémon box: fixed **percent** upper-left, **editable level** upper-right, artwork centre, name
below. **Click** a slot → the species picker (Pokémon-details `SelectSpecies`); **drag** → reorder
(re-weights rarity, since a slot's chance is its position). **Enabling a blank table seeds ten random
mons**; **unchecking only disables — it never clears** (re-enabling brings the same ten back).

⚠️ **The research caught a real save-model bug.** The save stores each slot as **`level, species`** but
`AreaPokemonWild` read species-first — inverting every real save's wild table. **Fixed** (level first),
pinned by `tst_area_pokemon::wildTables_byteOrderIsLevelThenSpecies`. Species is the **internal index**
(165 = RATTATA). And it's **live on Continue** — `LoadWildData` rides the same `LoadMapHeader` linchpin
as warps/signs (verified from the disassembly). New: [`reference/wild-encounters.md`](reference/wild-encounters.md),
`WildPokemonPanel.qml` + `WildMonList.qml`, and the `MapModel` encounter surface. Green:
`tst_area_pokemon` 6/6, `tst_map` 27/27, `tst_qml_screens` 16/16. Also this session: the **Door
meaning-layer is off by default** now (Twilight asked; only Warps on) — backend mapping verified **not**
swapped. ⏳ **Owed: Twilight's live pass** (drag-reorder / click-to-pick / inline level edit).

### 🌿 "3-step wild encounter cooldown" checkbox — BUILT (2026-07-15)

Twilight briefed one checkbox for the **map details page**. Research: v2's `AreaPokemon::pauseMons3Steps`
(`0x29D8` bit 0) is `wStatusFlags2` bit 0 = **`BIT_WILD_ENCOUNTER_COOLDOWN`** — the console's **post-battle
cooldown**; set after every battle, and on the next `EnterMap` a set bit loads
`wNumberOfNoRandomBattleStepsLeft = 3`, decremented per step until it self-clears. **Console-verified
DURABLE** (kept on load → no `!`) via `scripts/emu/probe_wild_encounter_cooldown.py` (bit kept + steps-left
3; cleared control → 0). ⚠️ **BaseSAV already carries it set.** Renamed the model property →
**`wildEncounterCooldown`** (+ doc), exposed on `MapModel` (new `AreaPokemon*` ctor arg), placed as a
`MapSwitch` on `DetailsPanel.qml`'s nothing-selected section. Note:
[`reference/wild-encounter-cooldown.md`](reference/wild-encounter-cooldown.md). Green: `tst_area_pokemon`
5/5, `tst_qml_screens` 16/16, `tst_map` 27/27. ⏳ **Owed: Twilight's live pass** (toggling it).

### 🧍 CHARACTER PANEL (`AreaNPC` flags) — BRIEFED + RESEARCHED (2026-07-15, not built)

Twilight briefed the right-hand **Character panel** (the nine map-global NPC/control/battle flags from
v1's "NPC" page). Researched + written up in
[`reference/npc-character-state.md`](reference/npc-character-state.md): all nine are transient
script/battle/link scratch, **three v1 labels are wrong** (the "Scripted Battle" flag is really the
**debug** `BIT_TEST_BATTLE`), the two Sprites bits are **zeroed on load** (console-verified), and v2's
`areanpc.{h,cpp}` carries every wrong name. Design is **Phase 9 (Character State)** in
[`plans/map-screen.md`](plans/map-screen.md), graduated out of §12b, built **9a research ✅ → 9b
model-fix → 9c emulator probe (gates the UI) → 9d panel**. Owed: `scripts/emu/probe_npc_character_state.py`
(not yet written) before any UI ships. Enforcement pinned in
[`context/collaboration.md`](context/collaboration.md).

### 🔗 CONNECTIONS (connecting routes) — BRIEFED + DESIGNED (2026-07-15, not built)

Twilight briefed the four edge connections and answered the design questions; written up as
**Phase 7** in [`plans/map-screen.md`](plans/map-screen.md), graduated out of §12b. The key idea:
a connection has only **two human inputs** (neighbour map + a signed offset −27…+27); the other nine
bytes are **macro-derived** — so editing is offset-driven with auto-derive, raw values shown (editable
on break-sync). Full neighbour map draggable on the canvas; both add-gestures; explicit re-home to
another edge (no rotation — it isn't representable). Human model + save footprint (flags `0x261C` +
four 11-byte blocks) now in [`reference/map-connections.md`](reference/map-connections.md).

**Phase 7a BUILT (2026-07-15, `0.39.1-alpha`) — the model, made true, no UI yet.**
`MapEngine::connectionBytes()` derives the whole 11-byte struct from map + offset via the
cartridge-verified macro; `MapModel` gains the edit API (`addConnection`/`removeConnection`/
`setConnectionOffset`/`setConnectionMap`/`rehomeConnection`/`connectionSynced`/`connectionEditList`),
each writing only the flag bit + its one slot. **`tst_connections` 14/14** — keystone
`savedConnectionsAreInSync` proves the derive reproduces the fixture's real console bytes; siblings
(tst_warps 24, tst_map 27, tst_map_sprites 18) still green.

**Phase 7b PART 1 BUILT (2026-07-15, `0.39.2-alpha`) — add / select / delete / slide on the canvas.**
`ConnectionArrow.qml` (ghostly click-to-add arrow on each empty edge → map picker → `addConnection`) and
`MapConnection.qml` (a selectable/deletable strip you drag along the edge to set the offset, snapping to
corner-aligned / centred / flush). `connectionEditList` gained neighbour size + `snaps`.
**`tst_qml_screens` 16/16**, tst_connections 14/14. ⏳ **Owed:** the full draggable neighbour-map render,
the drag-a-map-onto-arrow gesture, the sync-aware raw handles (7d), and
**Twilight's live pass** (drag/select/snap can't be still-reviewed). Turn on the **Connections** layer
to see arrows + strips.

**Phase 7c BUILT (2026-07-15, `0.39.3-alpha`) — the connection inspector.** Selecting a connection
shows: neighbour picker, offset spinner + snap buttons (corner/centred/flush), re-home buttons, a
**Break sync** switch unlocking the raw nine (`connectionFields`/`setConnectionField`), Delete, and the
live/restored honest note. `tst_connections` **16/16** (two new for the fields + break-sync write),
`tst_qml_screens` 16/16.

**Phase 7b PART 2 BUILT (2026-07-15, `0.39.4-alpha`) — the full neighbour map on the canvas.** Each
connection renders its actual neighbour map (its own tileset) bleeding off the edge, aligned to the
offset, drawn behind our opaque buffer so only the off-edge part shows, dimmed 45%, re-positioned live on
a drag. `connectionEditList` gained `toTileset`. ⏳ Still owed in Phase 7: the drag-map-onto-arrow
gesture, the on-canvas resize nodes (7d), and **Twilight's live pass** (the alignment + all the drag
interactions can't be still-captured — layer-gated, edge-located, zoom-dependent).

### 🧍 THE PLAYER details panel — BUILT (2026-07-14, `0.39.0-alpha`)

Twilight briefed **fleshing out the full player details panel** — all 26 `AreaPlayer` map-state bytes
(the v1 `area-player` field set: Moving, Last Stop, Current direction, X/Y + block coords, Jumping Y,
Strength, Fly, Surf, the battle four, the special-warp offsets, the standing-on trio, Walk/Bike/Surf,
End-edge jump, Spin, Card Key, Link Cable). Research **console-verified byte-by-byte**:
[`reference/player-state.md`](reference/player-state.md). Design + build:
[`plans/map-screen.md`](plans/map-screen.md) → **Phase 5f**.

**Built, same session.** Select the player → the Details panel edits every one of his map-state bytes,
grouped and named in English, full-range/hack-values-included. The ten rewritten-on-load + three dead
are gathered in a **"Rewritten on load, or never read"** group behind the *Reloaded values* switch,
each wearing an amber **!** (with its own reason) or a grey **💀**. New: `MapModel::playerFields()`/
`setPlayerField()`, `PlayerField.qml`, and **`tst_player`** (7 cases; keystone byte-diffs the whole
32 KB across every field → exactly one byte/bit moves). `tst_qml_screens` 16/16.

⏳ **Owed: Twilight's live pass** — scrolling the panel, toggling *Reloaded values* to reveal the
rewrite group, and the enum/flag controls are things the offscreen still-PNG review can only partly
cover. (Screenshots captured: `tmp/player-panel-default.png`.)

The headline, and the answer to *"which are rewritten on startup"*: **ten of the 26 are.**

- ⚠️ **`wPlayerDirection` (Current direction) is FORCED to DOWN on every Continue** — `MainMenu`
  `.pressedA` writes `PLAYER_DIR_DOWN` before entering the map. You always wake up facing down.
- ⚠️ **`wStatusFlags3` is zeroed whole** — so `isBattle`/`isTrainerBattle` (really
  `BIT_TALKED_TO_TRAINER` / `BIT_PRINT_END_BATTLE_TEXT`) die on load, same wipe as the warp/NPC bits.
- ⚠️ **`strengthOutsideBattle` is reset** on an ordinary Continue — **but survives if the battle-over
  bit is set** (a real `EnterMap` interaction the probe caught with a two-save test).
- ⚠️ **`battleEndedOrBlackout`, `usingLinkCable`, `standingOnDoor`, `movingThroughDoor`,
  `finalLedgeJumping`, and `Jumping Y`** are cleared/zeroed on entry too.
- 💀 **Three are dead:** `x/yOffsetSinceLastSpecialWarp` (`; they don't seem to be used`) and
  `usedCardKey` = `BIT_UNUSED_CARD_KEY` (setter is `; never checked`).
- The offsets/bits in the v2 model are **all correct**; five fields are **misnamed** (fixed in 5f-0).

⚠️ **The probe earned its keep:** `wMovementFlags` clears bits **per-routine, not whole-byte** — bits
0/1/6 are cleared on load while 2/7 are kept. A read of the asm alone gets this wrong (the sprite-pass
mistake). Tool: `scripts/emu/probe_player_state.py` (local-only, ROM-gated).

### 🗺️ The "Map" page fields (Area State) — BUILT (2026-07-15, Phase 8)

v1's "Map" page fields now live in the **"Map state"** section of the map-details panel (shown when
nothing is selected). Research + **console-verified**:
[`reference/area-map-state.md`](reference/area-map-state.md) (`scripts/emu/probe_area_map_state.py`).

**Built:** `MapModel` gained the area-state API and `DetailsPanel.qml` the section: **Current script
step** (descriptive ComboBox from `MapDBEntry::getScriptSteps`, parsed from `maps.json`'s
`scriptEntries`, + "Something else…" raw), **Run this script step on load** (`BIT_USE_CUR_MAP_SCRIPT`),
**Always on bike** (`BIT_ALWAYS_ON_BIKE`), the **Camera / view box** (`wCurrentTileBlockMapViewPointer`
— derived, **synced to the player by default**; `movePlayer` keeps it synced; a **break-loose** switch
+ raw-address entry for the power path), and the **reset-on-load** scratch (`vramViewPtr`→$9800,
`cardKeyDoorX/Y`→0) behind the "Reloaded values" switch with an amber `!`. Every setter writes exactly
its byte(s); `areamap.{h,cpp}` docs corrected (names kept). New DB parse: `MapScriptStep` +
`getScriptSteps`. Pinned by **`tst_area_state`** (6 cases; keystone byte-diffs every setter + the
view-box sync doctrine). `tst_qml_screens` 16/16, `tst_map` 27/27, `tst_player` 7/7. ⏳ **Owed:** the
on-canvas **drag** of a broken-loose view box (canvas interaction), and Twilight's live pass.

### 🌿 The "Map" page — the earlier research state (kept for context)

Twilight brought v1's "Map" page (under "Area") into the map-details panel and asked what the seven
leftover fields do and **which are rewritten at boot**. Console-verified in
[`reference/area-map-state.md`](reference/area-map-state.md).

The answer to *"which are rewritten at boot"*:

- **Durable, editable (2):** `wCurMapScript` (Current Script — this map's story-script step) and
  `BIT_ALWAYS_ON_BIKE` (force bike ride — the Cycling-Road lock). Both **kept**.
- **Derived, TRUSTED on load (1):** `wCurrentTileBlockMapViewPointer` (the camera / view box).
  ⚠️ Predicted "recomputed"; **wrong** — its recompute is off the Continue path, so the save's
  value is trusted: writing `0xFFFF` made the cartridge **draw garbage** (`tmp/emu-areamap/areamap.png`).
  Doctrine (clarified by Twilight): **synced to the coords by default**, with a **break-sync**
  toggle + alert-on-manual-entry + **canvas dragging** of the view box for power users.
- **Rewritten/reset every load (2):** `wMapViewVRAMPointer` (→ `$9800`) and `wCardKeyDoorX/Y` (→ 0).
- **Auto-trigger lever (1):** `BIT_USE_CUR_MAP_SCRIPT` — ⚠️ predicted "cleared"; **survives on a
  quiet map**, so a save can carry it set to **auto-run a chosen map script on load** (a feature,
  not plumbing). Consumed on the first tick of a scripted/trainer map.
- **Ghost (1):** "to blackout dest" = `BIT_ESCAPE_WARP`, **already moved to `AreaWarps::escapeWarp`
  (2026-07-14)** — do not re-add it here.

No save-corruption bug and no loaded gun; it's a naming/organisation fix plus the derived-value
sync discipline. **Design direction (Twilight):** derived values (the view box) **kept in sync by
default** with a power-user break-sync path; scripts = descriptive ComboBox + "Something else…";
pointer fields intuitively selectable, **never an address** except behind "Something else…".

**Data landed for the script combo (2026-07-15):** `scripts/import_map_scripts.py` imported each
map's `SCRIPT_<MAP>_<NAME>` steps into a new **`scriptEntries`** field in `maps.json` (additive,
116 maps / 458 steps, `--check`-idempotent) — the descriptive list the "Current Script" ComboBox
will read. Per-map progress (`w<Map>CurScript`) is out of scope (Twilight: current scripts only).

**CI fix (2026-07-15):** `dev` was red on a missing CI Qt module, not a test failure —
`projects/CMakeLists.txt` requires Qt `ShaderTools` (genuinely: `app/CMakeLists.txt` `qt_add_shaders`
compiles `shaders/pixelart.frag`), but the CI's Qt install lacked it, so `find_package` failed at
configure. Fix: added `qtshadertools` to the CI Qt modules in `{tests,lint,release}.yml`.
`release.yml`/`pages.yml` are already `main`-only, so the release/deploy pipeline never ran on `dev`.

### 🚪 WARPS — phase 5 is COMPLETE (2026-07-14, `0.36.0-alpha`)

| | | |
|---|---|---|
| **5a** | The model, made true | ✅ seven bugs fixed + **negative-controlled**; the legal-value tables modelled; `wLastMap`/`wLastBlackoutMap` surfaced |
| **5b** | Doors on the canvas | ✅ select · drag (**exactly two bytes**) · ✕ · ✎; a live "→ where it goes" label; the Doors layer |
| **5c** | The toolbar becomes TOOLS | ✅ `⇄+ Place warp`, `🧍+ Place sprite` (random, but **only from the map's own loaded pictures**), and `[ Outside is: … ]` |
| **5d** | The Warp state panel (**right dock**) | ✅ every byte named in English; the guns offer legal values, full range one click away; the dead/wiped four behind the switch |
| **5e** | The player | ✅ position, in the Details panel |

New test: **`tst_warps`** (24 cases). Keystone: drag a door across town and byte-diff the whole 32 KB —
**exactly `x` and `y` moved**. Full `ctest` **85/85**.

⏳ **Owed: Twilight's live pass** — the drag, the drop, the delete, the maker tools and the pickers are
all things a still PNG cannot review.

### 🐺 …and the screenshot review caught a cry-wolf that would have shipped (2026-07-14)

The fixture save — an **ordinary** one — holds `dungeonWarpDestMap = 194` and `whichDungeonWarp = 0`.
That pair is not in `DungeonWarpList`, so the first cut lit a **red !** on it.

But **0 is the resting value**: `IsPlayerOnDungeonWarp` writes it as its *first instruction* whenever
you are not standing on a hole. **Every save ever made carries one**, `BIT_DUNGEON_WARP` is off, and the
console never looks. The warning was true and **useless** — it would have fired on every file anybody
ever opened. Exactly the mistake the sprite *"your cast has changed"* notice made in its first cut.

> **The rule now:** `legal` (is it in the table?) and **`armed`** (will the console read it?) are **two
> different questions**, and the red **!** fires only on **both**. Out-of-table-but-inert gets a quiet
> grey line. And the map and the hole are judged **separately** — the first cut failed both whenever the
> *pair* was wrong, so a perfectly good hole map came up flagged because of the 0 beside it.

Pinned by `tst_warps::guns_dontCryWolfOnAnOrdinarySave`.

### 🪧 SIGNS — phase 6 is COMPLETE (2026-07-14, `0.38.0-alpha`)

| | | |
|---|---|---|
| **6a** | The text, from the cartridge | ✅ `import_sign_text.py` pulls the real words out of `pret/pokered` into `maps.json` (**additive-only**); `MapDBEntryText` + `MapDBEntry::getTextEntries()` |
| **6b** | Signs on the canvas | ✅ orange `▤` chips on a **Signs** layer; select · drag (**exactly two bytes**) · ✕ · ✎; the selected sign shows its words |
| **6c** | The Place sign tool | ✅ `▤` in the makers rail (shortcut **S**), 16-cap up front, lights its layer |
| **6d** | The Details panel | ✅ one X/Y control + the grouped **Says…** picker (Signs / People / Other, real words); hack ids shown-not-refused; Delete; the live-on-Continue note |

New test: **`tst_signs`** (15 cases). Keystone: drag a sign and byte-diff the whole 32 KB —
**exactly its Y and X moved**; plus `everyShippedSignResolvesInItsMapsText`. Research:
[`reference/signs.md`](reference/signs.md); design: [`plans/map-screen.md`](plans/map-screen.md) → Phase 6.

- ✅ **The save model was already correct** — the rare pass with no model bug to fix first (contrast
  sprites: 4, warps: 7). `AreaSign` (`wNumSigns` `0x275C`, cap **16** = `MAX_BG_EVENTS`) + `SignData`
  (`wSignCoords` Y,X `0x275D`; `wSignTextIDs` `0x277D`) match the cartridge byte-for-byte.
- 🗣️ **The words came from `pret/pokered`.** A sign's `text` is a **1-based index into the map's
  `def_text_pointers` table**; `maps.json` shipped only the id, so 6a extracted the strings and
  **grouped** each id by whether it's a sign / a person / script-only.
- A sign rides the **same persistence linchpin as a warp** (`.loadSignData` inside `LoadMapHeader`,
  behind `BIT_NO_PREVIOUS_MAP`): live on Continue, restored on re-entry — the panel says so.

⏳ **Owed: Twilight's live pass** — the drag, the delete, the tool and the grouped picker are things a
still PNG can only partly review.

### 🚪 WARPS — what the research found (2026-07-14)

Everything: [`reference/warps.md`](reference/warps.md). Design:
[`plans/map-screen.md`](plans/map-screen.md) → **Phase 5**.

**The linchpin (and it is why a warp editor is possible at all).** `LoadMapHeader` rebuilds the warp
list from ROM on every map load, with **no escape-hatch bit** — but `LoadMainData` **sets
`BIT_NO_PREVIOUS_MAP` on the saved tileset byte** as it reads the save, which makes the next
`LoadMapHeader` **`ret nz` before it copies anything**. So **an edited warp is genuinely live on
Continue** — console-verified, including a **4th warp invented in a 3-warp town** — and the game
restores the map's original doors when the player leaves and walks back in. The sprite rule, reached
by a different route, and the screen must say it out loud.

⚠️ **`wStatusFlags3` is ZEROED on every save load — the whole byte.** It shares an address with
`wCableClubDestinationMap`, and `SpecialEnterMap` (on the Continue path) writes `0` to it. Verified:
wrote `$FF`, console read back `$00`. So `scriptedWarp` + `isDungeonWarp` — **and `npcsFaceAway`,
`tradeCenterSpritesFaced`, `isBattle` (really `BIT_TALKED_TO_TRAINER`) and `isTrainerBattle` (really
`BIT_PRINT_END_BATTLE_TEXT`)** — can never survive a save. Every other warp byte came back exactly as
written.

💀 **`wWarpedFromWhichWarp` / `wWarpedFromWhichMap` are DEAD.** The game writes them on every warp and
**nothing anywhere reads them**. Two writes, zero reads. A wiped byte and an unread byte are different
facts and the panel must say which is which.

⭐ **The two bytes that matter weren't on the warps screen at all:** `wLastMap` (`0x2611`) — *the map a
`$FF` door returns you to*, i.e. every building's exit — and `wLastBlackoutMap` (`0x29C5`) — where
blacking out / **Dig** / **Escape Rope** put you. Both **already modelled** in `WorldGeneral`; they just
need surfacing. `wLastMap` goes in the **toolbar**, in words (*"Outside is: Pallet Town"*), because
changing it re-labels every `$FF` door on the map at once.

🔫 **Two loaded guns, and `AreaWarps::setTo()`/`randomize()` load them today.** `wDestinationMap` (Fly)
has **13** legal values in an unterminated, unbounded table; the dungeon (map, hole) pair has **12**, and
hole numbers are **1-based**. `setTo()` currently picks *any* cave, *any* map, and a 0-based hole index —
out-of-table values that make a real console read arbitrary ROM as warp data. **Dormant only until the
`MapsDB` deep-link landmine is defused** (see Open issues), exactly like `AreaAudio::setTo()` was.

Tool: `scripts/emu/probe_warp_persistence.py` (local-only, ROM-gated).

### ⛔ NOT YET BRIEFED — don't design or build these (2026-07-14, Twilight)

> *"Signs and stuff, connecting routes, wild Pokémon — these are examples of things I haven't gotten to
> yet. I'd hate to have to undo a lot of work because it was done before I explained anything."*

**Area state (the AreaWarps/AreaLoadedSprites leftovers) · the tileset deep pass** are **un-briefed**.
(Signs, connections, wild Pokémon/encounters and character state have since been briefed + built.) The sketches for them in `plans/map-screen.md` were written from the *save
layout* — a map of what bytes exist, not of what Twilight wants a person to be able to *do* — and they
**carry no authority**. Each gets its own conversation, then research, then a design, then code.

⚠️ **Adjacency is not a brief.** Signs load out of the *same ROM block* as warps and are the *same shape*
— and on exactly that logic they nearly rode into the warps phase. They were **cut** (2026-07-12 sketch →
removed 2026-07-14). A briefed feature that needs an un-briefed one **reads** it; it does not build a UI
for it. Full list + the rule: [`plans/map-screen.md`](plans/map-screen.md) → **§12b**, and `CLAUDE.md` →
*Critical Things Not to Get Wrong*.

### 🚶 The NPCs walk the way the GAME walks (2026-07-13, `0.30.0-alpha`)

`MapSim` is now **`UpdateNPCSprite`, transliterated** — the console's per-frame state machine out of
`engine/overworld/movement.asm`, instruction for instruction, **bugs included**, ticking at the DMG's
own **59.7275 Hz** (every counter in it is measured in Game Boy frames). Full write-up with the assembly
beside it: [`reference/npc-movement.md`](reference/npc-movement.md).

Three things it taught us, and none of them were guessable:

1. ⚠️ **A `STAY` sprite is not standing still — it is TURNING.** It runs the whole random-direction path,
   and `TryWalking` writes the new facing *before* `CanWalkOntoTile` refuses the step. Oak picks a
   direction, turns to face it, fails to move, and sets a new delay — about once a second, forever. Our
   first sim just *skipped* `STAY` sprites, which is a still picture, not a simulation.
2. ⚠️ **The game only animates the people NEAR THE PLAYER.** The screen-bounds check is measured
   *relative to him*, so Pallet's Fisherman — eight tiles below Red — takes no step until Red walks
   towards him. The test found this by failing on him.
3. ⚠️ **The wander limit is BUGGED and we copy the bug.** Walk up five times and `yDisplacement` hits 3;
   now `3 + 1 = 4 < 5` and the sprite **can never walk down again**. The disassembly flags it itself.

**And every "animation scratch" byte is now live state** — `movementStatus`, `movementDelay`,
`walkAnimationCounter`, the two frame counters, the step vectors, `yDisp`/`xDisp`. They are not scratch.
They are the simulation.

### 📐 STANDING RULE: use the GAME's file formats (2026-07-13)

Where `pret/pokered` has a format, **we use that format** — by default, without asking. `.blk` already is
theirs. **Music becomes their own `.asm` sheet music, parsed line by line** (⏳ owed): it is line-based
assembly with macro names, so a *line parser* turns it straight into sound-engine commands — we never
needed a byte compiler. See [`reference/file-formats.md`](reference/file-formats.md) and `CLAUDE.md`.

### ⏳ Owed from Twilight's 2026-07-13 review (captured, not yet done)

- **The sprite Details panel is bad and gets rebuilt.** Raw values, cryptic, cramped, "Who/Where/When" is
  dumb. Group X+Y into one control; picture = a *picker* with the artwork; hide fields the combo makes
  irrelevant; movement status is not a number; "delay until next move" must be *shown*, not explained in
  prose. **Text id / item / trainer class / trainer team must resolve to REAL map data** (extract it if we
  haven't got it). `✕` becomes a **Delete** button. A **?** icon in the panel title (the one allowed
  tooltip-icon), and a **yellow !** on anything the game rebuilds on load.
- **Music**: a real grouped **ComboBox** like the map picker (not a hand-rolled list), **volume slider
  below it**, and **hover must not change the track** — only selection does.
- **Canvas/panels**: player drag is glitchy; drag-and-drop *still* not working; drop the "cast no longer
  matches" notice; the map must not resize when panels open; Layers "Clear" belongs in the panel title
  (+ one per category) and must look like a clear button; delete "none here"; **"Meaning" → "Components"**;
  the Sprite Set and Strength panels' walls of text go; the scrollbar-overlap problem (it is in the notes).

### 🧍 SPRITES — phase 4 is COMPLETE (2026-07-13, `0.27.0-alpha`)

| | | |
|---|---|---|
| **4a** | The data model, made true | ✅ the four bugs fixed + **negative-controlled**, the five unread fields modelled, `sprites.json` grouped |
| **4b** | NPCs on the canvas | ✅ all 72 sprites **imported and drawn**; *People & objects* layer; **select, drag-to-move, ✕ delete, ✎ edit**; the ground is **no longer selectable** |
| **4c** | The Characters bar | ✅ 72 characters on 5 shelves + filter; **drag in to place, drag out to delete**; collapses to a strip |
| **4d** | The Details panel | ✅ edits what's selected; **the map's own details when nothing is**; every sprite byte named, explained, full-range, hack values flagged not refused |

**The artwork blocker is gone.** The repo carried exactly *one* sprite sheet (`red.png`); every town was
empty. `scripts/import_sprites.py` pulls **all 72** out of `pret/pokered` into one atlas
(`assets/sprites/overworld.png`) + a generated `spriteart.h`. It is self-validating, and it **found a
third art shape by itself**: `16x48` — the **twenty "still" people** (nurses, guards, Mom, the Gameboy
Kid) who have **no walking art at all**, because the game never drew them a step.

⚠️ **A sprite whose picture this map has NOT loaded is outlined in amber.** That is the one the console
draws as garbage. Shown, never silently corrected.

⚠️ **The "you changed the cast" warning tracks the EDIT, not a diff against the ROM** — and the first
version got that wrong. **A real save's cast already differs from the cartridge's**, because walking
NPCs wander: Pallet's Girl is at (3, 8) in the ROM and (3, 6) in `BaseSAV`, having taken a couple of
steps. Diffing would have cried wolf on every save ever opened. Pinned by
`tst_map_sprites::npcsEdited_isQuietUntilYouActuallyChangeSomething`.

New test: **`tst_map_sprites`** (10 cases). Its keystone is `moveNpc_writesExactlyTwoBytes` — drag a
person across town and byte-diff the whole 32 KB save: **exactly `mapX` and `mapY` moved, nothing else.**

⏳ **Owed: Twilight's live pass** on the drag, the drop, the delete and the panel — a still PNG can
review none of those.

### 🧍 SPRITES — the research: the model was WRONG in four places (2026-07-13)

The next phase of the map screen is **sprites**, and the research pass found that our sprite model — a
straight port of v1's — has been **writing the opposite of what it says** into saves. Nothing is built yet;
**phase 4a exists to fix this before anything is built on it.**

| | v2 has | the truth |
|---|---|---|
| `SpriteMobility` | `Moving = 0xFF`, `NotMoving = 0xFE` | **inverted** — `STAY = $FF`, `WALK = $FE` (console: Oak reads `$FF`) |
| `load(MapDBEntrySprite*)` | `"Stay"` → `0xFE` | writes **WALK for a STAY sprite** — every `setTo()`/randomize |
| `SpriteGrass` | `InGrass = 0x00` | **inverted** — `$80` = in grass. `reset()` flags every blank sprite as in grass |
| `face` / `range` | two fields | **one byte** (movement byte 2). `face` is being written into `faceDir` — a different field in a different table |

Not modelled at all: StateData1 `a`/`b`/`c`, StateData2 `9`/`d`, and **`wToggleableObjectFlags`**
(`0x28A0`, 32 bytes) — the flags that actually decide whether a missable NPC appears. (We model only the
per-map *list* at `0x287A`, which the game rebuilds from ROM every map load — it does nothing.)

⚠️ **And the cartridge overruled a careful reading of the source.** `LoadMapHeader` appears to zero and
rebuild all sprite state from ROM on every map load — so I concluded sprite edits were being thrown away.
**They are not.** `scripts/emu/probe_sprite_persistence.py` boots the real ROM: a re-pictured, moved sprite
**and a fourth NPC invented in a three-NPC town** all survive Continue, because `MainMenu` sets
`BIT_CUR_MAP_LOADED_1` and the map header is never re-read on that path. The true statement — **which the
screen must say out loud** — is that an edited sprite is really there, and **the game restores the map's
original cast when the player leaves and re-enters**.

Everything: [`reference/sprites.md`](reference/sprites.md) → Parts 3, 5, 6.
The design: [`plans/map-screen.md`](plans/map-screen.md) → **Phase 4**, rewritten into **4a** (make the
model true) · **4b** (NPCs drawn, selectable, draggable; background squares **stop** being selectable) ·
**4c** (the Characters bar — drag in to add, drag out to delete) · **4d** (the **Details panel**, left side,
showing the map's own details when nothing is selected).

### 🗺️ The Map screen rebuild — phases 0–3 are IN (2026-07-12)

| Phase | | Status |
|---|---|---|
| **0** | Unblock the bridge | ✅ shipped — nine opaque Area children opened up; the Q_INVOKABLE returns (`WarpData*` …) registered at last; `MapsDB` deep-linked at boot |
| **1** | The chassis | ✅ shipped — identity bar · tool rail · context bar · dark canvas well · **collapsing icon dock (one panel, never stacked)** · status bar. The eviction queue and the chip bar are deleted |
| **2** | The layer system | ✅ shipped — **Guides / Meaning / Game View**, tri-state group eyes, alt-click solo, folding; **the player and the red + grey boxes are layers now**; `tst_map_layers` byte-diffs the save across every toggle |
| **3** | The map is ALIVE | ✅ shipped — the water's **rotation** and the flower's `1,1,2,3` at the console's **20/21-frame cadence**, a ▶/⏸/step transport, and **frame 0 when headless** so no test flaps. Found + fixed two long-standing inventions (see below) |
| **4–13** | objects · inspector · encounters · area state · tileset/blocks · tools · polish · verification · notes · *(optional)* walk-the-map | ⏳ next |

⚠️ **The water and the flowers were WRONG for years.** `TilesetEngine` ran the water `0,1,2,3,4,3,2,1`
(the console swings **−1..+3**, not 0..+4) and the flower `2,3,1,1` (the console runs **`1,1,2,3`** —
flower1 shows for twice as long). Both plausible, both invented, both now read out of
`UpdateMovingBgTiles` and pinned by `tst_map_animation`.
⏳ **Owed:** `tst_emu_parity` does not yet dump VRAM tiles `$14`/`$03` frame-by-frame — the animation
is verified against the *disassembly*, not yet against the *silicon*. See
[`reference/map-animation.md`](reference/map-animation.md).

### 🗺️ The plan of record — read it before touching the map screen (2026-07-12)

The map grew organs fast (blocks, tiles, meaning, palettes, the player, connections, music) and every one
was bolted onto the same screen. Twilight's verdict — *"UX is one of the highest priorities and right now
it's really bad"* — and she is right: three bars of unrelated chrome, panels that stack sideways and
**evict each other**, no real layer system, nothing editable on the canvas, and **most of the Area block
has no UI at all**.

The complete overhaul is **designed and approved**: [`plans/map-screen.md`](plans/map-screen.md). The
chassis (identity bar · tool rail · context bar · dark canvas well · **collapsing icon dock, one panel at a
time, never stacked** · status bar), a **4-group layer tree** (Guides / Meaning / Game View / Objects — the
red screen box, the accent draw area and the player are **layers**), **on-canvas object editing** (drag,
select, add, delete warps/signs/NPCs), **every byte of the Area block editable** (hack values included,
shown never rewritten), and **frame-accurate animation** ([`reference/map-animation.md`](reference/map-animation.md)).
**Thirteen phases, plus one optional** — each a full pass, finished before the next begins.

⚠️ **Two blockers sit in front of ALL of it** (phase 0, both already diagnosed):
1. `area.h` `Q_DECLARE_OPAQUE_POINTER`s **nine of its eleven children** → QML reads `area.map.*`,
   `area.warps.*`, `area.player.*` as **`undefined`**. Same bug that bit `AreaAudio`. Fix: full `#include`.
2. `DB::deepLinkAll()` never calls `MapsDB::inst()->deepLink()` (the latent landmine below).

⚠️ **And the map does not animate.** `UpdateMovingBgTiles` rotates water tile `$14`'s bytes every 20 frames
and cycles three flower tiles every 21 — we draw **frame 0, forever**. The water is dead; the console's
water is not. That is a correctness bug, not a missing garnish.

**New territory: the MAP (2026-07-12).** The app has fully resurfaced from the revival, and the first
new ground is the biggest one. The old Maps screen (a greyed-out tile → a menu of dead ends) is
**deleted**, and in its place is a **map emulator**, step 1 of several.

It rebuilds what the Game Boy rebuilds: the map ringed by its **3-block border**, the **6×5-block
scratch area** the game redraws, and the **20×18-tile screen** sliding around inside that in half-block
steps — all drawn from `.blk`/`.bst` data imported **verbatim** from `pret/pokered`, at one screen pixel
per Game Boy pixel. It is an emulation, not an impression, and it proves it: the view pointer the game
itself computed and left in the save (`0x260B`) is **reproduced byte-for-byte** from just the player's
coords and the map width, on both real saves (`tst_map::viewPointer_matchesWhatTheGameStored`). If that
ever fails, our model of the game is wrong — read it first.

Pieces: `BlocksDB` (db) → `MapEngine` + `MapProvider` (app) → `MapModel` = `brg.map` → `Map.qml`.
Domain write-up: [`reference/gen1-knowledge.md`](reference/gen1-knowledge.md) → "VERIFIED from the
disassembly". Import: `scripts/import_map_blocks.ps1` (self-validating, `-Check`).

**Every one of the 248 map ids renders** — including the glitch and half-baked ones, which are not empty
maps but *unfinished copies*: `maps.json`'s own `incomplete` field says which map of, in exact agreement
with the ROM, so we follow it and draw the map they copy (what a Game Boy actually does with those ids).
Nothing invented, no JSON changed.

**Not yet drawn:** the **NPCs** (the other 15 sprite slots), the grass-priority bit, warps/signs overlays, and
tile animation frames (frame 0 only). **Connection strips, the palettes/"contrast" and the PLAYER are DONE**
(below).

### ✅ 🗺️ The map now says what it MEANS — and it caught a save-corrupting bug (2026-07-12, 0.24.0-alpha)

The map could draw Pallet Town; it could not tell you a thing about it. A wall and a floor are just two
pictures. Now the **meaning layer** is in: **Show** chips over the map light up **Walls, Grass, Water,
Warps, Doors, Ledges (with the arrow you jump), Counters, Elevation edges, Cut trees** and the **border
ring** — each a tint *and* its own 8×8 pattern, so several stack and still read apart, and so they
survive the glitch palettes. **Off by default** (the map is the point), and a chip with nothing to show
says so rather than switching on an empty overlay. Rendered as **one image in C++** — Route 17 is 20,000+
tiles and a per-tile QML delegate would crawl.

**Click a block** → the **Blocks panel** draws it big with all 16 tiles, each labelled with what it *does*
("Grass — wild Pokémon", "Ledge — jump down"). Hover a tile there, it lights up on the map.

Domain write-up: [`reference/tiles.md`](reference/tiles.md). Import: `scripts/import_tile_traits.py`
(self-validating; re-reads every collision list **out of the cartridge** and demands a byte-for-byte
match). New DB: `TileTraitsDB`. Pinned by `tst_tile_traits` (10 cases).

#### 🔑 There is no Indoor/Cave/Outdoor byte — there's an ANIMATION byte, and it was a real bug

The tileset header's last byte is `TILEANIM_NONE / WATER / WATER_FLOWER`, saved at `0x3522`
(`sTileAnimations`, the byte before the checksum) — what `AreaTileset` calls `type`. **`tileset.json`'s
Indoor/Cave/Outdoor is a verified 1:1 rename of exactly that** (all 24 checked against the cartridge's
header table).

`Settings::previewOutdoor` was a **bool**, so it collapsed **Cave into Indoor** — every cave in the game
rendered with **dead, motionless water** when the console animates it. Now tri-state everywhere
(`previewTilesetType`), and each option says what it *does*.

#### 🐞 THREE tilesets pointed at the WRONG collision list — and it was writing that into saves

`tileset.json`'s `collPtr` was wrong for **Mart (→5971)**, **Forest (→5989)** and **Reds House 2
(→5961)**. The collision lists are **shared** between tilesets in the ROM (Red's House 1 *and* 2 are one
list; Mart *and* Poké Center are one list), and the v1 importer assumed one list each in index order — so
the chain slipped by one.

**This had teeth:** `AreaTileset::loadFromData()` writes `collPtr` into the save, so "put the player in a
Poké Mart" wrote **Red's-house collision** into it. Fixed with Twilight's go-ahead (3 lines), verified
against the cartridge, and pinned by `tst_tile_traits::derivedCollPtrs_matchTilesetJson` +
`martAndRedsHouse_areNotTheSameList` — which were **negative-controlled**: put the bug back and they fail
by name with the exact reason.

⏳ **Owed: Twilight's live pass** on the chips, the click-to-inspect, and the two new panels.

### ✅ And now the actual Game Boy checks our work (2026-07-12)

`tst_emu_parity` boots the **real ROM** in an emulator with one of our saves, reads the **console's own
RAM**, and demands `MapEngine` match it byte for byte. The verdict: the view pointer, the map's blocks, the
24×20 scratch area and the **20×18 screen tiles all MATCH** — `wTileMap` matching means the entire view
pipeline is right, with no sprites or palettes in the way.

It also immediately caught the one thing that *was* wrong — **the border ring** — which is now **fixed and
verified**: see below.

### ✅ 🎵 THE EDITOR PLAYS THE GAME'S MUSIC (2026-07-12, 0.22.0-alpha)

There is not an audio file in the repo. **`pse-audio` is a Game Boy sound chip** (`GbApu` — two squares, a
wave table, a noise LFSR, the 512 Hz frame sequencer) with **Pokémon Red's own sequencer running on top of
it** (`Gen1SoundEngine` — `engine_1.asm`, transliterated over a 256-byte state laid out exactly like the
console's `$C000`). The music data is **imported from `pret/pokered` and verified byte-for-byte against the
cartridge** (38 KB for the whole soundtrack).

On the **Map screen**, behind a **♪** toggle: the map's music (its own is selected on open), the two save
flags — **No Audio Fadeout** and **Prevent Music Change** — a **▶**, and **hover-preview**: with the music
playing, run the mouse down the list and it changes as you go. **Hover auditions; click commits** — moving a
mouse never touches the save, and a line says plainly when what you hear isn't what's stored.

⚠️ The **bank** picker offers only 2 / 8 / 31, because we measured what a real console does with anything
else: it **stops producing frames**. A save holding a bad bank is **shown, never silently rewritten**.

**And the console says it's right.** `tst_sound_parity` boots the real cartridge, photographs the engine's
entire mind (`$C000–$C0FF`) **every frame**, seeds our port from the console's own state, and demands it
reproduce every frame after — **all 46 tracks plus an inner voice, byte-for-byte: 48/48**. It found three
real bugs in the port (the pitch-slide routine **clobbers `de`**, so a slide note starts on the divide's
leftovers; `PlaySound` never restores `wSoundID` after a drum; `wSfxHeaderPointer` was never written) and
one in the test rig itself (photographing the console **mid-fade**, while `wAudioROMBank` was still the
*old* bank, produced dumps that looked plausible and were nonsense). A negative control was run: break the
note length by one and it fails on frame 7 with the exact byte.

Green: `tst_sound_parity` 48/48, `tst_audio` 10/10, `tst_qml_screens` 16/16, full `ctest` **78/78**. The
whole story: [`plans/music.md`](plans/music.md).

⏳ **Owed: Twilight's live pass.** It is sound and it is hover — a still PNG can review neither.

### 🎵 Music — the research (2026-07-12)

The next organ of the map emulator is **sound**, and it all lands on the **Map screen**: the two save flags
(**No Audio Fadeout**, **Prevent Music Change**), the map's **music track** picker, and — the real ask —
**actually playing the music**, accurately, with a ▶ per track and **hover-to-preview**.

Decided with Twilight: the music data is **imported from `pret/pokered`** (the same precedent as the map
blocks — it ships, no ROM needed), and playback is a **full port** — the real Gen 1 sequencer driving a real
**DMG APU** model in C++ (`pse-audio`), not an impression. Nothing is built yet.

The research is done and written down:
[`reference/gameboy-apu.md`](reference/gameboy-apu.md) (the chip),
[`reference/gen1-sound-engine.md`](reference/gen1-sound-engine.md) (the game's sequencer + the two flags,
**verified against the disassembly**), and the six-phase build in [`plans/music.md`](plans/music.md).

The keystone, when it's built: the engine's **entire state is 243 bytes at `$C000`**, so PyBoy can dump it
from the real cartridge every frame and our port must match **byte-for-byte, frame by frame**. Same doctrine
as the map — the console is the judge.

### 🎼 …and there is no such thing as glitch music. There are 105 extra songs. (2026-07-12)

A music header is **3 bytes per channel**, and ids are computed *by address* (`SFX_Headers_N + id × 3`) — so
a 3-channel song **eats three ids**, and the spare two parse as perfectly valid **one-channel headers
pointing at that song's channel 2 and channel 3**. **Id 186 is Pallet Town; id 187 is Pallet Town's
bassline, alone.**

Parsed out of the cartridge: across the three banks the music region holds **46 real tracks, 105 inner
voices, and zero garbage ids**. The console confirms it — with id 187 the save loads, `wChannelSoundIDs` is
`[0, 187, 0, …]` and NR51 is `$22`: **one channel, playing a melody.** Every inner voice points *into* a
stream we already intend to import, so the shipped app gets **151 pieces of audio for the price of 46** —
and, because we run the *engine*, every note is known exactly, so sheet-music export (MIDI/MusicXML) is
nearly free.

⚠️ **The BANK byte is a loaded gun, though.** `PlaySound` maps whatever bank the save names and *then*
picks an engine — so a bank that isn't 2/8/31 executes **arbitrary cartridge bytes as code, every frame**.
**Verified: the console stops producing frames the instant the map loads.** The editor will offer only
2/8/31, and will *show* (never silently rewrite) a save that holds anything else.

Everything, with the cartridge's own testimony: [`reference/glitch-music.md`](reference/glitch-music.md).
Tools: `scripts/emu/analyze_music_ids.py`, `scripts/emu/probe_glitch_music.py` (both local-only, ROM-gated).

### ✅ Connection strips — done, and the hardest part of the map engine (2026-07-12)

The border ring is not a wall of trees: the game bleeds the **connected maps' edges** into it, so Pallet
Town's ring is really Route 1's bottom rows and Route 21's top rows. Now reproduced —
**78 of 78 connections in the game, byte-for-byte against the compiled structs in the real cartridge, zero
mismatches**, and the resulting ring is byte-identical to the console's `wOverworldMap`.

⚠️ **`MapDBEntryConnect::stripSize()` is WRONG** (it branches on `fromWidth < toWidth`; the macro clamps on
`min(curW + 3 - offset, toW)`), and `maps.json`'s **`flag`** field exists only to patch that. The real game
has no flag. `MapEngine` ignores both and recomputes from the macro, so nothing is broken today — but the DB
still carries a wrong formula. **Fixing it is Twilight's call** (curated data + a public DB API).

Everything about it: [`reference/map-connections.md`](reference/map-connections.md).

### ✅ Palettes / "contrast" — the six glitch palettes render (2026-07-12)

The save's `contrast` byte (`0x2609` = `wMapPalOffset`) is **not a brightness dial**: the game **subtracts it
from a pointer** into its fade-palette table (`LoadGBPal`, `home/fade.asm`). `0/3/6/9` land on real entries —
**the four contrast levels** (0 normal, 6 the "needs FLASH" cave palette, 9 black). Everything else reads
**across the seam between two entries** — `1, 2, 4, 5, 7, 8`: **the six glitch palettes.** Exactly what
Twilight said, and now we know why.

The map is drawn **through** whichever `rBGP` that produces, so a glitch palette renders as the genuine
article, not an imitation. **All ten verified against the real console's palette registers — zero
mismatches.**

⚠️ Note contrast **1 and 2 look normal on the map** — their damage is in `rOBP0`/`rOBP1`, the **sprite**
palettes. They will show the moment the player is drawn. That is the console's behaviour, not a gap.

Everything about it: [`reference/palettes.md`](reference/palettes.md).

### ✅ The player is drawn — and the "harmless" glitch palettes stop being harmless (2026-07-12)

He is where the **console's own OAM** says he is: screen (64, 60) — his tile column, and **4 pixels above**
his tile row ("*which makes sprites appear to be in the centre of a tile*"). Facing right is drawn as facing
**left, mirrored** — there is no right-facing art in the game. Colour 0 of an object is **always
transparent** (his cut-out).

**And this is what finally makes contrast 1 and 2 bite.** They leave `rBGP` alone — the map looks perfectly
normal — and wreck `rOBP0`/`rOBP1`, the **object** palettes. With the player on screen, contrast 1 now leaves
the world untouched and ruins *him*, exactly as the cartridge does.

Everything about it (incl. the OAM hardware and Gen 1's 16-slot system):
[`reference/sprites.md`](reference/sprites.md).

The ROM is Twilight's own cartridge backup: **git-ignored, never committed, never shipped**; without it
every case SKIPs. Setup + the traps (the "has the save loaded?" trap, `wCurMapTileset` bit 7):
[`reference/emulator-verification.md`](reference/emulator-verification.md).

The big structural blocker is **solved**: the `brg.file.data.dataExpanded.*` chain works, data reads
and **persists** across every screen, and the build is fast. The other major bug class — **QML
garbage-collecting parentless C++ QObjects** (the font/name blanking and the clicking-Pokémon crash)
— is also fixed (DB entries via `DB::qmlProtect`; savefile `Q_INVOKABLE` returns via `qmlCppOwned`;
storage boxes/mons/moves self-protect from their ctors). Data flows, names render, no crash clicking
around. Recovery from the 2026-06-06 corruption is complete and confirmed runtime parity.

We are in a **UI-polish phase**. Two big screens are polished + signed off: the **Pokémon
details editor** (General / DV-EV / Moves tabs + Glance pane) and the **name editors** (full keyboard
+ quick-edit popup). The Trainer Card, Bag, Pokémon storage, Rival, and Credits/About screens have all
had cleanup/redesign passes. The recurring underlying theme is the **Qt 6 Material control-height
issue** (Qt 6.5+ taller `TextField`/`ComboBox`); the fix everywhere is proper layouts, not pixel
offsets ([`reference/qt-patterns.md`](reference/qt-patterns.md)). **Read
[`reference/ui-patterns.md`](reference/ui-patterns.md) before any UI work.**

The **Market** now does real item trading. Its Exchange tab has three sub-tabs — **Currency**
(money↔coins), **Healing**, and **Custom** — where the last two swap one item for another, priced by
each item's **buy price**, across the bag + PC storage combined, previewed live and written only on
Checkout. The give side lists what you own, the get side lists **every** item with the unaffordable
ones greyed out (which is what guarantees the two "+" buttons are never both dead). Backed by
`ItemExchangeModel` and pinned by `tst_item_exchange` (14 cases).

⚠️ **The one hard-won rule there:** an exchange is priced as **one whole trade**, not per step — the
*total* value is rounded up to a whole number of the given item and only that single leftover is
refunded. 3 Fresh Water (₽600) costs exactly 2 Potions (₽600) and refunds **nothing**. Pricing each
step separately (the original bug, caught by Twilight and fixed on 2026-07-11) invents money out of
thin air. `giveFor()` / `refundFor()` are the single pricing path shared by the preview, the "+"
gating, and `checkout()` — keep it that way.

The **full keyboard** was rebuilt on 2026-07-11 into an actual **ASDF keyboard deck** — the headline of
**0.16.x**. **47 assignable keys** (26 letters + 10 digits + the 11 punctuation keys) × **8 pages**
(255 tiles need 8 pages; Shift/Ctrl/Alt give exactly 8 chords), each cap carrying one game tile with
the key that types it printed in the corner. The tile→key map is C++ (`mvc/fontkeyboard.*` →
`brg.keyboard`) and **pinned by `tst_font_keyboard`** — every tile reachable, and the only duplicated
tiles anywhere are the two box-frame edges (the Tiles I page lays the frame glyphs out **as the box**:
`Q W E / A _ D / Z X C` draws one).

The doctrine, in one line: **a tile goes where a real keyboard would put it whenever it can, and must
never pretend.** Base layer = lowercase + digits + punctuation on their own keys; Shift = uppercase +
the real shifted symbols (`!`, `$`, `?` on `/`, `:` on `;`); a cap whose tile matches what the physical
keyboard would type **drops its corner legend**, because there's nothing left to teach. **Caps Lock
locks the Shift page** (Shift inverts it, Ctrl/Alt ignore it), and **touching a physical modifier drops
any latched page** — otherwise Shift silently does nothing on a clicked-in Uppercase page. The name row
has two explicit modes: **keyboard mode** (no text field at all — a label with a soft caret; Backspace
eats a whole tile) and **edit mode** (a real field, live-updating, keyboard faded out and dead; check
applies, cross discards). The old chip list, filter sidebar, tilemap view and `FontSearchModel` are
**deleted**. Design + the full map:
[`plans/full-keyboard-redesign.md`](plans/full-keyboard-redesign.md); conventions:
[`reference/ui-patterns.md`](reference/ui-patterns.md) → "The full keyboard's DECK".

**Next:** in-app review of the new keyboard (see "Pending rebuilds" below); an end-to-end save/reopen
verification pass; remaining per-control test depth. See [`plans/next-steps.md`](plans/next-steps.md).

## Pending rebuilds / awaiting in-app review

- **The new full keyboard — reviewed in-app across five rounds with Twilight and SHIPPED in
  0.16.6-alpha (2026-07-11).** Nothing outstanding. The live-only behaviours (typing with the caps
  flashing, held-vs-latched modifiers, Caps Lock, animated tiles, token-aware Backspace, the shake when
  a key won't fit, edit mode, Tab opening the tileset picker on the tile pages) were all exercised on
  the real build.
  ⚠️ Known environment caveat: on a Windows box with **two keyboard layouts installed**, the OS eats
  Shift+Alt / Ctrl+Shift (switch layout) and Ctrl+Alt (AltGr) — those pages are still reachable by
  clicking the modifier caps or the page strip, which is exactly why they latch.
- **File-load crash fix (`s14`) — needs a kit rebuild.** C++ changed (`savefile.cpp`,
  `filemanagement.cpp/.h`, `router.cpp`) **and** a new QML file was added
  (`screens/modal/FileError.qml`, already in `app.qrc`). After building, test: (1) a recent file whose
  path no longer exists → silently dropped from the list on next launch, never crashes; (2) a
  present-but-truncated/locked `.sav` → shows the `FileError` modal, save stays untouched. ⚠️ Not yet
  build-verified on the dev machine.
- Several recent QML/asset passes are **awaiting in-app review** (see the latest entries in
  [`sessions/`](sessions/README.md)).

> **Build reminder:** rebuild the **kit dir**
> (`projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`) for in-app testing — not just `build/`.
> New `.qml` files MUST be added to `app/app.qrc` or they fail at runtime ("Type X is not a type").
> Editing a `savefile` `.cpp` rebuilds the **DLL**, not the exe — verify by the DLL timestamp.

## Open issues

| Issue | Where | Status / notes |
|-------|-------|----------------|
| ~~Glitch / half-baked maps don't render~~ **RESOLVED 2026-07-12 — they all render now** | `MapEngine::sourceMap()` | They were never a data gap. `maps.json` **already models them**: every one of the 25 (22 unsized "Unused Map XX" + the 3 sized `*_Copy`) carries an `incomplete` field naming the map it is an unfinished duplicate of — and it agrees **exactly** with the ROM's header-pointer table (11 → Saffron City, 105–117 → Lance's Room, 204–206 → Rocket Hideout Elevator, 231 → Route 16 Gate 1F, 237–244 → Silph Co 2F, 69 → Trashed House, 75 → Path Entrance Route 6, 173 → Cinnabar Mart). The renderer now follows that link and draws the map they copy — which is precisely what a Game Boy loading that id puts on screen. **No JSON was changed and nothing was invented.** The screen says plainly that it's showing an unfinished copy, and of what. **All 248 ids render** (`tst_map::everyMapIdRenders`); only "Last Map" (255) is genuinely empty. |
| ~~🐞 `AreaAudio::setTo()` clobbers the track id with the bank~~ **FIXED 2026-07-12** | `savefile/…/expanded/area/areaaudio.cpp` | It was `musicBank = musicID = musicEntry->bank;`. Fixed and pinned by `tst_area::audio_setTo_keepsIdAndBankApart` (every map in the game). ⚠️ Writing that test proved the bug was **dormant**: `MapsDB` is never deep-linked at boot, so `getToMusic()` is null for every map and `setTo()` has been writing `0/0`. **The moment the deep-link landmine below is defused, this code path goes live** — which is exactly why it's fixed now. |
| **Latent landmine: map DB `getToMap()`/`getToSprite()` never resolved** | `db.cpp` `deepLinkAll()`; consumers in `WarpData`/`MapConnData`/`SpriteData`/`AreaMap` | Still dormant, and the new map screen **deliberately does not touch those accessors** (it resolves the tileset by name and looks maps up by id, so it needs no deep link). Not a crash today. **Still must be defused before map *editing* / re-enabling map randomize** — those will dereference the unresolved `to*` links → add `MapsDB::inst()->deepLink()` to `DB::deepLinkAll()` first. Confirmed safe once called, via `tst_sprite_data` (all 918 sprites resolve). |
| Randomizer: not-yet-built screens (Maps, Hall of Fame, Options) excluded | `savefileexpanded.cpp`, `worldgeneral.cpp` | **Working within scope as of 2026-06-07.** `randomizeExpansion()` runs end-to-end + is test-covered. Maps/HoF/Options calls are commented out (matching the disabled home tiles), each with a re-enable note. Re-enabling map randomize is gated mainly on calling `MapsDB::inst()->deepLink()` at boot (the type strings + per-call guards turned out to be the same deepLink landmine, not separate defects). |
| Name editors — ongoing review | `name-full/*`, `general/NameDisplay.qml` | Ongoing live tweaks. `NameEdit`/`NameDisplay` are **shared** by player/rival/nickname + the keyboard footer preview — verify all of them on each rebuild. |
| Keyboard caps are cramped at the default 750×480 window | `name-full/KeyboardDeck.qml` | By design it *scales* (key unit = min(width/13.5, height/6.0)), so it's comfortable on a resized window and tight on the default one. Multi-char code labels (`trainer`, `player`) elide at the smallest size. Revisit if Twilight wants the default window bigger, or the header/footer slimmer, to buy the deck more room. |
| Dead menu files (unused after s13z7) | `name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`, `TilesetMenu.qml` | No longer instantiated; left in place + in qrc. Safe to delete later. |

**Intentional (not bugs):** in the storage grid, Pokémon names are always visible below each icon
(dark text, no background). The player **ID** commits on Enter/focus-out (not per keystroke) —
revertible if wanted live.

## Testing

A comprehensive automated suite lives under `projects/tests/` (QtTest + CTest). **Full `ctest` is
green (75/75 on the Qt 6.11 kit).** Newest: **`tst_map`** (18 cases) — the block data, the overworld
buffer, the view maths, the renderer and `brg.map`. Its keystone is
`viewPointer_matchesWhatTheGameStored`: it recomputes the view pointer the *Game Boy* wrote into the
save and demands a byte-exact match. That test is the map emulator's foundation — if it goes red,
nothing downstream of it can be trusted. Library-layer line coverage is at/above 90% (common 100%, db
~90%, savefile ~90%; app layer is the laggard). The Linux Docker env runs four variants green
(standard / asan+ubsan / xvfb / coverage **89.98%** as of 2026-06-22). A QML-load smoke test
(`tst_qml_screens`), a real-app GUI suite (`tst_gui_*`), signal/slot (`tst_signals`), model-contract
(`tst_model_tester`), visual-regression (`tst_visual_regression`) and BDD acceptance (`tst_acceptance`)
suites gate `main`. The road to "100%" (3 gap kinds; only the reachable-fillable one is worth chasing)
is mapped in [`plans/testing.md`](plans/testing.md) → "Coverage status". A **static-analysis layer**
(clang-tidy + cppcheck +
informational qmllint, via `scripts/lint.*` and a `lint` CI workflow) was added 2026-06-22 — the
clang-tidy gate is clean (143 TUs, 0 findings) and surfaced/fixed 8 real defects (see version.md).
Strategy, coverage baseline, and remaining gaps: [`plans/testing.md`](plans/testing.md).

## Build health

| Layer | Status |
|-------|--------|
| common | ✅ Clean |
| db | ✅ Clean |
| savefile | ✅ Clean (`Q_DECLARE_OPAQUE_POINTER` only on untraversed types; `qmlownership.h` in place) |
| app | ✅ Clean |

Build speed restored s13c (over-includes trimmed). `dllimport` warning silenced via
`-Wno-ignored-attributes` in root `CMakeLists.txt`.

## Runtime health

| Area | Status |
|------|--------|
| Window / DB load / file open+save | ✅ |
| `dataExpanded.*` chain — all screens read + persist | ✅ |
| Trainer Card / Bag / Pokémon storage data | ✅ confirmed |
| Trainer + Rival name render (animated) + persist | ✅ (was QML GC of FontDBEntry) |
| Pokémon box: click → details opens; no crash | ✅ (GC crash fixed) |
| Pokémon box: hover name (+ pen icon) | ✅ |
| Combo box (Select*) popups scroll on long lists | ✅ (capped popup height) |
| Badges; Pokédex toggles | ✅ |
| Number fields (playtime / item count / PP) width + centering | ✅ |
| Trainer-card layout — centered box, compact fields, clock width | ✅ confirmed |
| Randomize name (full editor + trainer screen) | ✅ |
| Pokémon **editor** responsiveness + layout/styling | ✅ confirmed |
| Pokémon editor **Moves tab** — grouped-panel restyle + drag-to-reorder (`reorderMove`) | ✅ tests green; in-app review pending |
| Name editors (nickname / player / rival) — popup + full keyboard | ✅ iterated |
| Player/rival name + player ID — atomic commit-on-finish, no hang, no OT corruption | ✅ |
| Tileset picker — every tile clickable; no freeze on variable render | ✅ |

## Recurring non-fatal warnings (harmless)

- `'dllimport' attribute ignored` on `MapDBEntry` etc. — Qt + llvm-mingw shared-lib cosmetic;
  **silenced** via `-Wno-ignored-attributes`.
- Items "could not be deep linked" / "Values are not correct on sprite X" — pre-existing data.
- On exit: `QDxgiVSyncService not destroyed in time`, `QThreadStorage entry N destroyed…` — benign Qt
  shutdown ordering.
- Offscreen test runs: `QFontDatabase: Cannot find font directory` — benign (allowlisted in the GUI
  harness).

## The "crashes" — two different things

1. **System-wide** Qt-debugger pop-ups (also in Notepad/taskbar/Settings) — environment/Qt-install
   issue, NOT this app. Don't chase.
2. **In-app** use-after-free from QML GC'ing parentless C++ QObjects — **fixed**. A silent "terminated
   abnormally" after interaction was this. If a real in-app crash recurs, get a project-debugger stack
   trace. Details: [`reference/diagnostic-methods.md`](reference/diagnostic-methods.md),
   [`sessions/`](sessions/README.md).
