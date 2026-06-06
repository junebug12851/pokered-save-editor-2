# Diagnostic Methods

How to find and fix systemic problems — not individual errors, but categories of failure
that affect many files at once or require investigative tooling to locate.

---

## File Truncation — Detection

Editor crashes can silently truncate source files mid-line. Symptoms: QML components show
blank data, C++ fails to compile or link with incomplete symbols, functions seem to vanish.

**Detection script** (run from repo root):

```python
import os, sys

exts = {'.qml', '.cpp', '.h'}
roots = ['projects/app/ui', 'projects/app/src', 'projects/db/src', 'projects/savefile/src']

for root in roots:
    for dirpath, _, files in os.walk(root):
        for fname in files:
            if not any(fname.endswith(e) for e in exts):
                continue
            path = os.path.join(dirpath, fname)
            with open(path, 'rb') as fh:
                raw = fh.read()
            text = raw.decode('utf-8', errors='replace')
            unclosed = text.count('{') - text.count('}')
            stripped = raw.rstrip(b'\r\n \x00')
            if unclosed != 0 or not stripped.endswith(b'}'):
                print(f"TRUNCATED ({unclosed:+d}): {path}")
```

Notes:
- Some files may have **null-byte padding** at the end (`\x00`) — strip with `rstrip(b'\x00')`
  before the closing-`}` check.
- Some QML files end on `}` but still have brace imbalance — both checks matter.
- C++ headers may legitimately end on `;` not `}`. Tune as needed.

---

## File Truncation — Repair

Once a truncated file is identified, the repair approach depends on how much is missing.

**Short missing tail** — identify exact truncated suffix and append:

```python
filepath = 'path/to/file.cpp'

with open(filepath, 'rb') as f:
    content = f.read()

# Match the known truncated ending:
if content.endswith(b'known_truncated_suffix'):
    with open(filepath, 'wb') as f:
        f.write(content + b'missing_bytes\r\n}\r\n')
```

**Important**: Match the line endings already in the file. Check with:
```python
crlf = content.count(b'\r\n')
lf   = content.count(b'\n') - crlf
print('CRLF' if crlf > lf else 'LF')
```

**Longer missing section** — reconstruct from:
1. The class header (`.h` file) — function signatures tell you what bodies are missing
2. Usage context — how the function is called tells you what it must return/do
3. Companion files — similar files in the same folder follow the same pattern

Example: `mainwindow.cpp` was truncated before `injectIntoQML()` and `ssConnect()`.
The `.h` file had their declarations; the reconstructed bodies were:
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

---

## Boot Hang Diagnosis

App compiles clean but hangs 40+ seconds on startup, window never appears.

**Step 1 — Add timing instrumentation**

In `boot/boot.cpp`, wrap each boot phase with `QElapsedTimer`:
```cpp
QElapsedTimer timer;
timer.start();

// ... boot phase A ...
qDebug() << "[boot] Phase A:" << timer.elapsed() << "ms";
timer.restart();

// ... boot phase B ...
qDebug() << "[boot] Phase B:" << timer.elapsed() << "ms";
```

The phase that shows a very large elapsed time (10,000+ ms) is the hang.

**Step 2 — Per-DB timing** (if hang is in `DB::loadAll()`)

In `db/db.cpp`:
```cpp
auto lap = [&](const char* name, auto fn) {
    QElapsedTimer t; t.start();
    fn();
    qDebug() << "[DB::loadAll]" << name << "—" << t.elapsed() << "ms";
};

lap("Names",      [](){ Names::inst(); });
lap("CreditsDB",  [](){ CreditsDB::inst(); });
// etc.
```

The DB that hangs will show an enormous elapsed time. The one just before it in the list
is the last one to succeed.

**Known hang causes** (all fixed — document here as reference):

| Symptom | Root cause | Fix |
|---------|-----------|-----|
| Hang in `MainWindow` constructor | `qt_add_qml_module()` conflicts with `app.qrc` | Remove `qt_add_qml_module()` from CMakeLists |
| Hang in `QQuickWidget::setSource()` | Same as above | Same fix |
| Hang during QSurfaceFormat setup | `setSamples(N)` hangs Windows FBO context creation | Remove `QSurfaceFormat` setup entirely |
| Hang in one specific DB's `inst()` | Static-init mutex deadlock (Qt 6) | Remove `load()` from DB constructors — see `decisions/architecture.md` |

---

## QML Property Chain Failures (`dataExpanded = undefined`)

**Symptom**: A deep property chain like `brg.file.data.dataExpanded.player.basics.money`
returns `undefined` in QML, causing `TypeError: Cannot read property 'X' of undefined`
on every screen that uses the chain.

**The actual root cause (confirmed session 13): `Q_DECLARE_OPAQUE_POINTER` (or a bare
forward-declaration) on a QObject type in the chain.** It forces
`IsPointerToTypeDerivedFromQObject<T*> = false`, so Qt stores the pointer as opaque and QML can't
read its sub-properties. `qRegisterMetaType` and `qmlRegisterUncreatableType` do NOT override it.
Fix: `#include` the full header at the declaring site, remove the opaque decl. See
`reference/qt6-patterns.md` and `decisions/architecture.md`.

**The fastest way to confirm it (the "natural experiment"):** find a property at the same depth
that *works* and one that *fails*, and compare how their types are declared.
- `brg.file` worked → `FileManagement` is fully `#include`d in `bridge.h`, never opaque.
- `brg.file.data` failed → `SaveFile` was forward-declared + `Q_DECLARE_OPAQUE_POINTER`.
Same property pattern, opposite include treatment ⇒ the opaque/forward-decl is the cause.

**Pinpoint which hop breaks** from the error text: `Cannot read property 'player' of undefined`
for `a.b.c.dataExpanded.player` means `…dataExpanded` evaluated to `undefined` (so `data` is the
opaque hop). The break is at the FIRST opaque/forward-declared hop; fix each hop QML walks.

**Diagnosis order (revised):**
1. Identify the exact undefined hop from the error message (see above).
2. Check that hop's declared type for `Q_DECLARE_OPAQUE_POINTER` (grep `savefile_autoport.h`,
   `db_autoport.h`, and the declaring header's own local opaque block — `area.h` had one) OR a
   bare forward declaration with no `#include`.
3. Fix: `#include` the full header at the declaring header; remove the opaque decl for that type.
   Then continue down the chain — fixing one hop reveals the next.
4. Build performance: only do this for branches QML actually traverses (grep the `.qml` for
   `dataExpanded.X.Y`). Keep untraversed types opaque. See architecture decision.

**Truncation / `qRegisterMetaType` are NOT the cause** (sessions 10–12 thought so; disproven in
13 — the binary was current and registered, chain still undefined). They were still worth fixing.

**The signal parameter is not the cause.** `SaveFile::dataExpandedChanged(SaveFileExpanded*)`
with a parameter is correct. Do not remove the parameter. See `decisions/rejected.md`.

---

## Random Crash / "Decays After Use" / Use-After-Free (QML GC of C++ objects)

**Symptoms**: app runs fine, then after some interaction a feature breaks (rendering goes blank, a
dropdown empties, saving stops) OR it crashes with `0xC0000005` read access violation at an
address like `0xffff…ffff`; an app **restart** fixes it. In the project debugger the crash frame is
in C++ code dereferencing a pointer obtained from a QObject the QML side touched.

**Root cause**: QML garbage-collected a **parentless** C++ `QObject` that C++ still holds. QML
ownership rules:
- Object returned from a **`Q_PROPERTY`** → CppOwnership (safe).
- Object returned from a **`Q_INVOKABLE`/slot** with no QObject parent → JavaScriptOwnership →
  QML's GC may free it once the QML reference drops → C++'s copy of the pointer dangles.

**How to confirm**: get a project-debugger stack trace (Qt Creator's own debugger, not a random
system one). It will point at the exact deref (e.g. `pokemonstoragemodel.cpp:146 mon->isBoxMon()`).
The freed object is whatever a `Q_INVOKABLE` recently handed to QML.

**Fixes (this project's standing solutions)**:
- DB entries (FontDBEntry, etc.): protected en masse at boot by `DB::qmlProtect(engine)` (wired in
  `MainWindow::injectIntoQML`, s13f).
- Savefile `Q_INVOKABLE` returns: wrap the return in `qmlCppOwned()` (`pse-savefile/qmlownership.h`).
  All `…At()` methods done s13h. **Any new Q_INVOKABLE returning a QObject must be wrapped.**

Full rule + reasoning: `reference/qt6-patterns.md` → "QML garbage-collects parentless C++ QObjects"
and "Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT". Note: **system-wide** Qt-debugger
pop-ups (also in other apps) are an environment issue, not this — see `status.md`.

---

## QML Component Not Loading / Blank Screen

If an entire screen shows nothing and the QML error log is quiet, suspect truncated QML files.

**Quick check**: In debug builds, `QQuickWidget` can show QML errors in a dialog.
If it's connected (see `mainwindow.cpp` statusChanged handler), QML parse errors will appear.

If the dialog doesn't show and the screen is blank:
1. Run the truncation detector on all `.qml` files
2. Check `Qt Creator > Application Output` for any QML warnings
3. Add `console.log("loaded")` at the top of `Component.onCompleted` in the suspect file

**Note**: In debug mode, QML files load from the filesystem at runtime (not embedded QRC).
QML changes take effect immediately. C++ changes always require a rebuild.

---

## Linker "Undefined Symbol" for DB Methods

```
undefined symbol: XxxDB::someMethod() const
```

This almost always means one of:
1. Method is declared in the header but has no implementation in the `.cpp`
2. Class is missing the `DB_AUTOPORT` export macro (linker can't see the symbol across DLL boundary)

For (1): add the implementation body.
For (2): add `DB_AUTOPORT` to the class declaration + `#include "./db_autoport.h"` to the header.
The `DB_AUTOPORT` macro must appear immediately before the class name:
```cpp
#include "./db_autoport.h"
class DB_AUTOPORT NamesDB : public QObject { ... };
```
