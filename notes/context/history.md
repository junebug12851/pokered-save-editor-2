# Project History

The story of how this project was revived. Useful context for understanding why certain
decisions were made and why the codebase looks the way it does.

> **Before this file:** the 2019–2020 origin story — three from-scratch rewrites, the
> JavaScript detour, and the library/DB refactor that shaped today's architecture — lives in
> [`origins.md`](origins.md). This file picks up where that one ends (the abandoned March-2020
> state). Read `origins.md` first for the full arc.

---

## State When Work Resumed (2026)

The repository had 592 commits, last pushed March 2020. It was mid-refactor — abandoned
not from lack of care, but from complexity overwhelming solo capacity. Twilight wrote this
entirely solo, line by line.

What the code looked like at revival:

- **DB layer**: 11 of 23 classes converted to `inst()` singleton pattern. The other 12 were
  still using old static methods.
- **Build system**: qmake `.pro` files, incompatible with Qt 6.
- **Qt imports**: Qt 5 style throughout (`import QtQuick 2.15`, `QtGraphicalEffects`, etc.).
- **Include guards**: `#ifndef`/`#define` everywhere (Qt 5 style). Should be `#pragma once`.
- **`db.pro`**: Had a Windows backslash path bug — `entries\creditdbentry.cpp` instead of `/`.
- **`mapsearch.h`**: Malformed `Q_PROPERTY` declarations — `READ MapDBEntry*` instead of
  `READ pickRandom` (return type in the READ field instead of function name).

None of these were fatal individually. Together they made the project unbuildable in 2026.

---

## Phase 1 — DB & Common Layer (Compilation)

**Goal**: Get the `common` and `db` sub-libraries compiling cleanly under Qt 6.

Work done:
- Fixed `db.pro` backslash path bug
- Completed DB singleton refactor for remaining 12 classes: MovesDB, MusicDB, ScriptsDB,
  SpriteSetDB, SpritesDB, StarterPokemonDB, TilesetDB, TmHmsDB, TradesDB, TrainersDB,
  TypesDB, PokemonDB
- Converted build system from qmake to CMake (4 `CMakeLists.txt` files)
- Updated all `#ifndef` include guards to `#pragma once`
- Removed deprecated Qt attributes from `boot.cpp`
- Updated all QML imports to Qt 6 style (no version numbers on module imports)
- Migrated `QtGraphicalEffects` → `Qt5Compat.GraphicalEffects` → `QtQuick.Effects`
  with `MultiEffect` (the compat path was a dead end; `QtQuick.Effects` is the real fix)
- Wired all new DB singletons into `db.h` and `db.cpp`
- Simplified `bootDatabase.cpp` to a single `DB::inst()` call

**Result**: `common` and `db` compiling clean.

---

## Phase 2 — Savefile & App Layer (Compilation)

**Goal**: Fix the remaining two sub-libraries. Hundreds of errors, all following the same
patterns — Qt 6 MOC strictness, protected member access via old direct field reads, and
non-static method calls against static APIs.

Major patterns fixed throughout:
- All DB entry fields are protected → must use getters (`entry->getName()`, not `entry->name`)
- All DB singletons have private constructors → must use `::inst()` (not `new XxxDB`)
- Entry type headers live in `entries/` subdirectory — parent DB header only forward-declares
- `getIndAt()` takes exactly 1 argument (old code had 2)
- Qt 6: key modifier operator changed from `+` to `|`
- Qt 6: string split enums moved from `QString::` to `Qt::`

New API added during this phase: `GameCornerDB::getSellPrice()` — the call site referenced
a `sellPrice` field that never existed. The class comment already described the policy
(sell-back is half buy price), so this was added as a proper method.

Also found and fixed:
- `names.h` was missing `DB_AUTOPORT` macro → linker "undefined symbol" for `Names::inst()`
- `savefile_autoport.h` needed `#include <QMetaType>` before `Q_DECLARE_OPAQUE_POINTER` calls
- `savefiletoolset.cpp` had `QVector<int>` / `QVector<var8>` mismatch requiring element-by-element cast

**Result**: All four layers compiling clean.

---

## Phase 3 — Boot Hang Diagnosis

**Goal**: App compiled but hung 40+ seconds on startup, then had to be force-killed.

Three independent root causes discovered, all fixed:

**Hang 1 — `qt_add_qml_module()` conflict**: CMake had both `app.qrc` (QML at
`qrc:/ui/app/...`) and `qt_add_qml_module()` (same files at `qrc:/qt/qml/App/...`).
Generated module init code conflicted with existing QRC paths, hanging
`QQuickWidget::setSource()` inside the `MainWindow` constructor. Fix: removed
`qt_add_qml_module()` entirely.

**Hang 2 — QSurfaceFormat MSAA**: `boot/boot.cpp` set `QSurfaceFormat::setDefaultFormat()`
with `setSamples(8)`. `QQuickWidget` renders to an offscreen FBO — MSAA on the default
format hangs Windows GPU drivers during context creation (40+ second timeout). Fix: removed
`QSurfaceFormat` setup entirely.

**Hang 3 — DB static-init deadlock**: The main hang. Every DB constructor called `load()`.
In Qt 6, static local init is mutex-guarded. Re-entering `inst()` from within its own
constructor deadlocks. Only two entries triggered this directly — `creditdbentry.cpp` and
`gamecornerdbentry.cpp` — but the fix was applied to all 21 DB constructors for safety.
`DB::loadAll()` is now the sole caller of each DB's `load()`.

**Result**: Window appears. App runs.

---

## Phase 4 — QML Functional Testing

**Goal**: Window showed but data fields were blank everywhere. Work through QML bugs
screen by screen.

Key fixes:
- Qt 6 strict ID scoping: IDs no longer leak through file boundaries. `detailView` from
  `FullKeyboard.qml` wasn't accessible in nested child files. Fixed by threading it as
  explicit `property var` through the component hierarchy.
- `FontDBEntry` Q_PROPERTY names had `get` prefix — QML had to write `.getName`. Renamed
  to drop prefix (`.name`, `.ind`, etc.).
- `FontSearch` filter methods were `Q_PROPERTY` — QML couldn't call them as functions.
  Removed Q_PROPERTY, added `Q_INVOKABLE`.
- Added missing `FontsDB` QML methods: `fontAt(ind)` and `fontCount()`.
- Fixed `isNaN()` usage throughout — `=== NaN` is always false in JS.
- Fixed `unsigned int` Q_PROPERTY blank in TextField — requires explicit `.toString()`.
- Fixed `parent.width` null crash in ListView delegates during model reset.
- Added `qRegisterMetaType<T*>()` for 18 pointer types in `bootQmlLinkage.cpp`.

**Still not working after this phase**: `brg.file.data.dataExpanded` still returned
`undefined` in QML — Trainer Card and Pokemon names blank.

---

## The File Truncation Incident

This happened twice at different scales — a smaller version first, then a larger sweep.

**What happened**: During active development, an editor crash caused ~22 QML files and
~15 C++ source files to be truncated mid-line. The truncation was silent — files were
present and non-empty but ended in the middle of a statement or function. This caused:
- QML files to fail parsing (blank components throughout the UI)
- C++ files to fail compilation or link with incomplete symbols
- One file (`bootQmlLinkage.cpp`) was truncated before its closing `}`, meaning it had
  compiled from an older version without the `qRegisterMetaType` calls

Detection: Python brace-balance checker + check that file ends on a closing `}`.
Repair: Python byte-level append — match exact truncated suffix, append missing bytes,
matching the line endings already in the file.

The `dataExpanded = undefined` bug that persisted through QML testing was caused by this:
the binary had been compiled from a truncated `bootQmlLinkage.cpp` without type
registrations. After full truncation repair and rebuild, this chain works correctly.

**The signal parameter was never the cause** — `SaveFile::dataExpandedChanged(SaveFileExpanded*)`
is correct. See `decisions/rejected.md`.

---

## Notes Reorganization

The project's AI-assisted development sessions originally captured notes in a folder called
`ai/`. This was reorganized into the current `notes/` structure — neutral, readable by anyone,
organized by topic rather than session number.

The session-log format was the starting point; this file (and the rest of `notes/`) represents
the synthesized, topic-organized version of everything learned.

---

## Phase 5 — Keyboard Bug Fixes (Session 11)

**Goal**: Fix the three known broken areas in the full-keyboard (name editing) UI.

Bugs fixed:

**SearchContainer.qml — "Control filter clears everything"**: `reSearch()` was calling
`andNormal()` + `andControl()` together, which ANDs to empty (a char can't be both
simultaneously). The real semantic is: within a category axis, multiple Checked items
should OR. Rewrote `reSearch()` with axis-aware logic — type axis (normal/control/picture)
and size axis (singleChar/multiChar/variable) each use OR semantics internally; across axes
they AND. Also fixed a typo: `singleSearchStateState` → `singleSearchState`.

**SearchRoot.qml — character clicks don't append + DetailView never shows**: Two bugs in
one file. (1) `onStrChanged: str = top.str` had the arrow backwards — it reset the value
to the old string instead of propagating the new one up. Fixed to `top.str = str`.
(2) `detailView` property was defined and received but never forwarded to `SearchResults`,
so `searchResults.detailView` was always null and hover never fired. Added
`detailView: top.detailView`.

**FullKeyboard.qml — DetailView top anchor invalid**: `anchors.top: pagedPicker.Top`
had a capital `T`. `pagedPicker.Top` is undefined (valid anchor line is lowercase `.top`).
Fixed to `pagedPicker.top`.

All session 11 changes are pure QML — no rebuild needed.

---

## Phase 6 — Rebuild Compile Errors (Session 12)

**Goal**: The session 10 full repair + session 11 QML fixes put all source in good shape on
paper. Session 12 was the actual rebuild attempt — hitting every compile/linker error that
had been hiding behind the already-broken state.

All errors traced to one root cause: **header truncation** from the session 10 incident had
also silently dropped private sections from several DB class headers. When those files were
left out of the earlier brace-balance repair (which only checked braces, not semantic
completeness), their missing content caused clean compile failures during the rebuild.

**Truncated private sections found and restored:**

- `gamecornerdb.h` — private members `store` and `buyPrice` were gone; `buyPrice` had been
  replaced with a stray method declaration `int buyPrice() const` (wrong on two counts: it's
  a data field, not a method, and the `.cpp` assigns to it). Restored as `int buyPrice = 0;`.
  Also added `friend struct GameCornerDBEntry` (needed for the entry constructor to set it).

- `fontsdb.h` — `store` (QVector) and `ind` (QHash) missing from private section.

- `names.h` — private constructor declaration dropped entirely. Compiler synthesized an
  implicit default constructor, then the `.cpp`'s explicit definition conflicted with it.

- `mapsearch.h` — several methods implemented in `.cpp` but never declared: `getMaps()`,
  `mapAt()`, `qmlProtect()`, `getMapCount() const`. Also missing `results` data member and
  `qmlRegister` private slot. The non-`const` `getMapCount()` left from truncation conflicted
  with the `const` implementation in `.cpp` — replaced with the correct `const` version.

**Missing friend declarations on `ItemDBEntry`:**

Cross-referencing deep-link code against `ItemDBEntry`'s protected fields revealed three
callers that needed friend access: `GameCornerDBEntry` (sets `toGameCorner`),
`PokemonDBEntryEvolution` (sets `toEvolvePokemon`), and `PokemonDBEntry` (sets
`toTeachPokemon`). Added all three to the friend list.

**Linker errors — slots declared but never implemented:**

`DB::qmlProtect()` and `DB::qmlHook()` were declared as public slots in `db.h` but had no
bodies in `db.cpp`. MOC generates a dispatch entry for every slot regardless, so the linker
always requires them. Added both: `qmlProtect` cascades to all 26 sub-databases;
`qmlHook` sets the `"db"` context property on the QML engine context.

**App-layer fixes:**

- `bridge.h` declared `MapSelectModel* mapSelectModel = new MapSelectModel` as a default
  member initializer. `MapSelectModel` requires an `AreaMap*` — no default constructor exists.
  The actual initialization happens in bridge.cpp's constructor mem-init-list. Changed to
  `= nullptr`.

- `settings.cpp` called `mapData->isOutdoor()` which doesn't exist on `MapDBEntry`.
  The outdoor/indoor concept lives on `TilesetDBEntry` via `TilesetType::OUTDOOR`.
  Fixed to `mapData->getToTileset()->typeAsEnum() == TilesetType::OUTDOOR`.

- Several `QFile::open()` `[[nodiscard]]` warnings fixed across `gamedata.cpp`,
  `filemanagement.cpp`, and `bootDatabase.cpp` (the last using `(void)` cast for a
  side-effect-only `DB::inst()` call).

**Result**: All compile and linker errors cleared. Clean rebuild expected to succeed.


**Result**: All compile and linker errors cleared. Clean rebuild expected to succeed.

---

## Phase 7 — The `dataExpanded` Chain, Actually Solved (Session 13–13c)

After the session-12 rebuild, the whole `brg.file.data.dataExpanded.*` chain was **still**
`undefined` — disproving the truncation/`qRegisterMetaType` theory (the binary was current and
the registrations were present).

**Real root cause: `Q_DECLARE_OPAQUE_POINTER` on the QObject chain types.** It forces
`IsPointerToTypeDerivedFromQObject<T*> = false`, so Qt stores those QObject pointers as opaque
values and QML reads their sub-properties as `undefined` — and neither `qRegisterMetaType` nor
`qmlRegisterUncreatableType` overrides it. The giveaway: `brg.file` worked (FileManagement is
fully `#include`d in `bridge.h`, never opaque) while everything opaque-declared beneath it failed.

Fix: de-opaque the traversed QObject types and `#include` their full headers at the declaration
sites so Qt detects them as QObject pointers. Then (session 13c) the includes were **scoped down**
to only the branches QML actually traverses, because including the whole tree into the
everywhere-included `savefileexpanded.h` dragged db headers into every TU and tanked build time.
See `decisions/architecture.md` → "QML Property-Chain Traversal" and `reference/qt6-patterns.md`.

This unblocked everything: Trainer Card, Bag, Pokédex, Pokémon storage all read and persist;
the "Connections: no signal matches" warning flood cleared (the targets were just undefined).

## Phase 8 — UI Polish (Sessions 13b–13e)

With data flowing, the work shifted to UI bugs, most tracing to the **Qt 6 Material 3 control
height** change (taller `TextField`/`ComboBox` than the Qt 5-era hardcoded layouts assumed):
clipped number fields (width ignored padding), overlapping trainer-card fields (fixed-offset
anchors), item-count centering, Pokémon cell not clickable (the `MouseArea` had no `onClicked`),
trainer-screen randomize calling the nonexistent `randomName()`, a null-deref in `TilesetPicker`.
All fixed; see `status.md` session logs. Remaining work is more of the same Material-height polish
plus a few functional gaps (moves dropdown, tileset preview render, hover nickname).

## Current State (as of 2026-06-05, session 13e)

The app is **functional**: data reads and persists across screens, names/badges/pokédex/items all
work, the Pokémon editor opens. Remaining work is UI polish and a handful of small functional
bugs — see `status.md` (current state + open-issues table) and `plans/next-steps.md`.

