# Project Status

_Last updated: 2026-06-06 (post sed/mount corruption recovery; the pre-corruption work was session 13z11)_

> **RECOVERY NOTE (2026-06-06):** A bulk-`sed`/mount corruption truncated 55 source files + 8 notes;
> **all were fully recovered** from Cowork chat transcripts (method: `reference/diagnostic-methods.md`
> → "Recovering files from Cowork chat transcripts"). `pokemonbox.cpp`/`.h` were rebuilt by replaying
> the captured edit history onto the `af883fd` clone base and validated against transcript reads; the 7
> clone-based files (`settings.cpp`, `fontsdb.cpp`, `area.h`, `areasign.cpp`, `areasprites.cpp`,
> `pokemonstoragebox.h`, `storage.cpp`) had every residual fixed.
>
> **POST-RECOVERY BUILD-UP (2026-06-06):** The recovered tree did NOT build clean — the recovery left
> many residual defects that surfaced only on compile/link/run. Worked through them error-by-error;
> **the project now compiles, links, AND runs.** Key lesson (Twilight): **diagnose against git history,
> not HEAD** — HEAD (`2c2d6e5`) is the corrupted commit (many files truncated in it); the only clean
> reference is the 2020 tree (`af883fd`). Defect classes fixed (all in `reference/fix-patterns.md`):
> stray duplicate `util/hiddencoinsdb` in CMake; dropped `#include`s (mapdbentry, Qt6 metatype-complete
> property headers across savefile, pse-db/db.h); dropped private members (gamecornerdb/fontsdb
> `store`/`ind`/`buyPrice`, mapsearch tail); dropped friends (ItemDBEntry); dropped method bodies
> (DB::qmlProtect/qmlHook, FontSearch::clear/keepAnyOf, PlayerBasics::getPlayerId/getNonTradeMons/
> fixNonTradeMons); truncated/eaten declarations (area.h class line, filemanagement expandRecentFiles);
> old-API reversions (pokemonbox.cpp Random::/store/getIndAt); protected-field direct access
> (areasign/areasprites/mapselectmodel → getters); and a **runtime hang** — `FontsDB::splice` lost its
> `out.remove()` (reverted s13y replace-not-insert) → infinite loop expanding a variable tile.
> **Next: Twilight must COMMIT/BACK UP now (builds+runs; the prior backup was the corrupted one), then
> continue runtime testing.** Full account: `sessions/session-log.md`.

This file is the **current state** only. For the chronological history of what changed each
session and why, see `sessions/session-log.md`. For root-cause mechanics, see
`reference/qt6-patterns.md` and `decisions/`.

## Current State (read this first)

The big structural blocker is **solved**: the `brg.file.data.dataExpanded.*` chain works, data
reads and **persists** across every screen, and the build is fast. The other major class of
bugs — **QML garbage-collecting parentless C++ QObjects** (which caused the font/name blanking and
the clicking-Pokémon crash) — is also fixed (DB entries via `DB::qmlProtect`; savefile
`Q_INVOKABLE` returns via `qmlCppOwned`). Twilight confirmed: data flows, names render, no crash when
clicking around.

We are in a **UI-polish phase**. Two big screens are now polished + Twilight-iterated:

1. **Pokémon details editor** (General / DV-EV / Moves tabs + Glance pane) — sessions 13k–13t, Twilight
   signed off ("looking solid"). Conventions in `reference/ui-patterns.md`.
2. **Name editors — full keyboard + quick-edit popup** (`name-full/*`, `general/NameDisplay.qml`) —
   sessions 13v–13z8, heavily iterated with Twilight live. Now: wide centered name box; a **"Simulated"**
   group (label + Outdoor toggle + tileset combo + **Grid/Tileset** view toggle) using a flat-square
   `FlatToggle`; **single-select radio filters** (All / Normal Only / Single / Multi / Variable /
   Picture / Control) with help on a ⓘ dot; a **color-coded pill grid** whose hover tooltip renders the
   actual tile (image only, static, control = none); the **⋮ menus removed** in favor of explicit
   buttons (dice Randomize-Name, a **Name/Example** toggle + `>>` re-roll); the popup gained a Simulated
   bar across its top. The **example/box demo is now local to each editor** (never the regular name row).

The recurring underlying theme remains the **Qt 6 Material control-height issue** (Qt 6.5+ taller
`TextField`/`ComboBox`); the fix everywhere is proper layouts, not pixel offsets
(`reference/qt6-patterns.md`). **Read `reference/ui-patterns.md` before more UI work.**

**Last sessions (s13z10–13z11) — Trainer Card layout pass (QML-only, no rebuild):** (1) restored the
**centered grey box** (the `SwipeView` had been changed to `anchors.fill: parent` in the refactor →
back to centered `500×250`); (2) **compacted every field** via one `CardFront.fieldH` (28) knob +
tighter row margins, fixing the too-tall Qt 6 Material boxes; (3) **fixed the playtime clock** —
narrowed each digit field to ~2 chars (`PlaytimeEdit.digitPad`) and stopped the row sizing to the
Material implicit height (it now pins to `fieldH` and vcenters the `:`), which had made the digits ride
high; (4) **vcentered `DefTextEdit`'s built-in label** (it lacked `verticalAlignment`, so ID/Money/Coins
labels rode high). All Twilight-driven, live-iterated.

**Next:** continued Twilight review of the name editors; an end-to-end save/reopen verification pass. See
`plans/next-steps.md`.

**Build/run notes:** C++ changed earlier in this arc (`FontSearch` 13v/13y, `PlayerBasics` +
`PokemonBox` 13w, `settings.h` colors + `fontsdb.cpp` splice 13y) and **two new QML files were added to
`app.qrc`** (`FlatToggle.qml`, `SimulatedTilesetCombo.qml`, 13z7) — all of those need a **Rebuild**.
Pure edits to existing QML hot-reload in debug. **New QML files MUST be added to `app/app.qrc`** or they
fail at runtime with "Type X is not a type" (this project resolves QML types via the qrc, not directory
scanning).

## Open Issues

| Issue | Where | Status / notes |
|-------|-------|----------------|
| ~~Pokémon editor: values not reacting / DV-EV / moves / shiny menus dead~~ | `pokemon-details/*` | **Fixed + Twilight-confirmed (s13l).** `function onX()` dead handlers → `onX:`. |
| ~~Pokémon editor: boxes too tall / overlapping / misalignment~~ | `pokemon-details/*` | **Fixed + Twilight-confirmed (s13m–s13t).** |
| ~~Full keyboard rebuild (name box / Simulated group / filters / pill grid / view toggle)~~ | `name-full/*`, `FullKeyboard.qml` | **Done + Twilight-iterated (s13v–s13z8).** |
| ~~Quick-edit popup broken out of its ⋮ menu (buttons + Simulated bar)~~ | `general/NameDisplay.qml`, `NameEdit.qml` | **Done (s13z7–z8).** |
| ~~Player/rival name (+ ID) edit hang / per-keystroke save writes / OT corruption~~ | `PlayerBasics`, `PlayerNameEdit`/`Rival`/`PlayerIdEdit` | **Fixed s13w.** Writeup: `reference/player-name-hang.md`. UX confirmed-ish: player **ID** applies on Enter/blur. |
| ~~`expandStr` infinite-loop freeze on a lone variable tile~~ | `db/.../fontsdb.cpp` `splice` | **Fixed s13y** (replace-not-insert). |
| ~~Tileset picker last tile (bold "9") not clickable~~ | `name-full/TilesetPicker.qml` | **Fixed s13y2** (off-by-one; `fontAt` is 1-based). |
| Name editors — Twilight's continued review | `name-full/*`, `general/NameDisplay.qml` | Open: ongoing live tweaks. `NameEdit`/`NameDisplay` shared by player/rival/nickname + keyboard footer — verify all on each rebuild. |
| Full keyboard: right-side `DetailView` (text info on hover) | `name-full/DetailView.qml` | Still present + wired alongside the pill tooltip. Confirm it still reads well / whether Twilight wants to keep it. |
| ~~Trainer-card box size / field spacing / clock width / label centering~~ | `TrainerCard.qml`, `CardFront.qml`, `PlaytimeEdit.qml`, `DefTextEdit.qml` | **Done + Twilight-confirmed (s13z10–z11).** Centered box restored (`500×250`); `CardFront.fieldH` (28) compacts every field; playtime narrowed (`digitPad`) + row pinned to `fieldH` so digits/`:` vcenter; `DefTextEdit` label now vcenters. Tuning knobs in `ui-patterns.md`. (Horizontal text padding inside the non-clock fields still at `DefTextEdit` default — trim per-instance only if Twilight wants it tighter.) |
| Dead menu files (unused after s13z7) | `name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`, `TilesetMenu.qml` | No longer instantiated; left in place + in qrc. Safe to delete later. |

Intentional (not bugs): Pokémon names show **on hover** only (Twilight confirmed). The player **ID**
commits on Enter/focus-out (not per keystroke) — Twilight can revert if she wants it live.

`NameDisplay` / `NameEdit` are **shared** by the player-name, rival, and nickname editors (and
`NameDisplay` is the keyboard footer preview). Changes touch all of them; verify each on a rebuild.

## Build Health

| Layer | Status |
|-------|--------|
| common | ✅ Clean |
| db | ✅ Clean |
| savefile | ✅ Clean (header includes reworked s13/s13c; `Q_DECLARE_OPAQUE_POINTER` only on untraversed types; `qmlownership.h` added) |
| app | ✅ Clean |

Build speed restored s13c (over-includes trimmed). `dllimport` warning silenced via
`-Wno-ignored-attributes` in root `CMakeLists.txt`.

## Runtime Health

| Area | Status |
|------|--------|
| Window / DB load / file open+save | ✅ |
| `dataExpanded.*` chain — all screens read + persist data | ✅ (s13/s13b) |
| Trainer Card / Bag / Pokémon storage data | ✅ Twilight confirmed |
| Trainer + Rival name render (animated) + persist | ✅ (s13f — was QML GC of FontDBEntry) |
| Pokémon box: click → details opens; no crash | ✅ (s13d click, s13g/h GC crash fix) |
| Pokémon box: hover name (+ pen icon) shows | ✅ (s13j — explicit `contentItem`); pen now tints light (s13k `brightness:1.0`) |
| Combo box (Select*) popups scroll on long lists | ✅ (s13k — capped popup `height`) |
| Badges; Pokédex toggles | ✅ |
| Number fields (playtime / item count / PP) width + centering | ✅ (s13b/s13e) |
| Trainer-card Coins/Starter/Money overlap | ✅ (s13e `.bottom` anchoring) |
| Trainer-card layout — centered box, compact fields, clock width, label centering | ✅ Twilight-confirmed (s13z10–z11): `SwipeView` 500×250; `CardFront.fieldH`; `PlaytimeEdit` `digitPad` + row pinned to `fieldH`; `DefTextEdit` label vcentered |
| Randomize name (full editor + trainer screen) | ✅ (s13e) |
| Keyboard filter / description / random / button styling | ✅ (s10–11) |
| Pokémon **editor** responsiveness (species/level/status/HP, moves, DV/EV, shiny, menus) | ✅ Fixed + Twilight-confirmed (s13l) |
| Pokémon **editor** layout + styling (GridLayout/RowLayout, heights, borderless combos w/ hover, square move pills, ⋮ buttons, slider hover tooltips) | ✅ Polished + Twilight-confirmed (s13m–s13t) |
| Pokémon **nickname / player-name / rival** edit popup (centered overlay, dismissible, live preview, keyboard button) | ✅ (s13r–s13t); broken out of its ⋮ menu into buttons + Simulated bar (s13z7–z8) |
| **Full keyboard** — name box, Simulated group (Outdoor/Tileset/Grid-Tileset), radio filters, pill grid + tile tooltip, Name/Example toggle | ✅ Reworked + Twilight-iterated (s13v–s13z8) |
| Player/rival name + player ID — atomic commit-on-finish, no hang, no OT corruption | ✅ (s13w) |
| Tileset picker — every tile (incl. last) clickable; no freeze on variable render | ✅ (s13y / s13y2) |

## Recurring Non-Fatal Warnings (harmless)

- `'dllimport' attribute ignored` on `MapDBEntry` etc. — Qt + llvm-mingw shared-lib cosmetic;
  **silenced** via `-Wno-ignored-attributes` (s13c).
- Items "could not be deep linked" / "Values are not correct on sprite X" — pre-existing data.
- On exit: `QDxgiVSyncService not destroyed in time`, `QThreadStorage entry N destroyed…` — benign
  Qt shutdown ordering.

## The "crashes" — two different things

1. **System-wide** Qt-debugger pop-ups (also in Notepad/taskbar/Settings) — environment/Qt-install
   issue, NOT this app. Don't chase.
2. **In-app** use-after-free from QML GC'ing parentless C++ QObjects — **fixed** (s13f/g/h). A
   silent "terminated abnormally" after interaction was this. If a real in-app crash recurs, get a
   project-debugger stack trace. Details: `reference/diagnostic-methods.md`, `sessions/session-log.md`.

