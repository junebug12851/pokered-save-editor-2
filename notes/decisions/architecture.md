# Architecture Decisions

Key choices made during the modernization, with rationale.

---

## i18n via Qt Linguist, not a custom stringfile (2026-06-13)

**Decision**: Added a full Qt Linguist translation pipeline (`qsTr`/`tr` + `translations/*.ts` →
`.qm` embedded at `:/i18n`, `QTranslator` installed in `boot.cpp`) for **UI chrome only**, rather
than a hand-rolled "stringlist/stringfile in a folder." Source language is en_US; English ships.
Full mechanics: `reference/i18n.md`.

**Why**: For a Qt app, a custom string table reinvents what the framework provides and would be a
hack by this project's bar. Qt's system still gives the desired clean `translations/` folder (just
`.ts`/`.qm`), is editable in Qt Linguist, and needs zero custom loading code. It opens the door to
future locales (add one `.ts` line) at near-zero ongoing cost, and untranslated strings fall back to
the source — matching the graceful-degradation principle.

**Notable sub-decisions**:
- **Router screen titles** use `QT_TRANSLATE_NOOP("Screen", …)` and are translated at point of use
  (`router.cpp`), because `loadScreens()` runs at boot *before* the translator is installed —
  translating eagerly there would freeze them to English.
- **Display vs. value** strings are kept distinct: only display text is wrapped; logical keys
  (`previewTileset !== "Cavern"`, internal mon names, `"phony"`) are left as English literals so
  translation can't desync logic. Mandated license/attribution text and tiny format prefixes
  (`"L"+level`) are intentionally not wrapped.
- **Game-data names** (Pokémon/move/item) are explicitly **out of scope** — they're region/encoding
  -bound save data, a separate and larger effort, not Qt-translation material.
- **Language switching is deferred** (Twilight, 2026-06-13): with only en_US and no in-app
  Options/Settings screen yet, there's nothing to switch to. The `ui/language` setting is the hook;
  revisit when a second locale and the Options screen exist. (lupdate/lrelease both work fine — the
  initial "lupdate hang" was just stacked Windows UAC dialogs, not a tooling issue.)

---

## Trainer randomize grants badges linearly from badge 1 (2026-06-08)

**Decision**: `PlayerBasics::randomize()` (the Trainer Card "Re-Roll" button) now awards a
**random count** of badges earned **linearly in gym order** — always at least the first badge
(Boulder), with the earned badges contiguous from badge 1. Previously it force-enabled the 4
HM-relevant badges (Thunder/Cascade/Soul/Rainbow) and left the other 4 off.

**Why**: Gen 1 progression is strictly linear — you earn badges in gym order. A random bitmask
(or a fixed HM subset) could grant a later badge without the earlier ones, which is an
impossible game state. "Random from badge 1, linear only" mirrors how a real save progresses:
roll `rangeInclusive(1, 8)` and set `badges[0..N-1]`. Twilight-specified.

**Tests**: `tst_randomizer` invariants updated to assert badge 0 is always set and the earned
badges are contiguous from badge 1 (no gaps). `tst_player_basics` + `tst_verbs` byte-fidelity
still green.

---

## Removed qt_add_qml_module()

**Decision**: Removed the entire `qt_add_qml_module()` block from `projects/app/CMakeLists.txt`.

**Why**: The app uses `app.qrc` to embed QML files at `qrc:/ui/app/...`. Adding
`qt_add_qml_module()` registered the same files again at `qrc:/qt/qml/App/...`. The generated
module initialization code then tried to pre-register QML components under the `App` URI,
conflicting with the existing QRC paths. This caused `QQuickWidget::setSource()` to hang
indefinitely (it's a synchronous call inside the `MainWindow` constructor).

**Alternative**: `app.qrc` + `qmlRegisterType()` / `qmlRegisterUncreatableType()` in
`bootQmlLinkage.cpp` is fully sufficient for this codebase. No module URI is needed.

---

## Removed QSurfaceFormat MSAA

**Decision**: Removed `QSurfaceFormat` setup from `boot/boot.cpp`.

**Why**: The original code set `format.setSamples(8)` for anti-aliasing. `QQuickWidget`
renders into an offscreen FBO, not a regular window surface. Windows GPU drivers time out
(40+ seconds) when negotiating MSAA support for offscreen FBOs — the feature simply isn't
commonly supported. Qt Quick manages its own antialiasing internally and doesn't need a
custom default format.

---

## DB Constructors Must Not Call load()

**Decision**: Removed `load()` from all 21 DB singleton constructors.

**Why (the Qt 6 deadlock)**:
The original design had every DB singleton call `load()` in its constructor, making
`SomeDB::inst()` fully self-contained. This worked in Qt 5 because C++03 static local
initialization had no mutex — re-entering `inst()` from within its own constructor was
technically undefined behavior but harmless in practice.

Qt 6 (C++11) uses a mutex to guarantee thread-safe static local initialization. If the same
thread calls `inst()` while it's already inside the `inst()` static initialization (e.g., from
within the constructor that `inst()` is running), it tries to re-acquire the mutex and
deadlocks.

**The chain**: `DB::loadAll()` → `CreditsDB::inst()` → `new CreditsDB` (mutex locked) →
`CreditDBEntry::process()` → `CreditsDB::inst()` (tries to re-lock same mutex) → **DEADLOCK**.

**The fix**: `DB::loadAll()` is now the sole caller of each DB's `load()`, called after all
`inst()` calls complete. The behavior is equivalent since `DB::inst()` always runs before any
other DB access.

**Safe vs unsafe entry patterns**:
- Safe: entries that call `inst()` only in `deepLink()` (all singletons exist by then)
- Safe: entries that call a *different* DB's `inst()` during `load()`
- Unsafe: entries that called their own parent DB's `inst()` during `load()`:
  - `creditdbentry.cpp` — called `CreditsDB::inst()->store.append()`
  - `gamecornerdbentry.cpp` — called `GameCornerDB::inst()->buyPrice = price`

---

## DB Singleton with inst() Pattern

**Decision**: Converted all 23 DB classes from static methods/members to proper
`QObject` singletons with `static T* inst()`.

**Why**: Qt 6 MOC requires that types used in `Q_PROPERTY` are either complete or declared
opaque. The old static pattern had private data inaccessible to other classes. The singleton
pattern lets us:
- Expose data to QML via `Q_PROPERTY`
- Protect data with proper access control
- Enable the `db.h` aggregate to expose all DBs via properties

---

## CMake Build System

**Decision**: Converted from qmake `.pro` files to CMake.

**Why**: Qt 6 strongly prefers CMake. The qmake tooling is in maintenance mode. CMake gives
better IDE integration, cleaner dependency declarations, and is required for modern Qt features.
The old `.pro` files are kept in the repo but not used.

---

## GameCornerDB::getSellPrice() — New API (Session 7)

**Decision**: Added `getSellPrice()` returning `buyPrice / 2` to `GameCornerDB`.

**Why**: The call site in `itemmarketentrymoney.cpp` referenced `GameCornerDB::sellPrice` which
never existed. The class comment already described the sell-back policy: "regular casinos give
you an even exchange, you get the exact amount back. But in the Poke-World I want to follow the
global sell-back mechanics whereby you get half back." `getSellPrice()` just makes that explicit.
Also exposed as `Q_PROPERTY` so QML can read it directly.

---

## Cross-Deep-Link Friend Declarations on ItemDBEntry

**Pattern**: DB entry structs that write back to `ItemDBEntry`'s protected fields during
`deepLink()` must be listed as `friend` in `itemdbentry.h`.

**Current friends** (as of session 12):
```cpp
friend class ItemsDB;
friend struct MapDBEntrySpriteItem;
friend struct GameCornerDBEntry;     // writes toGameCorner
friend struct PokemonDBEntryEvolution; // writes toEvolvePokemon
friend struct PokemonDBEntry;          // writes toTeachPokemon
```

**Why friends rather than setters**: `ItemDBEntry`'s protected fields are write-once,
set during deep-linking, and never changed again. A setter would imply mutability.
Friends make the limited cross-class access explicit and compile-enforced.

**When adding a new deep-link that writes to ItemDBEntry**: add `friend struct NewEntryType;`
to `itemdbentry.h`.

---

## QML Property-Chain Traversal: Include, Don't Opaque (Sessions 13–13c)

**Decision**: For any QObject type that QML must traverse through a `Q_PROPERTY` chain or receive
from a `Q_INVOKABLE` return, **fully `#include` its header** at the declaration site. Do NOT use
`Q_DECLARE_OPAQUE_POINTER` for such types. Keep opaque only for types QML never traverses.

**Why**: `Q_DECLARE_OPAQUE_POINTER(T*)` forces
`QtPrivate::IsPointerToTypeDerivedFromQObject<T*> = false`. For a real QObject type, Qt then stores
the pointer as an opaque value, and QML reads `obj.thatProperty.sub` as `undefined` — even though
the C++ object is valid, and regardless of `qRegisterMetaType` / `qmlRegisterUncreatableType`.
This was THE cause of the long-standing "dataExpanded chain = undefined" bug. (A forward-declared
QObject pointer without any opaque decl is ALSO non-traversable, for the same incomplete-type
reason — the type must be *complete* at the declaring header's MOC TU.)

**But scope the includes tightly (performance)**: the chain root (`savefileexpanded.h` via
`savefile.h` via `filemanagement.h` via `bridge.h`) is included almost everywhere, so a heavy
sub-tree added there fans out into every TU and tanks build time (it dragged db headers like
`mapdbentry.h` into the whole project — session 13c). **Only include the branches QML actually
traverses; keep everything else forward-declared + opaque.** Find the real set with:
```
grep -rhoP 'dataExpanded\.\K[a-zA-Z]+\.[a-zA-Z]+' --include=*.qml app/ui | sort -u
```

**Current traversed set** (include + de-opaqued): SaveFile, SaveFileExpanded, Player, Area, World,
Storage, PlayerBasics, PlayerPokedex, PlayerPokemon, ItemStorageBox, AreaGeneral, WorldOther,
PokemonBox, PokemonParty, PokemonMove, PokemonStorageBox.
**Kept opaque** (forward-declared, untraversed): Daycare, HallOfFame, Rival, the other 10 Area
children, the other 9 World children, PokemonStorageSet.

**Compile note**: removing an opaque decl is compile-safe even where the type is only a
forward-declared signal/slot parameter (Qt 6 MOC tolerates that — proven by the successful build).
Includes are only strictly required at `Q_PROPERTY` / `Q_INVOKABLE`-return sites for *traversal*.

Full mechanism + the "include only traversed branches" corollary live in
`reference/qt6-patterns.md`. The rule is also in the top-level `CLAUDE.md` "Critical Things".

---

## Documentation: Doxygen for C++, no generator for QML (Session 2026-06-06)

**Decision**: Generate C++ API docs with **Doxygen** (+ Graphviz, doxygen-awesome theme).
Do **not** run any documentation generator over the QML — document QML with plain
human-readable comments only.

**Why not qdoc** (the obvious Qt choice, and the only first-party tool that documents QML):
qdoc doesn't associate a comment with the declaration beneath it the way Doxygen does — it makes
you restate what you're documenting with topic commands (`\fn bool SaveFile::load(...)`,
`\qmltype Pokedex`, `\qmlproperty ...`). That turns in-code comments verbose and machine-like.
Twilight's UX-first bar applies to the *source itself*: comments must read cleanly for a developer.
qdoc fails that test.

**Why Doxygen**: it reads the next line of code, so comments stay human and Markdown-friendly
(`QT_AUTOBRIEF` on → first sentence is the brief, no commands needed). It's the long-standing,
rock-stable C++ standard.

**The trade accepted**: Doxygen can't document `.qml`. So there is **no generated doc site for
QML** — by choice. For a solo project, a clickable QML doc site adds little; readable inline QML
comments give the real value. Revisit qdoc only if contributors ever need a browsable QML reference.

**Footprint**: one root `Doxyfile`, a vendored theme under `docs/doxygen-awesome/`, generated
`docs/html/` (git-ignored). Build: `doxygen Doxyfile`. Details + comment-style examples in
`reference/documentation.md`.

**Reaffirmed same session**: Twilight reconsidered switching to qdoc for full-project (QML)
coverage, then chose to **stay on Doxygen**. Deciding factors: qdoc has **no Markdown** — its
prose pages use qdoc's own markup (`\page`/`\section1`/`\c`/`\l`), so it can't ingest the
`notes/*.md` and would force any custom pages out of Markdown; it also reintroduces the verbose
`\fn`/`\qmltype` comment style. Doxygen keeps comments human AND ingests Markdown pages natively
(curated `.md` pages can be added to `INPUT` later). Accepted cost: no generated QML doc site.
**Update (same session)**: Twilight then asked for the `notes/` to be **built into the Doxygen
output and cross-linked**. So `notes` is now in the Doxyfile `INPUT`: the Markdown notes render as
doc-site pages and their relative links resolve page-to-page. The notes remain plain, readable
Markdown (no Doxygen markup added) — they are still the living dev notes, just now also published
and threaded together. The new `notes/systems/` set is the architecture deep-dive hub. (This
supersedes the earlier "notes stay out of the generator" decision.)

---

## Error Handling Philosophy (Owner's Decision)

**Decision**: Debug builds show blocking `QMessageBox` for QML errors. Release builds degrade
silently.

**Why**: Owner's philosophy is "Sims 2-style graceful degradation" — the app should never
crash or block the user with unexpected error dialogs in production. QML errors in debug are
visible to developers; in release they log to `qCritical()` only.

Implementation: `mainwindow.cpp` connects `QQuickWidget::statusChanged` before `setSource()`.

---

## File Truncation Recovery (Session 10)

**Situation**: ~22 QML files and ~15 C++ source files were found truncated mid-line, likely
from a crashed editor. This caused all data screens to show blank and prevented compilation.

**Detection**:
```python
with open(f, 'rb') as fh:
    content = fh.read()
text = content.decode('utf-8', errors='replace')
unclosed = text.count('{') - text.count('}')
stripped = content.rstrip(b'\r\n \x00')
if unclosed != 0 or not stripped.endswith(b'}'):
    print(f"TRUNCATED: {f}")
```
Also: null-byte padding at end of file — strip with `rstrip(b'\x00')`.

**Repair approach**: Python byte-level append — identify exact truncated suffix, append only
the missing bytes. Match line endings of the existing file (`\n` vs `\r\n`).

**Key reconstructed functions** — `mainwindow.cpp` was truncated before these; they can be
reconstructed from the class declaration in `mainwindow.h`:
```cpp
void MainWindow::injectIntoQML() {
    auto* context = ui.app->rootContext();
    bridge = new Bridge(file);
    context->setContextProperty("brg", bridge);
    MainWindow::engine = ui.app->engine();
}
void MainWindow::ssConnect() {
    connect(file, &FileManagement::pathChanged, this, &MainWindow::onPathChanged);
    connect(file, &FileManagement::recentFilesChanged, this, &MainWindow::reUpdateRecentFiles);
}
```

**FontFilter type** — `fontsdb.h` had a private `splice(QVector<int>&, QString, FontFilter)`
where `FontFilter` was a private typedef lost in truncation. Since all call sites pass `int`
indices and the method is private, replaced with `int position` directly.

**mapsearch.h Q_PROPERTY READ methods** — when a header is truncated and the moc was generated
from the original, the moc will reference methods that are missing from the current header.
Check each `Q_PROPERTY(T name READ funcName)` and verify `funcName` is declared. In this case
`getMapCount()`, `isCity()`, `notCity()` were missing and needed to be added.

---

# Original Architecture Rationale (2019–2020)

The decisions above are from the 2026 revival. The ones below are the *founding* choices that
shaped the codebase in the first place — reconstructed from the commit history (`version-notes.md`,
`context/origins.md`). They explain **why the structure exists at all**, so revival work
extends the original intent instead of accidentally undoing it.

## The four-library split (common / db / savefile / app)

**Decision** (`7d5199b`, the "large-scale refactor"): break the monolithic app into separate
libraries — `common` (shared helpers: random, utility, types), `db` (the reference game
databases), `savefile` (save parsing + the expanded object model), and `app` (the executable +
QML UI + bridge/router/models/engines).

**Why**: separation of concerns and reuse — the databases and the save model have no business
depending on the UI, and isolating them keeps each layer testable and comprehensible. The build
order (common → db → savefile → app) reflects the dependency direction.

**Made shared, not static** (`5cbd7ff`): static linking hit an out-of-order link bug; switching
to shared libraries fixed it, at the cost of needing an export macro per library — hence the
`common_autoport.h` / `db_autoport.h` / `savefile_autoport.h` headers. A separate `core`
library was tried (`e682f2e`) and dropped as unnecessary (`01f51d1`); shared helpers live in
`common`. See `decisions/rejected.md`.

## DB-plus-entry + deep-linking + in-memory index

**Decision** (the long 2019 "Completed/Index X Data" runs, then the 2020 `XxxDB`/`XxxDBEntry`
refactor starting with Credits in `198effb`): each reference database is a `XxxDB` singleton
holding a list of `XxxDBEntry` rows loaded from a JSON asset, with an in-memory index for fast
lookup and **deep links** cross-referencing related entries (a Pokemon → its moves/TMs, a map →
its connections/warps/wild encounters, etc.).

**Why**: the game's data is densely interrelated; resolving those relationships once at load
time (deep-linking) lets the rest of the code — and QML — traverse them cheaply by pointer
instead of repeatedly searching by id. The first link was proven in `98660a1`; the pattern then
covered every database. The `entries/` subdirectory and the central `DB` aggregate that
bootstraps `loadAll`/`indexAll`/`deepLinkAll` both come from this design. See `systems/db.md`.

## The expanded-data object model + byte-exact flatten

**Decision** (the Dec 2019–Jan 2020 "Completed Expanded Data / X" run): a loaded save is parsed
into a tree of editable C++ objects — area, world, player, storage, daycare, Hall of Fame,
rival, and the shared fragments (Pokemon box/party, item/sprite/sign/warp/map-connection data).
The *shape* of this tree was first worked out in the JavaScript era (`context/origins.md`,
Era 2) and recreated in C++.

**Why**: editing raw save bytes directly is error-prone and un-QML-able. The expanded model
gives every save field a typed, bindable object the UI can read and write. Crucially, writing
back **flattens only the bytes that changed** — byte-exact fidelity was a value from the start
(the early corruption bugs, e.g. `e20c167` and the party-save fix `cb6fc99`, were chased down
precisely because a stray byte is unacceptable). This is the origin of the `flattenData`
contract the revival treats as sacred. See `systems/savefile.md` and
`context/principles.md` → "Save File Integrity".

## Bridge + Router (QML ↔ C++)

**Decision** (`d0b4f41`): a single `Bridge` object exposed to QML as the `brg` context property,
with a C++ `Router` driving all screen navigation. This replaced the failed QML
`Loader`/`Pages.js` approach (`decisions/rejected.md`).

**Why**: navigation logic and the data graph belong in C++ where they can be reasoned about and
tested; QML just renders and calls into `brg`. The router emits navigation signals that the QML
`StackView` turns into push/pop — keeping the view dumb and the logic central. See
`systems/app.md`.

## Font/tileset image providers + the `encodeBeforeUrl` hex trick

**Decision** (`22baf52` TilesetEngine/Provider, `49e47c3` FontPreviewProvider): render names and
tiles in the real Game Boy font through Qt `QQuickImageProvider`s, fed by `image://font/...` and
`image://tileset/...` URLs from QML.

**Why**: the name editor and previews must look *in-game*, not like hex. A C++ image provider
can composite the font/tileset bitmaps faithfully and hand QML a finished image.

**The hex-encoding workaround** (`8fe8447`): Qt Quick URL-encodes the image-source string but
**can't decode its own encoding**, so the name string is hex-encoded in QML, passed through the
URL, and decoded from hex in C++ — the `Utility::encodeBeforeUrl` helper. This is an accepted,
documented workaround for a genuine framework defect, not a hack of convenience (see
`context/principles.md` → "No hacks — but accept necessary workarounds"). Also note the provider
must report the correct "whole" image size or Qt Quick rescales and blurs it (`0fb0106`). See
`systems/app.md` and `reference/qt-gotchas.md`.

## The `Random` helper

**Decision** (`70f9207`): wrap `QRandomGenerator` behind a small `Random` interface in `common`,
and route all randomization through it (`e1543c3`).

**Why**: a single seam for randomness keeps call sites clean and makes the randomization
feature's behavior consistent and tunable in one place.
