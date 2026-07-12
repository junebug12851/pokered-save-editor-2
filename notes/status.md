# Project Status

_Current state only._ For the chronological history of what changed each session and why, see
[`sessions/`](sessions/README.md) (one file per day). For root-cause mechanics see
[`reference/qt-patterns.md`](reference/qt-patterns.md) and [`decisions/`](decisions/architecture.md). For the
commit-by-commit changelog see [`version.md`](version.md).

**Version:** `0.17.0-alpha` — on `dev`, **awaiting Twilight's in-app review, then "ship"**. (Previous
release: `0.16.6-alpha`, shipped 2026-07-11.) Single source of truth: repo-root `VERSION`; see
[`reference/versioning.md`](reference/versioning.md). Full `ctest` green (75/75).

> **Releases are MANUAL.** Commit and push to `dev` freely, but `main` only moves when Twilight says
> **"ship"**. Green is necessary, not sufficient. See [`reference/git-workflow.md`](reference/git-workflow.md).

## Current state (read this first)

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

**Not yet drawn:** the player, connection strips bleeding into the border ring, warps/signs/sprites,
tile animation frames (frame 0 only), and the palettes/"contrast" (currently the tileset PNG's greys).

### ✅ And now the actual Game Boy checks our work (2026-07-12)

`tst_emu_parity` boots the **real ROM** in an emulator with one of our saves, reads the **console's own
RAM**, and demands `MapEngine` match it byte for byte. The verdict: the view pointer, the map's blocks, the
24×20 scratch area and the **20×18 screen tiles all MATCH** — `wTileMap` matching means the entire view
pipeline is right, with no sprites or palettes in the way.

It also immediately caught the one thing that *is* wrong: **the border ring**. We fill it with the border
block; the game bleeds the **connected maps' edges** over it (Pallet Town's ring is Route 1 and Route 21,
not trees). Held as an explicit `QEXPECT_FAIL` so it can't be forgotten — implement connection strips and
the test starts passing.

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
