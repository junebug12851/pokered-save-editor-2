# Project Status

_Current state only._ For the chronological history of what changed each session and why, see
[`sessions/`](sessions/README.md) (one file per day). For root-cause mechanics see
[`reference/qt-patterns.md`](reference/qt-patterns.md) and [`decisions/`](decisions/architecture.md). For the
commit-by-commit changelog see [`version.md`](version.md).

**Version:** `0.16.1-alpha` — on `dev`, **not yet shipped** (last shipped: `0.15.2-alpha`, `main` @
`ac79c0a`). Single source of truth: repo-root `VERSION`; see
[`reference/versioning.md`](reference/versioning.md). Full `ctest` green (74/74).

> **Releases are MANUAL.** Commit and push to `dev` freely, but `main` only moves when Twilight says
> **"ship"**. Green is necessary, not sufficient. See [`reference/git-workflow.md`](reference/git-workflow.md).

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

The **full keyboard** was rebuilt on 2026-07-11 into an actual **ASDF keyboard deck**: 36 alphanumeric
keys × 8 pages (255 tiles ÷ 36 keys needs 8 pages; Shift/Ctrl/Alt give exactly 8 chords), each cap
carrying one game tile with the key that types it printed in its corner. The tile→key map is C++
(`mvc/fontkeyboard.*` → `brg.keyboard`) and **pinned by `tst_font_keyboard`** — every tile appears
exactly once across the pages + the spacebar. **The base layer is lowercase, Shift is uppercase, and
Caps Lock is a real Caps Lock** (letters only, ignored under Ctrl/Alt, inverted by Shift). Physical
modifiers are **momentary**; only *clicking* a modifier cap or a page button latches. The name field
has two explicit modes — **keyboard mode** (field read-only, Backspace eats a whole tile) and **edit
mode** (ordinary text field; the whole keyboard fades out and goes dead; check applies, cross
discards). The old chip list, filter sidebar, tilemap view and `FontSearchModel` are **deleted**.
Design + the full map:
[`plans/full-keyboard-redesign.md`](plans/full-keyboard-redesign.md); conventions:
[`reference/ui-patterns.md`](reference/ui-patterns.md) → "The full keyboard's DECK".

**Next:** in-app review of the new keyboard (see "Pending rebuilds" below); an end-to-end save/reopen
verification pass; remaining per-control test depth. See [`plans/next-steps.md`](plans/next-steps.md).

## Pending rebuilds / awaiting in-app review

- **The new full keyboard — awaiting Twilight's in-app review (2026-07-11).** Built + screenshot-
  reviewed on every page; `ctest` green. Worth checking live, because they're the things a still PNG
  can't show: **typing** on the real keyboard (and the caps flashing with it); **holding**
  Shift/Ctrl/Alt (momentary — the page must drop back on release) vs. **clicking** the modifier caps
  (latched); **Caps Lock** (letters go uppercase, the number row must stay numbers — and if your OS
  caps light was already on when the screen opened, the deck should correct itself on the first letter);
  the tiles **animating** with the tileset; token-aware **Backspace**; the **shake** when a key won't
  fit; and **edit mode** (pen → the keyboard fades out and goes dead, check applies, cross discards,
  Backspace is character-wise in there).
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
| **Latent landmine: map DB `getToMap()`/`getToSprite()` never resolved** | `db.cpp` `deepLinkAll()`; consumers in `WarpData`/`MapConnData`/`SpriteData`/`AreaMap` | Not a crash today — every consumer is part of the not-yet-wired Maps feature; normal save load reads Area straight from save bytes. **When Maps is enabled**, wiring map-change/re-enabling map randomize will dereference these → crash unless `MapsDB::inst()->deepLink()` is called first (add to `DB::deepLinkAll()`). Confirmed harmless today via `tst_sprite_data` (all 918 sprites resolve once `deepLink()` is called). |
| Randomizer: not-yet-built screens (Maps, Hall of Fame, Options) excluded | `savefileexpanded.cpp`, `worldgeneral.cpp` | **Working within scope as of 2026-06-07.** `randomizeExpansion()` runs end-to-end + is test-covered. Maps/HoF/Options calls are commented out (matching the disabled home tiles), each with a re-enable note. Re-enabling map randomize is gated mainly on calling `MapsDB::inst()->deepLink()` at boot (the type strings + per-call guards turned out to be the same deepLink landmine, not separate defects). |
| Name editors — ongoing review | `name-full/*`, `general/NameDisplay.qml` | Ongoing live tweaks. `NameEdit`/`NameDisplay` are **shared** by player/rival/nickname + the keyboard footer preview — verify all of them on each rebuild. |
| Keyboard caps are cramped at the default 750×480 window | `name-full/KeyboardDeck.qml` | By design it *scales* (key unit = min(width/13.5, height/6.0)), so it's comfortable on a resized window and tight on the default one. Multi-char code labels (`trainer`, `player`) elide at the smallest size. Revisit if Twilight wants the default window bigger, or the header/footer slimmer, to buy the deck more room. |
| Dead menu files (unused after s13z7) | `name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`, `TilesetMenu.qml` | No longer instantiated; left in place + in qrc. Safe to delete later. |

**Intentional (not bugs):** in the storage grid, Pokémon names are always visible below each icon
(dark text, no background). The player **ID** commits on Enter/focus-out (not per keystroke) —
revertible if wanted live.

## Testing

A comprehensive automated suite lives under `projects/tests/` (QtTest + CTest). **Full `ctest` is
green (71/71 on the Qt 6.11 kit).** Library-layer line coverage is at/above 90% (common 100%, db
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
