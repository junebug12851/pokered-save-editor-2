# Rejected Approaches

Things that were tried, failed, and should NOT be tried again.

---

## Wrong theory: dataExpandedChanged signal parameter (session 4 theory, session 10 confirmed)

**What was tried**: Removing the parameter from `SaveFile::dataExpandedChanged` signal.

**Why it seemed plausible**: QML was showing `Cannot read property 'player' of undefined`
whenever `dataExpanded` changed. Session 4 theorized that having a parameter in the signal
was confusing Qt's property binding system.

**Why it was wrong**: The signal parameter was never the issue.
`SaveFile::dataExpandedChanged(SaveFileExpanded*)` is correct. **Session 13 strengthened this:**
`SaveFileExpanded`'s own NOTIFY signals (`playerChanged()` etc.) are *parameterless* and the
chain still read `undefined` — so a signal parameter cannot be the cause.

**Do NOT** remove the parameter from this signal or change its signature.

### Correction (session 13): the "rebuild + qRegisterMetaType" explanation was ALSO incomplete

Sessions 10–12 concluded the `dataExpanded = undefined` chain was caused by a truncated
`bootQmlLinkage.cpp` missing `qRegisterMetaType<T*>()` calls, and that a clean rebuild would
fix it. The rebuild happened (the binary was current — exe newer than all sources) and the
registrations were present, yet the **entire chain still read `undefined`**.

The actual root cause is `Q_DECLARE_OPAQUE_POINTER` on the QObject chain types (in
`savefile_autoport.h` and `area/area.h`): it forces
`IsPointerToTypeDerivedFromQObject<T*> = false`, so Qt treats those QObject pointers as opaque
values that QML cannot traverse. `qRegisterMetaType` / `qmlRegisterUncreatableType` do not
override it. Fix = remove the opaque decls for the QObject types and `#include` their full
headers down the chain. See `reference/qt6-patterns.md` and `reference/fix-patterns.md`.

(Truncation repair and the registrations were still worthwhile — they just weren't the cause.)

---

## Using qt_add_qml_module() alongside app.qrc

**What was tried**: Adding `qt_add_qml_module()` to `projects/app/CMakeLists.txt` as part
of modernization.

**Why it seemed right**: Qt 6 documentation recommends `qt_add_qml_module()` for modern Qt
Quick applications.

**Why it failed**: The app already registers QML files via `app.qrc` at `qrc:/ui/app/...`.
Adding `qt_add_qml_module()` registers the same files again at `qrc:/qt/qml/App/...`. The
generated module initialization code conflicts with the existing paths and hangs
`QQuickWidget::setSource()` indefinitely.

**Decision**: Keep QML in `app.qrc` + use `qmlRegisterType()` / `qmlRegisterUncreatableType()`
in `bootQmlLinkage.cpp`. This is sufficient and avoids the conflict.

---

## QSurfaceFormat MSAA

**What was tried**: Setting `format.setSamples(8)` on the default `QSurfaceFormat` in boot.

**Why it seemed right**: Standard anti-aliasing setup for Qt apps.

**Why it failed**: `QQuickWidget` renders to an offscreen FBO, not a regular window surface.
Windows GPU drivers hang 40+ seconds trying to negotiate MSAA for FBO rendering (unsupported).

**Decision**: Remove all `QSurfaceFormat` configuration. Qt Quick handles its own antialiasing.

---

## Calling load() from DB singleton constructors

**What was tried**: The original design (kept from Qt 5) where each DB constructor called `load()`.

**Why it worked in Qt 5**: C++03 static local init had no mutex — re-entry was UB but harmless.

**Why it deadlocks in Qt 6**: C++11 static local init is mutex-guarded. Same-thread re-entry
during initialization deadlocks.

**Decision**: `DB::loadAll()` is the sole caller of each DB's `load()`, invoked after all
singleton objects exist. Do NOT put `load()` back into constructors.

---

## Wrong fix: Pokédex dexInd off-by-one (session 10, reverted)

**What was tried**: Changed `(dexInd+1)` to `dexInd` in `getMonUrl()` and `fixNum()` in
`Pokedex.qml`. Also changed `toggleOne(dexInd)` to `toggleOne(dexInd-1)` and modified
`playerpokedex.cpp` to add bounds checking and emit `dexItemChanged(val+1)`.

**Why it seemed plausible**: Assumed `dexInd` was 1-indexed (Pokédex numbers start at 1).

**Why it was wrong**: `IndRole` in `PokedexModel` returns `*mon->pokedex` which is **0-indexed**
(Bulbasaur = 0, Ivysaur = 1, ...). The original `(dexInd+1)` correctly converts to 1-indexed
filenames ("001-bulbasaur.svg"). Changing to `dexInd` produced "001-ivysaur.svg" for Ivysaur.

**Evidence of wrongness**: After the "fix", build output showed:
```
Cannot open: qrc:/assets/icons/mon-icons/001-ivysaur.svg
Cannot open: qrc:/assets/icons/mon-icons/002-venusaur.svg
```
All reverted. Original code is correct.

**Do NOT** change dexInd arithmetic. `(dexInd+1)` for filenames/display, `toggleOne(dexInd)` 
for direct array access — all correct as originally written.
