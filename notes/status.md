# Project Status

_Current state only._ For the chronological history of what changed each session and why, see
[`sessions/`](sessions/README.md) (one file per day). For root-cause mechanics see
[`reference/qt-patterns.md`](reference/qt-patterns.md) and [`decisions/`](decisions/architecture.md). For the
commit-by-commit changelog see [`version.md`](version.md).

**Version:** `0.7.0-alpha` (single source of truth: repo-root `VERSION`; see
[`reference/versioning.md`](reference/versioning.md)).

## Current state (read this first)

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

**Next:** continued review of the name editors; an end-to-end save/reopen verification pass;
remaining per-control test depth. See [`plans/next-steps.md`](plans/next-steps.md).

## Pending rebuilds / awaiting in-app review

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
| **Latent landmine: map DB `getToMap()`/`getToSprite()` never resolved** | `db.cpp` `deepLinkAll()`; consumers in `WarpData`/`MapConnData`/`SpriteData`/`AreaMap` | Not a crash today — every consumer is part of the not-yet-wired Maps feature; normal save load reads Area straight from save bytes. **When Maps is enabled**, wiring map-change/re-enabling map randomize will dereference these → crash unless `MapsDB::inst()->deepLink()` is called first (add to `DB::deepLinkAll()`). Confirmed harmless today via `tst_sprite_data` (all 918 sprites resolve once `deepLink()` is called). |
| Randomizer: not-yet-built screens (Maps, Hall of Fame, Options) excluded | `savefileexpanded.cpp`, `worldgeneral.cpp` | **Working within scope as of 2026-06-07.** `randomizeExpansion()` runs end-to-end + is test-covered. Maps/HoF/Options calls are commented out (matching the disabled home tiles), each with a re-enable note. Re-enabling map randomize is gated mainly on calling `MapsDB::inst()->deepLink()` at boot (the type strings + per-call guards turned out to be the same deepLink landmine, not separate defects). |
| Name editors — ongoing review | `name-full/*`, `general/NameDisplay.qml` | Ongoing live tweaks. `NameEdit`/`NameDisplay` are **shared** by player/rival/nickname + the keyboard footer preview — verify all of them on each rebuild. |
| Full keyboard right-side `DetailView` (text info on hover) | `name-full/DetailView.qml` | Still present + wired alongside the pill tooltip. Confirm it still reads well / whether to keep it. |
| Dead menu files (unused after s13z7) | `name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`, `TilesetMenu.qml` | No longer instantiated; left in place + in qrc. Safe to delete later. |

**Intentional (not bugs):** in the storage grid, Pokémon names are always visible below each icon
(dark text, no background). The player **ID** commits on Enter/focus-out (not per keystroke) —
revertible if wanted live.

## Testing

A comprehensive automated suite lives under `projects/tests/` (QtTest + CTest). **Full `ctest` is
green (67/67 on the Qt 6.11 kit).** Library-layer line coverage is at/above 90% (common 100%, db
~90%, savefile ~90%; app layer is the laggard). The Linux Docker env runs four variants green
(standard / asan+ubsan / xvfb / coverage 89.73%). A QML-load smoke test (`tst_qml_screens`) and a
real-app GUI suite (`tst_gui_*`) gate `main`. A **static-analysis layer** (clang-tidy + cppcheck +
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
