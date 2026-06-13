# Next Steps

_Ordered by priority. Updated end of session 13z8 (2026-06-05)._

Structural blockers are solved (the `dataExpanded` chain + the QML-GC use-after-free class). We're
in a **UI-polish phase**. Authoritative open-issues list: `status.md` → "Open Issues". History:
`sessions/session-log.md`. UI conventions: `reference/ui-patterns.md`.

## Done — two big screens polished + Twilight-iterated

- **Pokémon details editor** (13k–13t): responsive (`function onX()` → `onX:`), proper layouts,
  borderless combos w/ hover underlines, square move pills, slider tooltips, popup nickname editor.
- **Name editors — full keyboard + quick-edit popup** (13v–13z8): full rebuild + many live iterations.
  Simulated group (Outdoor/Tileset/Grid-Tileset, `FlatToggle`), single-select radio filters, pill grid
  + image-only static tile tooltip, ⋮ menus replaced by buttons (dice randomize, Name/Example toggle +
  `>>`), example demo localized to each editor, plus the player/rival/ID **edit-hang + OT-corruption**
  fix and the `expandStr` infinite-loop + tileset off-by-one fixes. Conventions: `ui-patterns.md`.

## Now / next

1. **Twilight's continued review of the name editors.** Live tweaks ongoing. `NameDisplay`/`NameEdit` are
   shared by player/rival/nickname + the keyboard footer — verify all on each rebuild.
2. **Decide the fate of the right-side `DetailView`** in the full keyboard. The pill tooltip now shows
   the rendered tile; `DetailView` still shows text info on hover (and the tileset page uses it). Ask
   Twilight whether to keep it, slim it, or drop it for more grid room.
3. **End-to-end persistence pass** (edit → save → reopen) across player, rival, money, coins, badges,
   pokédex, items, pokémon. Twilight has spot-confirmed several; do a full pass. Especially confirm the new
   **commit-on-finish** name path round-trips correctly and OT data on owned mons follows the player.
4. **Trainer-card number-field spacing** — apply the `FieldLabel` + RowLayout pattern to retire the
   last fixed-offset layouts (`CardFront.qml`, `DefTextEdit.qml`). Twilight owns the exact look.

## Testing (new track — planned 2026-06-07)

- **Comprehensive automated test suite** — full strategy in `plans/testing.md`. **Phase 1 is now
  implemented** under `projects/tests/` (CMake/CTest harness + fixture helper; round-trip identity &
  money-isolation tests on the real saves; DB integrity test). ⚠️ **Not yet built/run on Twilight's
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

## Pending decisions — tracked temporary exceptions (resolve, don't let linger)

These are deliberate "dirty patches" the test pass put in to keep things working until
Twilight makes a real call. Each is a single-truth question with (likely) one correct answer.

1. **type2 single truth (single-type Pokémon).** Real saves store a single type inconsistently —
   sometimes `0xFF`, sometimes a duplicate of type1 — so the **load/expanded side officially tolerates
   both** (byte fidelity: read it back exactly as loaded, change only when asked; `isCorrected()` was
   patched 2026-06-08 to accept either form). **Undecided:** the single canonical form the editor should
   **write** when it itself generates/corrects a single type (`0xFF` vs duplicate-of-type1). Today a
   generated single type collapses to `type2=0xFF` internally and `save()` writes the duplicate (`type1`)
   when `type2Explicit` is false. Pick the real truth, then tighten `isCorrected()`/`save()` to it.
   Refs: `pokemonbox.cpp` `isCorrected()`/`update()`/`save()`, `reference/fix-patterns.md`.

_(Resolved 2026-06-08: `isMinEvs()` `||`→`&&` — confirmed a bug by Twilight and fixed, not a pending
decision.)_

## Optional cleanup

5. Delete the now-unused menu files (`name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`,
   `TilesetMenu.qml`) and their `app.qrc` entries once Twilight's happy with the button-based editors.
6. Give the inline combos (`StarterEdit`, `Rival` starter, `SimulatedTilesetCombo`) the same hover
   underline as the `Select*` combos, for consistency.
7. Maps screen — confirm map data loads and `appBody.push` to MapDetails works.

## Don't chase

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. See `status.md` → "The crashes — two different things".

## Longer Term

- **Window chrome & selective fluidity**, randomization features, map editor, full screen coverage —
  `plans/future.md`. Twilight owns these.

## Don't chase

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. (The *in-app* GC use-after-free crashes ARE fixed.)
  See `status.md` → "The crashes — two different things".

## Longer Term

- **Window chrome & selective fluidity** — Twilight's exploratory idea (lock to the design resolution
  but let tall-list screens like Pokemart grow; leaning toward frameless custom chrome with
  app-driven per-screen sizing). Direction + open questions in `plans/future.md` → "Window Chrome &
  Fluid Layout". Twilight owns this.
- Randomization features, map editor, full screen coverage — `plans/future.md`.

- The **system-wide** `terminated abnormally` / Qt-debugger pop-ups (also in Notepad/taskbar) —
  environment/Qt-install issue, not this app. (The *in-app* GC use-after-free crashes ARE fixed.)
  See `status.md` → "The crashes — two different things".

## Cosmetic / layout (in progress)

- ✅ Combo box popups now scroll on long lists (s13k); ✅ hover pen tints light (s13k).
- **Window chrome & selective fluidity** — Twilight's exploratory idea (lock to resolution but let
  tall-list screens like Pokemart grow; possibly borderless custom chrome + drag handle that
  animates back to the design size). Direction + open questions captured in `plans/future.md`
  → "Window Chrome & Fluid Layout". Suggested low-risk first step: make tall-list screens
  internally scrollable / height-flexible before attempting custom window chrome. Twilight owns this.

## Longer Term

See `plans/future.md` — randomization features, map editor, full screen coverage.

