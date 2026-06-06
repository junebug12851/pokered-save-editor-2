# Session Log — pokered-save-editor-2

Chronological "what just happened" log, **newest first**. `../status.md` holds the *current*
state (health, open issues); this file is the running history of what changed each session and
why. Deep root-cause mechanics live in `../reference/qt6-patterns.md` and `../decisions/`; this is
the narrative.

The recurring theme of sessions 13f–13j: **QML garbage-collects parentless C++ QObjects.** Two
distinct fixes came out of it — `DB::qmlProtect` for DB entries (13f) and `qmlCppOwned()` for
savefile `Q_INVOKABLE` returns (13g–13h). See qt6-patterns "QML garbage-collects parentless C++
QObjects" for the full rule.

---

## Session — sed/mount corruption + full transcript recovery (2026-06-06)

**What happened:** a bulk `sed -i` rename (prior name -> Twilight) run over the Cowork **mounted**
filesystem silently **truncated 55 source files + 8 notes** on the real disk. The mount does
partial/truncated writes under load (and also gives false-truncated *reads*). The damage was real
(the Windows compiler saw the truncation), and git HEAD had been committed *after* the damage, so it
was not a usable restore point.

**Recovery method (worked):** reconstructed every file from prior-session **Cowork chat transcripts**
(`%APPDATA%\Claude\local-agent-mode-sessions\...\.claude\projects\*.jsonl`). Each transcript
records every tool call — `Read` results (full file dumps with line numbers), `Write`/`Edit` inputs,
and `bash` commands. Techniques, by fidelity: (1) a full `Write` or full `Read` capture = exact; (2)
line-number stitching of multiple partial `Read`s; (3) **replaying the captured `Edit` history onto the
`af883fd` clone base** for files only ever edited incrementally; (4) grafting intact disk-head +
`af883fd` tail. Every result was **validated against the most recent transcript reads** and written/
verified with Windows-side PowerShell (the bash mount is unreliable). Full how-to:
`reference/diagnostic-methods.md` -> "Recovering files from Cowork chat transcripts".

**Final outcome — fully recovered, project intact:**
- ~45 source files + all notes: exact recovery.
- `pokemonbox.cpp` (1781 ln) / `pokemonbox.h` (455 ln): reconstructed by replaying the captured Edit
  history + the `::ind.value(` -> `::inst()->getIndAt(` DB refactor onto af883fd; validated line-for-
  line vs recent reads (h 12/12 exact, cpp 120/122 — the 2 diffs are pre-edit older reads). These are
  ~1765-line files lightly edited from af883fd, NOT rewrites.
- The 7 "clone-based" files (settings.cpp, fontsdb.cpp, area.h, areasign.cpp, areasprites.cpp,
  pokemonstoragebox.h, storage.cpp): af883fd + replayed Edits + refactor, then read-corrected. The
  earlier residuals are now **FIXED**: fontsdb `splice()`/`search()`/`getStoreAt`/`getIndAt` restored
  from reads; area.h s13c include-trim restored; settings `previewOutdoor` accessor + areasign/
  areasprites `mapdbentry.h` include restored.
- Notes comprehensively restored to their fullest recent versions (history.md 294, CLAUDE.md 74,
  next-steps.md 81, qt6-patterns.md 496, etc.).
- The prior-name -> Twilight / Apache-2.0 header / gmail-removal work was re-applied consistently.

**Verified final state:** 380 source files, 0 truncated, 0 missing Apache headers, 0 stray name/gmail,
0 INCOMPLETE banners. Recommend a clean build to confirm linkage, then commit to lock it in.

**HARD RULE:** NEVER bulk-edit project files with `sed -i` / `perl -i` / shell redirection over the
Cowork mount — it silently corrupts files. Use the Read/Edit/Write tools or PowerShell
(`[System.IO.File]::WriteAllText`, UTF8-no-BOM) and verify every write (re-read + brace balance).
See `decisions/rejected.md`.

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

Did the full systemic fix for the Q_INVOKABLE-GC bug (Twilight chose `setObjectOwnership` over
parenting — parenting would have fought the existing manual `deleteLater`/cross-box-relocate
lifecycle and risked double-frees; `setObjectOwnership` leaves the C++ lifecycle untouched).

- Added `savefile/src/pse-savefile/qmlownership.h` with `template<typename T> T* qmlCppOwned(T*)`
  → `QQmlEngine::setObjectOwnership(o, CppOwnership)`. (`Qt6::Qml` was already linked into savefile.)
- Wrapped **all 13** `Q_INVOKABLE` `…At()` returns across 12 files: `connAt`, `grassMonsAt`,
  `waterMonsAt`, `signAt`, `spriteAt`, `warpAt`, HoF `pokemonAt`, `itemAt`, `movesAt`, storage-box
  `pokemonAt`, `recordAt`, `boxAt`, `partyAt`. Verified none left unwrapped.
- (The two app-model wrappers `getBoxMon`/`getPartyMon` were already done in 13g.)

Removes the whole class of "QML frees a savefile object I still hold" crashes/decay. Needs a
rebuild. Standing rule added (CLAUDE.md + qt6-patterns): wrap any new Q_INVOKABLE QObject return in
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
  "Pokemon" (Twilight's original spelling). No displayed string altered. Preserve `é` going forward.
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
> stale content. The Read/Edit/Write file tools are the source of truth. (Persisted to Claude's
> cross-session memory, not a project fact.)

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
the chain so Qt d