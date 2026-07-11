# Next Steps

_Ordered by priority. Updated end of session 13z8 (2026-06-05)._

Structural blockers are solved (the `dataExpanded` chain + the QML-GC use-after-free class). We're
in a **UI-polish phase**. Authoritative open-issues list: `status.md` → "Open Issues". History:
`sessions/`. UI conventions: `reference/ui-patterns.md`.

## Done — two big screens polished + iterated

- **Pokémon details editor** (13k–13t): responsive (`function onX()` → `onX:`), proper layouts,
  borderless combos w/ hover underlines, square move pills, slider tooltips, popup nickname editor.
- **Name editors — full keyboard + quick-edit popup** (13v–13z8): full rebuild + many live iterations.
  Simulated group (Outdoor/Tileset/Grid-Tileset, `FlatToggle`), single-select radio filters, pill grid
  + image-only static tile tooltip, ⋮ menus replaced by buttons (dice randomize, Name/Example toggle +
  `>>`), example demo localized to each editor, plus the player/rival/ID **edit-hang + OT-corruption**
  fix and the `expandStr` infinite-loop + tileset off-by-one fixes. Conventions: `ui-patterns.md`.

## Done — Market Exchange item trading (2026-07-11, shipped in 0.15.2-alpha)

- **Exchange sub-tabs**: Currency (money↔coins) / Healing / Custom. The latter two trade item↔item by
  buy price across bag + PC storage, previewed live, written only on Checkout (`ItemExchangeModel`).
- **Asymmetric dropdowns**: give side = what you own, get side = every item, unaffordable ones greyed
  (which makes "never both + disabled" structural). Healing opens on Potion ⇄ Fresh Water.
- **Whole-trade pricing fix**: the total is rounded up once, not per step — an even trade refunds
  nothing. See `status.md`; do not reintroduce per-step rates.
- **`tst_item_exchange`** (14 cases) pins all of the above. Full `ctest` 73/73.

## Now / next

1. **In-app review of the NEW full keyboard** (rebuilt 2026-07-11 as an ASDF deck — see
   [`full-keyboard-redesign.md`](full-keyboard-redesign.md)). Screenshot-reviewed on every page and
   `ctest` green; what a still PNG can't show still needs a live pass: typing (caps flashing with it),
   held vs. latched modifiers, tiles animating, token-aware Backspace, the shake when a key won't fit,
   and clicking into the name box (legends dim; Ctrl+C/V behave).
2. **Continued review of the name editors.** Live tweaks ongoing. `NameDisplay`/`NameEdit` are
   shared by player/rival/nickname + the keyboard footer — verify all on each rebuild.
3. **Does the deck want more room at the default 750×480 window?** It scales, so it's comfortable
   resized and tight at the default: caps land around 30px and the longer code labels (`trainer`,
   `player`) elide. Options if Twilight wants it roomier out of the box: a bigger default window, a
   slimmer header/footer, or a narrower detail pane. (`DetailView` was kept — it's now the only place
   a multi-char/variable code can be rendered large enough to read.)
3. **End-to-end persistence pass** (edit → save → reopen) across player, rival, money, coins, badges,
   pokédex, items, pokémon. several have been spot-confirmed; do a full pass. Especially confirm the new
   **commit-on-finish** name path round-trips correctly and OT data on owned mons follows the player.
4. **Trainer-card number-field spacing** — apply the `FieldLabel` + RowLayout pattern to retire the
   last fixed-offset layouts (`CardFront.qml`, `DefTextEdit.qml`). The exact look is a human/design call.

## Testing (new track — planned 2026-06-07)

- **Comprehensive automated test suite** — full strategy in `plans/testing.md`. **Phase 1 is now
  implemented** under `projects/tests/` (CMake/CTest harness + fixture helper; round-trip identity &
  money-isolation tests on the real saves; DB integrity test). ⚠️ **Not yet built/run on the dev
  machine** — next action: build in Qt Creator (or `cmake --build` then `ctest --output-on-failure`)
  and triage results. The first run may legitimately surface a round-trip imperfection or an offset to
  nudge. Then continue the phased rollout (savefile/common/db coverage → negative/integration/E2E →
  randomizer/fuzz → sanitizers/coverage → CI → app C++ → QML last). Remaining open questions there:
  coverage-gate strictness, CI host, characterizing `BaseSAV.sav`.

- **QML smoke test + first comprehensive GUI suite — BUILT 2026-06-13.** `tst_qml_screens` (loads
  every screen, fails on any QML warning; `main` gated on it) PLUS a real-app GUI suite on the
  `tests/helpers/guiapp.h` harness: `tst_gui_navigation` (sweep every screen on populated + new saves),
  `tst_gui_saveload` (edit through screens → save → reopen across files, independence, randomize,
  byte-stability), `tst_gui_input` (synthesized keyboard input). All run `offscreen` and gate BOTH the
  Linux and Windows CI jobs. ✅ **BUILT + RUN + GREEN on the Qt 6.11 kit 2026-06-13 (full `ctest`
  61/61).** Triage done (harness `keyType`/`appBody` StackView-class/`busy`-wait fixes, font allowlist,
  money-field `objectName`, detail-screen load-only smoke); a real **Pokemart empty-cart crash** was
  found + fixed (`status.md` Open Issues). Full writeup in `status.md` + `testing.md`.
  - **NEXT: continue the GUI roadmap** (`plans/testing.md` → "Broader GUI coverage"): synthetic
    fixture matrix (generated edge-case saves in a clearly-synthetic folder), detail-screen flows
    (pokemon→select→details with a real selection — verifies the cold-load bindings the smoke test
    leaves load-only), keyboard shortcuts, drag & drop flows, shell/fragment smoke, Red/Blue +
    game-state fixtures, and ASan-under-GUI on Linux CI.

## i18n (new track — added 2026-06-13)

The Qt Linguist pipeline is in place (`reference/i18n.md`); English ships. Open follow-ups, none urgent:

1. **In-app language switcher — DEFERRED** (decided 2026-06-13): with only en_US and no in-app
   Options/Settings screen yet, there's nothing to switch to. Revisit once a second locale + the
   Options screen exist. `ui/language` is the hook; it'll need engine `retranslate()` + re-evaluating
   bindings plus the Settings UI. Until then, locale = system/registry only.
2. **Ship a real second locale** when wanted — add one `TS_FILES` line, run `update_translations` in
   Qt Creator, translate in Linguist. Proves the path end-to-end (currently only en_US exists).
3. **Optional**: wrap the deliberately-skipped tiny format prefixes (`"L"+level`, `"x"+count`,
   `"No."+n`) as `qsTr().arg()` if a real locale needs them.
4. **Out of scope / much later**: game-data name localization (Pokémon/move/item) — region/encoding
   -bound save data, a separate effort; do NOT route through Qt translations.

## Pending decisions — tracked temporary exceptions (resolve, don't let linger)

These are deliberate "dirty patches" the test pass put in to keep things working until
a human makes the call. Each is a single-truth question with (likely) one correct answer.

1. **type2 single truth (single-type Pokémon) — RESOLVED 2026-07-09.** Settled from the pokered
   disassembly: the game stores a single type as **duplicate-of-type1** (`data/pokemon/base_stats/*.asm`,
   e.g. Charmander `db FIRE, FIRE`; `engine/pokemon/add_mon.asm` copies both type bytes verbatim into
   party/box data), and **`0xFF` is not a valid type** (`constants/type_constants.asm` runs `$00..$1A`).
   So the canonical WRITTEN form is duplicate-of-type1 — which is what `save()` already emits for a
   non-explicit single type, so on-disk output is unchanged for normal saves. Byte fidelity is
   untouched: a save that literally stored `0xFF` (only ever hacked/glitch) still round-trips as `0xFF`
   via `type2Explicit`. The fix: every editor (re)generation — `randomize()`, `correctTypes()`,
   `update(resetType)` — now clears `type2Explicit`, so a generated/corrected single type serialises as
   the duplicate, never a stray `0xFF`. `isCorrected()`'s tolerance (accept `0xFF` OR the duplicate) is
   now the finalized intended behaviour, not a temporary patch. Regression: `box_singleTypeCanonicalForm`.
   Refs: `pokemonbox.cpp` `load()`/`save()`/`update()`/`correctTypes()`/`randomize()`/`isCorrected()`,
   `reference/gen1-knowledge.md` → "Single-type storage".

_(Resolved 2026-06-08: `isMinEvs()` `||`→`&&` — confirmed a bug and fixed, not a pending
decision.)_

## Optional cleanup

5. Delete the now-unused menu files (`name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`,
   `TilesetMenu.qml`) and their `app.qrc` entries once the button-based editors are settled.
6. Give the inline combos (`StarterEdit`, `Rival` starter, `SimulatedTilesetCombo`) the same hover
   underline as the `Select*` combos, for consistency.
7. Maps screen — confirm map data loads and `appBody.push` to MapDetails works.

## Don't chase

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. See `status.md` → "The crashes — two different things".

## Longer Term

- **Window chrome & selective fluidity**, randomization features, map editor, full screen coverage —
  `plans/future.md`. These are human/design calls.

## Don't chase

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. (The *in-app* GC use-after-free crashes ARE fixed.)
  See `status.md` → "The crashes — two different things".

## Longer Term

- **Window chrome & selective fluidity** — an exploratory idea (lock to the design resolution
  but let tall-list screens like Pokemart grow; leaning toward frameless custom chrome with
  app-driven per-screen sizing). Direction + open questions in `plans/future.md` → "Window Chrome &
  Fluid Layout". A human/design call.
- Randomization features, map editor, full screen coverage — `plans/future.md`.

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. (The *in-app* GC use-after-free crashes ARE fixed.)
  See `status.md` → "The crashes — two different things".

## Cosmetic / layout (in progress)

- ✅ Combo box popups now scroll on long lists (s13k); ✅ hover pen tints light (s13k).
- **Window chrome & selective fluidity** — an exploratory idea (lock to resolution but let
  tall-list screens like Pokemart grow; possibly borderless custom chrome + drag handle that
  animates back to the design size). Direction + open questions captured in `plans/future.md`
  → "Window Chrome & Fluid Layout". Suggested low-risk first step: make tall-list screens
  internally scrollable / height-flexible before attempting custom window chrome. A human/design call.

## Longer Term

See `plans/future.md` — randomization features, map editor, full screen coverage.

