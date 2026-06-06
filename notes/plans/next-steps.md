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

