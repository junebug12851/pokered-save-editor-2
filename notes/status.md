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
> **the project now compiles, links, runs, AND Twilight confirmed runtime parity with the
> pre-corruption build.** Recovery is effectively complete. ⚠️ **COMMIT/BACK UP THIS STATE** — the only
> committed point (HEAD `2c2d6e5`) is the corrupted one, so until this is committed the working build
> exists only on disk. Key lesson (Twilight): **diagnose against git history,
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

**Documentation pass (started 2026-06-06):** A project-wide doc-comment effort is underway alongside the
Doxygen setup. The **common** layer is fully documented + verified (the style reference). Remaining layers
(savefile, db, app, qml) are queued and tracked in `reference/documentation-progress.md`; conventions in
`reference/doc-comment-style.md`. This is a deliberate multi-session effort (~374 files).

**Build/run notes:** C++ changed earlier in this arc (`FontSearch` 13v/13y, `PlayerBasics` +
`PokemonBox` 13w, `settings.h` colors + `fontsdb.cpp` splice 13y) and **two new QML files were added to
`app.qrc`** (`FlatToggle.qml`, `SimulatedTilesetCombo.qml`, 13z7) — all of those need a **Rebuild**.
Pure edits to existing QML hot-reload in debug. **New QML files MUST be added to `app/app.qrc`** or they
fail at runtime with "Type X is not a type" (this project resolves QML types via the qrc, not directory
scanning).

**Pending Rebuild (s14 — file-load crash fix):** C++ changed (`savefile.cpp`, `filemanagement.cpp/.h`,
`router.cpp`) **and** a new QML file was added (`screens/modal/FileError.qml`, already in `app.qrc`) — so a
full **Rebuild** is required. After building: test (1) clicking a recent file whose path no longer exists →
should silently be gone from the list on next launch, never crash; (2) opening a present-but-truncated/locked
`.sav` → should show the `FileError` modal, and the underlying save stays untouched.

**Pokémon-storage drag & drop (BUILT + tests green 2026-06-09; awaiting in-app review):** C++ changed —
two new `Q_INVOKABLE`s on `PokemonStorageModel` (`dragReorder`, `dragTransfer` in
`mvc/pokemonstoragemodel.cpp/.h`) — plus the QML edit in `fragments/screens/pokemon/PokemonBoxView.qml`.
**Built clean** in the kit dir (`PokeredSaveEditor.exe` relinked, QML re-embedded via `app.qrc` RCC) and
in the repo `build/`; ran `tst_storage_model`, `tst_models`, `tst_pokemonbox`, `tst_move_select_model`,
`tst_roundtrip` — **all exit 0** (byte-fidelity roundtrip included). **No new QML files** (no `app.qrc`
membership change). The storage grid now supports **drag-to-reorder within a pane** and
**drag-to-transfer between the two panes** (drop-to-commit, insert at the drop slot, drag-threshold so a
plain click still opens the editor, **group moves via the existing checkboxes**, dashed drop-slot
placeholder). Convention written up in `reference/ui-patterns.md` → "Drag & drop reordering + cross-pane
transfer". After building, test: (1) drag a mon within a box → lands at the drop slot; (2) drag a mon onto
the other pane's box → transfers + lands at the drop slot (incl. party↔box: party never empties, dest never
overflows); (3) check several mons, drag one of them → the whole checked set moves together; (4) a quick
click still opens the editor; (5) the trailing "+" slot accepts a drop (appends). Watch for: a stray
floating "ghost" after a drop (the `Qt.callLater` defer + overlay reparent are meant to prevent this), and
that the `GridView` doesn't flick instead of dragging (`preventStealing` guards it).

**Drag & drop — iterated in-app with Twilight (2026-06-09, built + storage tests green, app relaunched):**
(1) **drop now commits** — an internal MouseArea drag never auto-fires `DropArea.onDropped`; we drive
`Drag.active` manually and call `Drag.drop()` on release (was the "drag does nothing, no error" bug).
(2) **indicator is now an insertion caret** — a dashed vertical bar in the gap *before* the hovered cell
(overlay, no reflow; Twilight rejected the full-cell box and any live entry-shuffle); the "+" slot's caret
marks "after the last mon, before New". (3) **footer bulk-action bar removed** (`PokemonPane.qml`) — moves
are drag now. (4) **checkbox selection has scoped persistence** (Twilight's rule): survives **only** the
detail-editor round-trip, **clears** on box-switch and on leaving the screen. Mechanics: delegate `CheckBox`
**binds** `checked: itemChecked` (+ `onToggled`) so per-mon checks restore across the model reset that the
editor-close triggers; `switchBox` clears the outgoing box; and **`Pokemon.qml`'s `Component.onDestruction`**
clears both models — `appBody` is a `StackView`, so opening the editor *pushes over* `Pokemon.qml` (stays
alive, no clear) while leaving *pops* it (destroyed → clear). `closeNonModal` fires for both so it can't be
used; `pageClosing` is now inert. (5) **per-cell delete button** is a **round chip `Button`** with real
hover/press states — **dark** chip + red `times` (16px) at rest → fills red w/ white glyph on hover →
darkens on press (90ms fade), `28×28`, bottom-right, hover-or-checked. Checkbox + delete visibility key off
a **`HoverHandler`** (`cellHover`), not `containsMouse`, so reaching for the button doesn't hide it.
`deleteMon(index, group)` — group(checked) deletes the whole checked set, else just that mon. New/changed
C++: `deleteMon`; `switchBox` clears the outgoing box; `pageClosing` inert. Convention in
`reference/ui-patterns.md`.

**Pokémon box-header UX pass (2026-06-10, Twilight-directed; built clean in repo `build/`, KIT-DIR
REBUILD pending for her in-app review):** Reworked the two affordances on the Pokémon screen's box
header that were "clean UI but not clean UX." (1) **Fill circle removed** from the box dropdown —
`PokemonBoxSelectModel::getDecoratedName` no longer prepends `●`/`○`/blank; the `(N/Max)` count it
already shows conveys fullness better (unused `box*Sym` fill members deleted from the `.h`). (2)
**Current-box marker moved + flipped** — was a trailing `◁`, now a *leading* `▷` (where the circle
sat, pointing at the label); non-current rows pad to stay aligned. (3) **The "set current box" dot/
target button → an `Active` `FlatToggle`** on **both** panes: On (filled) when this pane's box is the
game's active box, Off (outlined) otherwise; `enabled: !active` (a save always has exactly one active
box — you click Off→On to activate, can't turn the active box off), **hidden on Party panes**. (4)
**`FlatToggle` color-parameterized** (defaults unchanged → keyboard toggles unaffected) so the header
instance can invert colors to read on the accent bar. Files: `mvc/pokemonboxselectmodel.cpp/.h`,
`fragments/general/FlatToggle.qml`, `fragments/screens/pokemon/PokemonPane.qml`. **C++ changed →
Rebuild required** (no new QML files, no `app.qrc` change). Repo `build/` compiled + linked green
(`pokemonboxselectmodel.cpp.obj` OK, app re-linked, RCC re-embedded QML); rebuild the **kit dir**
(`projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`) for her to test in-app. Convention:
`reference/ui-patterns.md` → "Pokémon storage screen layout".

**Credits screen — now living + AI contributors added (2026-06-10, Twilight-directed):** Added a new
**"AI Assistance"** credits section (Claude — 2026 revival/debug/test/notes; ChatGPT — dev & design help)
and credited **ChatGPT** under **Icons** for the in-app status symbol icons. Data is in
`projects/db/assets/data/credits.json`; the new section is wired into `CreditDBEntry::process()`
(`projects/db/src/pse-db/entries/creditdbentry.cpp`) in display order
(…Framework → **AI Assistance** → Tools Used…). **Rebuild required** — the JSON is baked into `db.qrc`
so the RCC must re-embed it (plus the `process()` C++ change). No `app.qrc` change; no test breakage
(credits tests read `getStoreSize()` dynamically). Going forward the credits screen is to be **kept up
to date by default** — see `CLAUDE.md` → "Keep the Credits Screen Living". (Section order is a UX choice;
Twilight can move "AI Assistance" — it's just the `process()` append order.)

**Items / Bag screen drag & drop (2026-06-10, Twilight-directed; BUILT + full `ctest` green 57/57,
kit-dir rebuilt + app launched for her in-app review):** Brought the same drag interaction the Pokémon
storage grid got to the **items LIST**. New C++ on `ItemStorageModel` (mirrors `PokemonStorageModel`):
`dragReorder`/`dragTransfer`/`deleteItem` `Q_INVOKABLE`s + an `otherModel` pairing wired in
`bridge.cpp`. QML (`fragments/screens/bag/ItemBoxView.qml`, `ItemsPane.qml`): each row is now a
`DropArea` whose `content` reparents to the overlay while dragging (ghost floats across both panes);
**drag is started only from a new left grip handle** (`grip-lines.svg`) because the row's combo/count
controls must keep their clicks (Twilight chose the grip over press-hold); reorder within a list +
**cross-pane transfer** (bag↔PC) via drag; the **footer bulk-action bar was removed** (match Pokémon);
a **per-row delete chip** sits to the right of the count field, `visible: hover || checked` (Twilight's
placement); horizontal dashed drop caret; group drag/delete via the existing checkboxes. Header
check-all `leftMargin` bumped `24→56` to clear the grip column. **C++ changed → Rebuild required** (no
new QML files, no `app.qrc` change). Repo `build/` + kit dir both built clean; full suite green incl.
new `tst_item_storage_model` drag tests (`dragReorder_*`, `dragTransfer_movesToOtherBox`,
`deleteItem_singleAndGroup`). Convention: `reference/ui-patterns.md` → "Drag & drop on the items LIST".
**Awaiting Twilight's in-app review** (watch: grip/checkbox column alignment under the header check-all,
the wide-row ghost when transferring across panes, and that combo/count clicks still work mid-list).

**Items screen — auto-stack on transfer + duplicate-pick guard + owned-total display (2026-06-10,
Twilight-directed; BUILT + full `ctest` green 57/57, kit rebuilt + app relaunched):** Follow-ups to the
items drag work. (1) **Auto-stack, never lossy:** `ItemStorageModel::dragTransfer` folds a moved item
onto an existing same-id dst row (the **last** duplicate) — but **only when the whole amount fits under
99**. If it would overflow it becomes its own **2nd row** (full amount kept, no clamp); if there's no
room for that row, the transfer is **refused** (item stays put). Never clamps/loses items (Twilight's
correction to the earlier clamp-to-99). Stacking-that-fits is allowed even when dst is row-full; group
move/delete unchanged. (2) **Duplicate-pick guard:** the `SelectItem` dropdown greys out item names
already present in the **same** pane (except the row's own current item). (3) **Owned total:** each
dropdown entry now shows the total owned across **both** panes in parens, e.g. `POTION  (x12)`, so the
user sees what they hold elsewhere even when this pane has none. New `Q_INVOKABLE`s on `ItemStorageBox`:
`hasItemInd`, `amountOfInd`. All three **leave pre-existing duplicate save data untouched** — the app
supports same-name rows; not our job to normalize someone's save. **C++ changed (savefile
`itemstoragebox.h/.cpp` + app `itemstoragemodel.cpp`) → Rebuild required.** Tests:
`dragTransfer_autoStacksOntoExisting`/`_stacksOntoLastDuplicate`/`_overflowAddsSecondRow`/
`_overflowRefusedWhenDstFull`/`amountOfInd_sumsAcrossRows`. Convention: `reference/ui-patterns.md` →
"Drag & drop on the items LIST".

**Items delete chip — no rest background + scrollbar clearance (2026-06-10, Twilight-directed; QML-only,
kit rebuilt + relaunched):** The per-row delete chip now has **no background at rest** (just an accent X
that reads on the white row); the red hover-fill / darker-press / white-X mouseover effects are
unchanged. Also fixed the recurring **"button under the scrollbar"** problem: the row layout now reserves
the 16px scrollbar lane (`rowEntry` spans to `right - 16`) and the `SelectItem` combo is `fillWidth`
(capped at its normal width) so it shrinks on tight rows to keep the delete clear. General fix written up
in `reference/ui-patterns.md` → "Scrollable forms" + the items delete-chip bullet. QML-only (hot-reloads;
kit rebuilt anyway for her review). **Follow-up (same day):** the chip used `visible:` which collapsed
its layout slot → the `fillWidth` combo grew/shrank and the **whole row reflowed on every hover**. Fixed
by reserving the slot permanently and fading `opacity` (+`enabled`) instead of `visible` — no resize/
reflow now.

**Items "View All" overview drawer (2026-06-10, Twilight-directed; BUILT + full `ctest` green 57/57, kit
rebuilt + relaunched):** New **View All** button (leftmost in the Bag footer, now an `AppFooterBtn3`)
slides in a **left-edge `Drawer`** with a condensed **alphabetized** table of every item the save holds:
`Item | Bag | Storage`, two right-aligned count columns, **0 hidden** (0 opacity). Backed by a new
read-only `ItemOverviewModel` (`mvc/itemoverviewmodel.*`, `brg.itemOverviewModel`) that aggregates both
item boxes by index (summing duplicate rows), drops both-zero rows, sorts by name (`QCollator`), and
iterates the boxes so glitch items still show. Wired: `app/CMakeLists.txt`, `bootQmlLinkage.cpp`
(registration), `bridge.h/.cpp` (property + construct). Drawer is button-only (`dragMargin: 0`, so a
left-swipe doesn't fight the bag grip handles) and rebuilds `onAboutToShow` (amount edits don't always
emit `itemsChanged`). **C++ changed + new source files + new QML embedded → Rebuild (reconfigure)
required.** Test: `itemOverview_aggregatesSortsHidesZeros`. Convention: `reference/ui-patterns.md` →
"View All overview drawer".

## Open Issues

| Issue | Where | Status / notes |
|-------|-------|----------------|
| ~~`PokemonBox::update(resetType=false, …)` clobbered a dual-type mon's `type2`~~ (+ 2 related type/reset bugs) | `pokemonbox.cpp` `update()`, `isCorrected()`, `isPokemonReset()` | **Fixed 2026-06-08 (Twilight-approved, brought to her before fixing).** Found while coverage-testing `pokemonbox.cpp` (`tst_pokemonbox`). (1) `update()`'s bare `else type2 = toType1->ind` ran on every call with `resetType=false`, overwriting a dual-type mon's `type2` with `type1` (silently dropping the second type; reachable via `maxLevel`/`maxEVs`/`resetEVs`/`reRollEVs`/`manualLevelChanged`; emitted no `type2Changed`). Fixed: type2 (re)derivation is wrapped in `if(resetType)`. (2) `isCorrected()` vs `update()` disagreed for a species whose DB `toType2`==`toType1` (update collapses to `type2=0xFF`; isCorrected demanded `type2==toType2->ind`). Fixed: isCorrected treats a record as dual-type only when `toType2` genuinely differs from `toType1`, accepting `0xFF` or `type1` for single types (faithful to the DB's mixed 0xFF-vs-duplicate storage). (3) the empty-slot bug in `isMaxPP()`/`isMaxPpUps()` (an empty moveID-0 slot counted as "not maxed") propagated into `isHealed()`, so **any mon with <4 moves could never read as healed** (user-facing — the heal indicator — not just `isPokemonReset()`). Twilight asked this not be squashed aside; fixed at source: `isMaxPP`/`isMaxPpUps` skip empty slots (mirroring `isMaxedOut`'s guard), `isPokemonReset` then simplifies to iterate the real initial moves, check `ppUp==0`, reuse `isHealed()`. Regression-guarded in `tst_pokemonbox` (incl. `box_healedWithFewerThanFourMoves`). type2 single-WRITE truth + `isMinEvs` `||` tracked in `plans/next-steps.md`. See `reference/fix-patterns.md`. |
| ~~db map-search/connect bugs (2, found coverage-testing db)~~ | `mapsearch.cpp`, `mapdbentryconnect.cpp` | **Fixed 2026-06-08 (Twilight-approved).** (1) `MapSearch::hasDynamicSpriteSet/noDynamicSpriteSet` dereferenced a null `toSpriteSet` on maps with no sprite set — `!spriteSet` only catches index 0, but `-1` is the "none" sentinel; `hasSpriteSet/noSpriteSet` had the same sentinel bug (wrong results, no crash). Now use `spriteSet < 0` + guard `toSpriteSet == nullptr`. (2) `MapDBEntryConnect::xAlign()` guard was missing `<= 0` (`if(toMap->getWidth())`), so it returned 0 for every real map and its connection-math body was dead — fixed to `getWidth() <= 0`, confirmed against the connection-data formulas Twilight added to `gen1-knowledge.md`. Both gated behind the disabled Maps feature; regression/coverage-guarded in `tst_mapsearch_predicates` + `tst_db_entry_getters2`. See `reference/fix-patterns.md`. |
| ~~Area-family bugs (3, found coverage-testing area/\*)~~ | `areatileset.cpp`, `pokemonbox.cpp`, `areapokemon.cpp` | **Fixed 2026-06-08 (Twilight-approved, brought before fixing).** (1) `AreaTileset::loadFromData` inverted ternary — `(map==nullptr) ? map->getToTileset() : nullptr` crashed on a null map and discarded the real tileset on a non-null map (both branches wrong); fixed to `? nullptr : map->getToTileset()`. Masked today (disabled Maps path). (2) `PokemonBox::newPokemon(Random_Pokedex)` rolled `rangeExclusive(1,151)` so could **never randomize to Bulbasaur** — dex keys are 0-based (probe-confirmed: dex0=Bulbasaur..dex150=Mew, dex151 null); fixed to `rangeExclusive(0,151)`. (3) considered adding an `i < wildMonsCount` bound to `AreaPokemon::setTo`'s array writes, but **Twilight declined** — gen-1 wild tables are fixed at exactly 10, so a defensive bound just implies a case that can't happen; left unbounded (trust the fixed data). Bugs (1)+(2) regression-guarded in `tst_area_logic` (+ `tst_pokemonbox`). See `reference/fix-patterns.md`. |
| ~~`isMinEvs()` used `\|\|` (true if ANY one stat-exp is 0)~~ | `pokemonbox.cpp` `isMinEvs()` | **Fixed 2026-06-08 (Twilight-confirmed).** Changed to `&&` (all five stat-exp must be 0), matching the header's "All stat-exp zero?" and symmetric with `isMaxEVs()`. The `\|\|` had a real UX impact: `StatsTab.qml` disables the **"Reset EVs"** menu on `isMinEvs`, so the action was greyed out whenever a single stat-exp happened to be 0, even on a heavily-EV'd mon. Regression-guarded in `tst_pokemonbox` (`box_reRollEvsAndMaxPpUps`). |
| **Latent (not a live bug): map DB `getToMap()`/`getToSprite()` are never resolved** — `MapsDB::deepLink()` is **not** called anywhere in the program (`DB::deepLinkAll()` omits it; nothing else calls it). So every map's warp/connect/sprite `getToMap()`/`getToSprite()` pointer stays null. | `db/.../db.cpp` `deepLinkAll()`; consumers `WarpData::load(warp)`, `MapConnData::loadFromData`, `SpriteData::load`, `AreaMap::loadFromData`/`setTo` | **Not a crash today** because every consumer is part of the **not-yet-wired Maps feature** (Maps screen greyed out; map randomize disabled — see the randomizer row). Normal save load/expand reads Area straight from save bytes, never from a `MapDBEntry`, so the null pointers are never dereferenced. **Landmine for when Maps is enabled:** wiring `AreaMap::setTo`/map-change or re-enabling map randomize will dereference these → instant crash unless `MapsDB::inst()->deepLink()` is called first (add to `DB::deepLinkAll()` or before the first map edit). Found 2026-06-08 while coverage-testing `WarpData`/`MapConnData` (the `tst_map_fragments` test calls `MapsDB::inst()->deepLink()` itself to exercise these paths; it's stable headless). Aligns with the existing "re-enable map randomize" checklist in the randomizer row. **Confirmed 2026-06-08 (`tst_sprite_data`):** with `deepLink()` called, **all 918 map sprites resolve `getToSprite()` (0 nulls)** and `SpriteData::setToAll`/`randomizeAll` run clean over all 249 maps — so the documented sprite-link "crash" is purely this deepLink-not-called landmine, **not** a SpriteData defect (spritedata.cpp is now 100% tested). The standing fix remains: call `MapsDB::inst()->deepLink()` (e.g. in `DB::deepLinkAll()`) before any map feature dereferences these. |
| Randomizer: not-yet-built screens (Maps, Hall of Fame, Options) excluded from randomization | `savefileexpanded.cpp`, `worldgeneral.cpp` | **Working as of 2026-06-07 (Twilight-authorised).** `randomizeExpansion()` now runs end-to-end and is test-covered (`tst_randomizer` 10-iteration invariants + `tst_verbs` byte-fidelity, both green). Commented out (matching the disabled home tiles — those features aren't wired yet): **Maps** = `area->randomize()` (`SaveFileExpanded::randomize`) + the two map picks in `WorldGeneral::randomize` (`isCity()`/`isType("Outdoor")`); **Hall of Fame** = `hof->randomize()`; **Options** = the `options`/`letterDelay` block in `WorldGeneral::randomize`. (Event Pokémon has no save-randomize path.) Each is clearly commented with a re-enable note. Bugs found+fixed along the way: `MapSearch::isType()` null-deref (deref of null `toTileset`); `HoFPokemon::load()` null-deref (dereferenced `saveFile` before its null check — also crashed `HoFRecord::pokemonNew`/`new HoFPokemon`). **To re-enable map randomize (future map feature) — CORRECTED 2026-06-08:** the earlier claim that this needs "tileset type-string fixes" + per-call null guards was a **misdiagnosis**. Probed (`tst_area_probe`, deepLink called): the type strings ARE literally `"Cave"`/`"Outdoor"` and `isGood()->isType("Cave")`/`isType("Outdoor")` match **60 / 38 maps** (not 0). The 0-match, the `pickRandom()->getInd()` null crashes in `area/*` (AreaWarps), and the `SpriteData::load()`/`getToSprite()` nulls are **all the same deepLink-not-called landmine** — with `MapsDB::inst()->deepLink()` called, `AreaWarps::setTo`/`randomize` run clean over all 249 maps and `SpriteData` over all 918 sprites (both now test-covered: `tst_area_logic`, `tst_sprite_data`). **So re-enabling map randomize is gated mainly on calling `deepLink()` at boot** (e.g. in `DB::deepLinkAll()`), not type-string or per-call repairs. Still to verify when HoF is covered: `HoFPokemon::randomize()`'s `getIndAt("dex"+N)` (note that `dexN` keys ARE valid, 0-based dex0..dex150, so this may be fine too). |
| ~~Save corruption: `recalcChecksums()` wrote the bank-2 box checksum off-by-one~~ | `savefiletoolset.cpp` `recalcChecksums()` | **Fixed + test-verified 2026-06-07.** Was `data[0x5A4B] = getChecksum(0x4000, 0x1A4B)`; corrected to **`data[0x5A4C] = getChecksum(0x4000, 0x1A4C)`** (matches `recalcBoxesChecksums()`, the `.bt` oracle, and real RBY `sBank2AllBoxesChecksum`). The old code clobbered **Box 6's last data byte** (`0x5A4B`, `00`→`C5`) on every save of a *progressed* file and stored the checksum one byte early. `tst_roundtrip` now round-trips `BaseSAV.sav` byte-perfect (`flatten` was already 0-diff; this was the only divergence). The `if(data[0x284C]==0) return` guard (skip box checksums when the game never formatted the boxes) is **intentional and faithful to the game** — confirmed by Twilight. See `reference/fix-patterns.md`. |
| ~~Crash (access violation) in `Daycare::~Daycare()` on an empty Day Care~~ | `daycare.cpp` | **Fixed 2026-06-07.** Destructor did `pokemon->deleteLater()` unconditionally; `pokemon` is null when the Day Care is empty → null-`this` deref. Now guarded `if(pokemon != nullptr)` (matching `reset()`). Masked in the app (SaveFile only destroyed at exit). **Found by `tst_roundtrip`.** See `reference/fix-patterns.md`. |
| ~~Crash on load: opening a missing/unreadable recent file crashed in `setData` (`memcpy` from null)~~ | `savefile.cpp`, `filemanagement.*`, `App.qml`, `FileError.qml`, `router.cpp` | **Fixed (s14, needs Rebuild — C++ + new QML in qrc).** `setData` guards null; `readSaveData` rejects files smaller than 32 KB (at-least, not exact — larger files load their first 32 KB) and captures the real OS/Qt error detail; loads route through `loadData()` (never mutates the save on failure). Startup `pruneRecentFiles()` silently drops unopenable recents (**"prune" not "scrub"** — scrub == `wipeUnusedSpace`). Present-but-unreadable/truncated files raise the new **`FileError`** full-window modal: plain-English reason centred, real technical detail small/muted below (**no fake codes**); closing returns to the prior screen. Twilight-specified UX. ⚠️ Not yet build-verified on her machine. |
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
| Item RNG: no duplicates within a box + Re-Roll sorts (C++; build+test-verified, rebuild app for runtime) | `fragments/itemstoragebox.cpp/.h`, `tst_items_logic.cpp` | **Done — compiled clean + full `ctest` green (57/57) on the real Qt 6.11 kit 2026-06-08 (her request). Rebuild the app binary to pick it up at runtime.** New-item ("+") and Re-Roll RNG can no longer produce a duplicate **within the same list** (bag and PC box are independent — uniqueness is per-list). `ItemStorageBox::itemNew()` now rolls via `randomUniqueInd()` (pool = all non-glitch/non-once items minus those already in the box, 103 real items vs. bag 20 / PC 50, so fills always land) instead of a blind `new Item(true)`; if the box somehow holds every item it adds nothing rather than a dup. `randomize()` (Re-Roll) now calls `sort()` at the end. Regression-guarded: `box_itemNewNeverDuplicates`, `box_randomizeIsUniqueAndSorted` in `tst_items_logic`. Existing round-trip tests unaffected (they overwrite `ind` after `itemNew`). |
| Bag / Items screen layout cleanup (QML-only, hot-reloads) | `screens/non-modal/Bag.qml`, `fragments/screens/bag/ItemsPane.qml`, `ItemBoxView.qml` | **Done — pending Twilight review (2026-06-08).** Removed the dirty hacks: Bag's two panes now split 50/50 via a `RowLayout` (was `Math.trunc(width*0.5)`); the header's magic `width:265` wrapper box is gone (title centered in the bar, count anchored to its right, check-all parked at the bar's left edge — same look, no magic number); every header/footer `IconButtonSquare` dropped its repeated `leftPadding/rightPadding/leftInset/rightInset:0` overrides (use as-is per `ui-patterns.md`); `ItemBoxView`'s empty trailing `Text{bottomPadding:25}` spacer replaced by a `ListView footer`. Twilight's inline perf/insert hack notes kept verbatim. Convention written up in `ui-patterns.md` → "Bag / Items screen layout". |
| ~~Intermittent crash: create/open a stored Pokémon's editor, back out, re-open → use-after-free (mon GC'd)~~ | `fragments/pokemonbox.cpp` `PokemonBox`/`PokemonMove` ctors (+ `mvc/pokemonstoragemodel.cpp` accessors) | **ACTUAL ROOT CAUSE found 2026-06-09 — the box was being GC'd, dragging its mons down.** After the ctor-ownership fix on mons, the crash persisted (now at `data()` line 153 `!mon->isBoxMon()` — a **virtual** call crashing while the scalar `species` read just above it succeeded: textbook freed-object/clobbered-vtable). The freeing agent is the **box, not the mon**: `PokemonStorageModel::getCurBox()`/`getBox()` are **public slots** (QML-callable exactly like `Q_INVOKABLE`), and the new-mon path calls `theModel.getCurBox().pokemonNew()` from QML — handing the parentless `PokemonStorageBox` to JS ownership. When QML GC'd the box, `~PokemonStorageBox()` `deleteLater()`'d **every mon in it**, so all the box's mons dangled regardless of their own CppOwnership; clicking one then hit the freed mon's vtable. (Storage boxes were partly shielded by `Storage::boxAt`'s wrap, but the **party** path `getBox()->party` never went through it — unprotected.) This also explains the create→click specificity: `getCurBox()` is called from QML *only* in the new-mon path. **Fix:** `QQmlEngine::setObjectOwnership(this, CppOwnership)` in the `PokemonStorageBox` ctor (covers all storage boxes + the party via `PlayerPokemon` inheritance, every accessor path). Built into both the kit `savefile.dll` and repo-root build; storage/mon/byte-fidelity tests all green. **Awaiting in-app retest.** Prior layers (mon/move ctor ownership + accessor wraps) kept — all complementary. **STALE-BINARY GOTCHA (2026-06-09):** the first two in-app retests still crashed because they ran a **stale `savefile.dll`** — the editor is launched from the **Qt Creator kit dir** `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/`, NOT the repo-root `build/` dir the automated loop uses. (Tell: the crash showed `PokemonBox::toData()` at the *old* line 1770; after the ctor edits `toData` is at 1781, so that DLL predated the fix.) `PokeredSaveEditor.exe` links `savefile.dll` via its import lib, so editing the DLL body does NOT relink/rebuild the exe (exe mtime stays put) — you must rebuild the **kit dir's** `savefile.dll` and just re-run the exe (it loads the new DLL). Rebuilt the kit `savefile.dll` 2026-06-09 (`setObjectOwnership` confirmed present); awaiting retest. **Lesson: always rebuild `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/` for in-app testing, not just `build/`.** Tests green on the repo-root build (`tst_pokemonbox`/`tst_storage_model`/`tst_move_select_model`/`tst_models` + byte-fidelity `tst_roundtrip`/`tst_verbs`). Two layers. **(1)** Recovery-corruption regression: the `qmlCppOwned()` wrap + include had been stripped from the app-layer `getBoxMon()`/`getPartyMon()` Q_INVOKABLE returns — restored. **(2) Root cause:** `PokemonBox` and `PokemonMove` are **parentless** QObjects (the `parentMon` is a plain member, not a QObject parent) and their ctors had a **commented-out `@TODO`** that was supposed to set `CppOwnership` on construction. So mons/moves only ever got CppOwnership *if/when* handed out through a wrapped accessor — any other exposure path (and the heisenbug timing of QML's GC vs. event-loop) left a window where a mon in `getCurBox()->pokemon` could be freed, then read as a dangling pointer by `hasChecked()` (stack `onReset`→…→`el->property()`) **or** `data()`→`PokemonBox::toData()` (reads `species`). **Realised the `@TODO`:** every `PokemonBox`/`PokemonParty`/`PokemonMove` now calls `QQmlEngine::setObjectOwnership(this, CppOwnership)` in its ctor (static — no engine needed), so all mons/moves are self-protecting from birth regardless of how they reach QML. Containers still free them via `deleteLater()`, so no leak/double-free; the accessor `qmlCppOwned()` calls remain as harmless redundancy. Mirrors the `DB::qmlProtect` precedent for DB entries. See `reference/fix-patterns.md` / `qt6-patterns.md`. |
| Pokémon storage screen layout cleanup (QML-only, hot-reloads) | `screens/non-modal/Pokemon.qml`, `fragments/screens/pokemon/PokemonPane.qml`, `PokemonBoxView.qml` | **Done — pending Twilight review (2026-06-08).** Same cleanup pass as Bag: the two panes now split 50/50 via a `RowLayout` (was `Math.trunc(width*0.5)` + chained anchors); the header's magic `width:265` wrapper `Row` is gone (check-all parked at the bar's left, `SelectPokemonBox` centered, "set current box" dot anchored to its right); every header/footer `IconButtonSquare` dropped its repeated `leftPadding/rightPadding/leftInset/rightInset:0` overrides. **Grid cell:** Pokémon **names are now always visible** below each icon as **dark text (`textColorDark`) on no background** (was a hover-only accent pill); the **edit pill + pen icon was removed** (the cell-wide MouseArea already opens the editor, so it was redundant — also dropped the now-unused `QtQuick.Effects`/`MultiEffect` import). Convention in `ui-patterns.md` → "Pokémon storage screen layout". |
| Home tiles for not-yet-available screens greyed out + non-clickable | `HomeIconsModel.qml`, `IconDelegate.qml` | **Done (QML-only, hot-reloads).** New `disabled` model role; delegate uses `enabled: !model.disabled` + a `MultiEffect` layer (slight desaturate/darken). Disabled: **Maps, Options, Hall of Fame, Event Pokemon.** Convention + tuning dials in `reference/ui-patterns.md` → "Disabled / coming soon Home tiles". |

Intentional (not bugs): in the storage grid, Pokémon **names are now always visible** below each icon
(dark text, no background — 2026-06-08, was hover-only; pending Twilight review). The player **ID**
commits on Enter/focus-out (not per keystroke) — Twilight can revert if she wants it live.

`NameDisplay` / `NameEdit` are **shared** by the player-name, rival, and nickname editors (and
`NameDisplay` is the keyboard footer preview). Changes touch all of them; verify each on a rebuild.

## Testing (added 2026-06-07)

A comprehensive automated test suite now exists under `projects/tests/` (QtTest + CTest, **57
executables, all green** — last full run re-verified 2026-06-08 on the real Qt 6.11 llvm-mingw kit). A
**coverage-gap-fill pass** has brought **all three library layers to/above 90% line coverage:
common 100%, db 90.2%, savefile 90.2%** (this pass also: `pokemonbox.cpp` 72%→94.6%, `spritedata.cpp`
46%→100%, the area family 61-70%→90-100%, `mapsearch.cpp` 47%→100%, fontsearch/fontsdb + the db entry
getters). See `plans/testing.md` for the live per-file gap list and progress. (App layer is the remaining
laggard at ~50-58%; see the app-coverage notes there.) The app logic was extracted into a static `appcore` library
(see `context/architecture.md`) so the **app layer is now unit-testable** (`tests/mvc/tst_models.cpp`
is the first; ~23 models + Bridge/Router remain). It found + drove fixes for **4 real bugs** (Daycare empty-dtor
crash, bank-2 checksum off-by-one save corruption, `MapSearch::isType` null-deref, `HoFPokemon::load`
null-deref). Covers the savefile engine (fields/verbs/Pokémon/items/world/toolset/errors/E2E),
common, db integrity, and the randomizer (within its current scope). Build/run any time with the real
kit: `cmake --build <build> && ctest --output-on-failure` (add `SetErrorMode(0x0003)` first so a
crashing test fails fast instead of popping `qtcdebugger`). Full strategy, measured coverage baseline,
and remaining gaps: **`plans/testing.md`**.

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

