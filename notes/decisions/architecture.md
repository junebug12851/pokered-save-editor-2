# Architecture Decisions

Key choices made during the modernization, with rationale.

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
