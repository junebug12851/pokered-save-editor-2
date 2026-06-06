# Next Steps

_Ordered by priority. Updated end of session 13z11 (2026-06-05)._

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
- **Trainer Card layout pass** (13z10–13z11): restored the centered `500×250` box; one `CardFront.fieldH`
  (28) knob compacts every field + tighter margins; playtime clock narrowed (`digitPad`) and its row
  pinned to `fieldH` so the digits/`:` vcenter; `DefTextEdit`'s built-in label now vcenters. Twilight-confirmed.

## Now / next

1. **Twilight's continued review of the name editors.** Live tweaks ongoing. `NameDisplay`/`NameEdit` are
   shared by player/rival/nickname + the keyboard footer — verify all on each rebuild.
2. **Decide the fate of the right-side `DetailView`** in the full keyboard. The pill tooltip now shows
   the rendered tile; `DetailView` still shows text info on hover (and the tileset page uses it). Ask
   Twilight whether to keep it, slim it, or drop it for more grid room.
3. **End-to-end persistence pass** (edit → save → reopen) across player, rival, money, coins, badges,
   pokédex, items, pokémon. Twilight has spot-confirmed several; do a full pass. Especially confirm the new
   **commit-on-finish** name path round-trips correctly and OT data on owned mons follows the player.

## Optional cleanup

5. Delete the now-unused menu files (`name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`,
   `TilesetMenu.qml`