# Revival Sessions — the `s13` series (legacy combined log)

This is the pre-per-day session narrative from the 2026 revival, preserved verbatim from the old
single-file `session-log.md`. These sessions were tracked by number (`s13`, `s13b` … `s13z11`, plus
`s14`) rather than by calendar date — the work was never committed (HEAD was the corrupted commit
until the 2026-06-06 recovery), so it has no reliable per-day dates. Newest first.

Dated work from 2026-06-06 onward lives in the per-day files (`2026-06-NN.md`).

The recurring theme of sessions 13f–13j: **QML garbage-collects parentless C++ QObjects.** Two
distinct fixes came out of it — `DB::qmlProtect` for DB entries (13f) and `qmlCppOwned()` for
savefile `Q_INVOKABLE` returns (13g–13h). See `../reference/qt-patterns.md` → "QML garbage-collects
parentless C++ QObjects" for the full rule.

---

## Session 13z10–13z11 — Trainer Card layout pass (QML-only, no rebuild)

(Pre-corruption; the last pre-corruption work was s13z11.) agreed, live-iterated:
(1) restored the **centered grey box** (the `SwipeView` had been changed to `anchors.fill: parent`
in the refactor → back to centered `500×250`); (2) **compacted every field** via one
`CardFront.fieldH` (28) knob + tighter row margins, fixing the too-tall Qt 6 Material boxes;
(3) **fixed the playtime clock** — narrowed each digit field to ~2 chars (`PlaytimeEdit.digitPad`)
and stopped the row sizing to the Material implicit height (it now pins to `fieldH` and vcenters the
`:`), which had made the digits ride high; (4) **vcentered `DefTextEdit`'s built-in label** (it
lacked `verticalAlignment`, so ID/Money/Coins labels rode high).

## Session 13j — Pokémon box hover name finally renders (+ pen icon restored)

Hover name on the Pokémon box grid was still blank for **all** mons (nicknamed or not), even after
the accent pill painted. Data was fine (the edit screen shows the name). Root cause: the Material
`Button`'s built-in icon+text label was **not rendering its text** at the button's small fixed
size (`height: 20`, `padding: 0`, `display: TextBesideIcon`). A plain `Text` in the same delegate
(the `L##` level badge) renders fine, so:

- Replaced the Button's built-in label with an explicit `contentItem` — a centered `Row` of the
  pen icon + a `Text` bound to `editBtn.text`. Renders reliably.
- Pen SVG tinted to `textColorLight` via `MultiEffect { colorization: 1.0 }` (added
  `import QtQuick.Effects`).

File: `PokemonBoxView.qml`. Pure QML (no rebuild).

## Session 13i — Hover name attempt 1: accent pill + species fallback

First pass at the blank hover name. The label is a `flat` Material button; flat buttons don't
paint `Material.background`, so there was no accent fill and (we thought) no contrast for the light
text. Removed `flat: true` and added `Material.elevation: 0` (keep the accent fill, no drop
shadow). Result: the accent pill painted but the name STILL didn't show → **ruled out contrast**
(the real fix was the contentItem in 13j). Also added a species-name fallback to
`getMonNickname()` so un-nicknamed mons show their species (matches the in-game display). Pure QML.

## Session 13h — Systemic Q_INVOKABLE-GC fix (`qmlCppOwned` across all `…At()`)

Did the full systemic fix for the Q_INVOKABLE-GC bug (`setObjectOwnership` was chosen over
parenting — parenting would have fought the existing manual `deleteLater`/cross-box-relocate
lifecycle and risked double-frees; `setObjectOwnership` leaves the C++ lifecycle untouched).

- Added `savefile/src/pse-savefile/qmlownership.h` with `template<typename T> T* qmlCppOwned(T*)`
  → `QQmlEngine::setObjectOwnership(o, CppOwnership)`. (`Qt6::Qml` was already linked into savefile.)
- Wrapped **all 13** `Q_INVOKABLE` `…At()` returns across 12 files: `connAt`, `grassMonsAt`,
  `waterMonsAt`, `signAt`, `spriteAt`, `warpAt`, HoF `pokemonAt`, `itemAt`, `movesAt`, storage-box
  `pokemonAt`, `recordAt`, `boxAt`, `partyAt`. Verified none left unwrapped.
- (The two app-model wrappers `getBoxMon`/`getPartyMon` were already done in 13g.)

Removes the whole class of "QML frees a savefile object I still hold" crashes/decay. Needs a
rebuild. Standing rule added (CLAUDE.md + qt-patterns): wrap any new Q_INVOKABLE QObject return in
`qmlCppOwned()`.

## Session 13g — Clicking-Pokémon crash root-caused (Q_INVOKABLE ownership)

Project-debugger stack trace: crash at `pokemonstoragemodel.cpp:146` `return !mon->isBoxMon();`,
read access violation at `0xffff…ffff` (freed `mon`). Cause: `getBoxMon`/`getPartyMon` are
`Q_INVOKABLE` and return a **parentless** `PokemonBox`; QML gives Q_INVOKABLE returns
`JavaScriptOwnership`, so after the details editor closes QML's GC frees the mon, leaving a
dangling pointer in the box's vector → next role read crashes. Fixed those two with
`QQmlEngine::setObjectOwnership(mon, CppOwnership)`; identified the systemic ~13-method version
(fixed in 13h). The earlier "random terminated abnormally with no output" crashes were this same
use-after-free.

## Session 13f — Name-disappearing glitch = QML GC of `FontDBEntry`; `DB::qmlProtect` wired in

Repro clues: all font rendering (trainer name, full keyboard, hover tooltips) goes blank at once
after "clicking around", name **stops saving** at the same moment, and only an app reboot fixes
it. Root cause: QML was garbage-collecting the shared `FontDBEntry` objects (parentless, in
`FontsDB`'s vector) → dangling pointers → all font rendering AND name saving break (saving runs
the name through the same font store via `FontsDB::convertToCode`); reboot reloads the DB. The fix
machinery `DB::qmlProtect(engine)` existed (cascades `CppOwnership` to every entry in all 26
sub-DBs) but **was never called** — wired it into `MainWindow::injectIntoQML()` (+`#include
<pse-db/db.h>`). Needs a rebuild.

## Session 13e — Trainer-card randomize, TilesetPicker null, overlap, item centering, PP width

Pokémon click works (the earlier "crash" was interference). Fixes:
- **Randomize name on Trainer screen** — `NameDisplayMenu.qml` (tileset version) still called the
  nonexistent `randomName()`; → `randomExample()`. (No-tileset version was fixed in s10.)
- **`TilesetPicker.qml:111` `Cannot read property 'name' of null`** — guarded the `fontAt()` result.
- **Coins/Starter/Money overlap (`CardFront.qml`)** — fields anchored with fixed offsets from the
  previous field's *top* (40/25px) < the Qt 6 Material field height → overlap. Switched to anchor
  below the previous field's *bottom* (auto-adapts).
- **Item count not vertically centered** — replaced fixed `topPadding: 13` with
  `verticalAlignment: TextInput.AlignVCenter`.
- **Moves PP field too narrow** (`PokemonMoveSel.qml`) — width now includes field padding.

## Session 13d — Pokémon cell click; vertical-overlap diagnosis; é note

- **Pokémon click did nothing** — the cell `MouseArea` had no `onClicked` (only the hover button
  did). Added `onClicked` to the cell (guards placeholder "+" slots). `PokemonBoxView.qml`.
- Diagnosed the trainer-card/playtime vertical overlap as the **Qt 6 Material control-height**
  issue (hardcoded offsets assume the shorter Qt 5 field). Not caused by the width fixes.
- `é` in "Pokémon" only ever appeared in one code comment; the app's tooltips already use plain
  "Pokemon" (the original spelling). No displayed string altered. Preserve `é` going forward.
- `val is not declared` warning is harmless (the `changeStr(string val)` signal declares `val`;
  Qt 6 just deprecates injected handler params).

## Session 13c — Build slowdown + `dllimport` spam: trim over-includes

Session 13 had included the *entire* expanded tree into `savefileexpanded.h` (included almost
everywhere), dragging the heavy `area` sub-tree (whose `.cpp`s pull `mapdbentry.h`/db) into nearly
every TU → ballooned compile time; full rebuilds surfaced every pre-existing `dllimport` warning
at once. Fix: include only the branches QML traverses — `area.h` → just `areageneral.h`, `world.h`
→ just `worldother.h`, dropped `daycare`/`hof`/`rival` from `savefileexpanded.h`; untraversed types
back to forward-decl + `Q_DECLARE_OPAQUE_POINTER`. Added `-Wno-ignored-attributes` to root CMake to
silence the harmless `dllimport` warning. Chain still works (verified no traversed type is opaque).

> Tooling note: the bash sandbox mount lags behind the editor's writes and shows false truncation /
> stale content. The Read/Edit/Write file tools (and PowerShell) are the source of truth.

## Session 13b — Chain works; pokemon types de-opaqued; number-box widths; About guard

Rebuild confirmed the chain fix — data flows into the UI. Then:
- **Pokémon box click didn't open details** — `getBoxMon()/getPartyMon()` (Q_INVOKABLE returning
  `PokemonBox*`/`PokemonParty*`) feed `PokemonDetails.qml`'s typed `property PokemonBox boxData`,
  which only works with real QObject pointers. Those pokemon-storage types were still opaque
  (s13 kept them opaque) → de-opaqued them + included their headers at the return/property sites.
  **Nothing is opaque anymore** in the savefile chain.
- **Number boxes too narrow** (playtime, item counts) — `width: 2 * font.pixelSize` ignored the
  TextField padding → `2 * font.pixelSize + leftPadding + rightPadding`.
- **`About.qml:31` `width of null`** — added `parent ? parent.width : 0` guard.

## Session 13 — REAL root cause of `dataExpanded = undefined`: `Q_DECLARE_OPAQUE_POINTER`

After the s12 rebuild the whole `brg.file.data.dataExpanded.*` chain was **still** `undefined`,
disproving the s10–12 truncation/`qRegisterMetaType` theory (binary was current and registered).

**Root cause:** `Q_DECLARE_OPAQUE_POINTER(T*)` forces
`QtPrivate::IsPointerToTypeDerivedFromQObject<T*> = false`, so Qt stores those QObject pointers as
opaque non-QObject values and QML reads their sub-properties as `undefined` — and neither
`qRegisterMetaType` nor `qmlRegisterUncreatableType` overrides it. Proof (natural experiment):
`brg.file` worked (FileManagement fully `#include`d in `bridge.h`, never opaque) while everything
opaque-declared beneath it failed. This also produced the "Connections: no signal matches" flood
(the targets were just `undefined`).

Fix: removed the opaque decls for the traversed QObject chain types and added full `#include`s down
the chain so Qt sees them as real QObject pointers. See `../reference/qt-patterns.md` →
"`Q_DECLARE_OPAQUE_POINTER` breaks the QML property chain".
