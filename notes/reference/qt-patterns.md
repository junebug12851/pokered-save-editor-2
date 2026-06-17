# Qt / QML Patterns & Gotchas

The project's single Qt reference: the **catalog** (a project-lifetime index of every Qt/QML landmine
hit, 2019 → 2026) followed by the **detailed Qt 5 → Qt 6 patterns** (with code) and **case studies**.
When something "impossible" happens at the QML/C++ boundary, assume a framework gotcha before assuming
your own bug — check the catalog here first, then the detailed section, then
[`fix-patterns.md`](fix-patterns.md). When you find a new one, add a row to the catalog and document
the workaround at the call site.

---

## Catalog (project-lifetime index)

A single index of the Qt/QML landmines this project has hit across its whole life. The same kinds of
surprises recur, and a lot of the project's hardest days went into these — many cost hours or whole
days, a few caused rewrites. The detailed fixes live in the sections below, or in
[`fix-patterns.md`](fix-patterns.md) (error → fix), [`../decisions/rejected.md`](../decisions/rejected.md)
(approaches abandoned), [`../decisions/architecture.md`](../decisions/architecture.md) (workarounds
adopted as design), and [`../version.md`](../version.md) (the commit where each historical one was hit).

### Historical (2019–2020)

| Gotcha | What happens | Resolution | Where |
|--------|-------------|------------|-------|
| **`QVariant` can't be a `QHash` value** | Won't compile / won't store as expected | Use `QString`-typed storage instead | `476ba72` |
| **`Q_PROPERTY` won't sit on plain data models** | QML/C++ interop rejects non-Qt-ecosystem object shapes | Reshape models into plain structs registered with the Qt Meta Object System | `bceb15e`, `99188ed`, `9fb2775`, `42da8d7` |
| **Qt Quick URL-encodes but can't decode its own encoding** | Image-provider source strings arrive corrupted | Hex-encode in QML, pass through the URL, decode from hex in C++ (`encodeBeforeUrl`) | `8fe8447` · `../decisions/architecture.md` |
| **Image provider must report the "whole" size or Quick rescales it** | Tiles/text render blurry | Provider returns the exact requested size | `0fb0106` |
| **QML `Loader` + object-chain navigation is unworkable** | Fragile, hard to reason about | Replaced with a C++ `Router` via the `Bridge` | `aba290a`/`cb36cbc`/`d0b4f41` · `../decisions/rejected.md` |
| **QML drag-and-drop is disproportionately hard** | Hours lost, docs mostly C++-only | (2019 era) explicit move buttons; (2026) finally done — see "Drag & drop" in `ui-patterns.md` | `53d69ea` |
| **Static libraries link out-of-order** | Build breaks unpredictably | Build sub-projects as shared libraries (+ export headers) | `5cbd7ff` · `../decisions/rejected.md` |
| **QML `ListView` + C++ MVC is fragile** | Recurring view/model glitches | Known hazard; some rare glitches left deliberately unfixed | `1799397`, `3e9e367` |
| **Naive random index repeats picks** | The same names keep coming up | No-repeat pool that reloads when depleted | `12eb978`/`7eff6bb` |

> Some Era-1/2 "QML only accepts a strict `int`" type pain is the historical cousin of the Qt-6
> `unsigned int` Q_PROPERTY issue below — both come from QML's narrow numeric handling.

### Revival-era (2026) — detail in the sections below / `fix-patterns.md`

| Gotcha | One-line |
|--------|----------|
| **`Q_DECLARE_OPAQUE_POINTER` blocks QML traversal** | Opaque-declaring a real QObject type makes QML read `obj.prop.sub` as `undefined`; `#include` + de-opaque the traversed branches only (build speed) |
| **DB static-init deadlock** | A DB constructor re-entering its own `inst()` deadlocks C++11 static-local init; `loadAll()` is the sole `load()` caller |
| **`qt_add_qml_module()` vs `app.qrc`** | Double-registering QML paths hangs `QQuickWidget::setSource()`; keep `app.qrc` + `qmlRegister*` only |
| **`QSurfaceFormat` MSAA on `QQuickWidget`** | MSAA on the offscreen FBO hangs Windows GPU drivers 40s+; remove the surface-format setup |
| **`unsigned int` Q_PROPERTY blank in `TextField`** | QML needs an explicit `.toString()` |
| **Strict QML ID scoping** | IDs no longer leak across files; thread shared refs as explicit `property var` |
| **`Q_PROPERTY` methods aren't callable from QML** | Use `Q_INVOKABLE` for functions QML calls |
| **`Q_INVOKABLE` returning a parentless QObject gets GC'd** | Wrap returns in `qmlCppOwned()` |
| **Qt 6 Material 3 control heights** | Taller than Qt-5-era hardcoded layouts assumed; pin heights / anchor below |
| **`parent` briefly null in delegates during model reset** | Guard `parent ? parent.width : 0` |
| **`=== NaN` / `isNaN` misuse** | `x === NaN` is always false in JS; use `isNaN(x)` |

### Deliberately accepted quirks (don't "fix" these)

A few Qt/QML behaviors had no clean fix and were consciously left alone. Documented at the call sites;
listed here so nobody burns time re-discovering them.

**Disabling a `Button` while it's hovered/pressed leaves it visually stuck.** `Button.hovered` /
`Button.pressed` are read-only, so flipping `enabled = false` while the cursor is over it (or
mid-press) sticks it rendering in the hovered/down state until the next interaction. The Pokémon
details "Heal" button leaves its `enabled` binding **commented out** (stays enabled); the Pokémart
"Checkout" button accepts the cosmetic glitch (forcing `down` off before disabling was tried, and
a `Connections` hook — same result, because the states are read-only). If you must disable-on-click,
expect this; the cleaner path is to leave the control enabled and make the *action* a no-op.

**A QML call to a 5-bool C++ method only passes 4 args.** In `PokemonDetails.qml`'s "Correct Data"
action, calling `boxData.update(...)` from QML with 5 bools only ever delivered 4. Workaround: call
`update(true,true,true,true)` then `correctMoves()` + `cleanupMoves()` directly. If you extend a
`Q_INVOKABLE`'s argument list and QML seems to drop the last argument, this is why.

**One rare ListView/MVC glitch left unfixed** (`3e9e367`) — most users won't notice it and the fix
risked worse ListView bugs. The QML `ListView` + C++ MVC combo is fragile (see the historical table).

### Meta-lesson

The throughline: **Qt's QML/C++ boundary is where the surprises live** — numeric types, property
exposure, object ownership, URL handling, meta-object requirements.

---

# Detailed patterns (Qt 5 → Qt 6)

Things that worked in Qt 5 that silently break or error in Qt 6.

## C++ Changes

### Key modifier operator
```cpp
// Qt 5 (broken in Qt 6 — operator+ deleted):
Qt::CTRL + Qt::Key_S
Qt::CTRL + Qt::SHIFT + Qt::Key_S

// Qt 6 correct:
Qt::CTRL | Qt::Key_S
Qt::CTRL | Qt::SHIFT | Qt::Key_S
```

### String split enum namespace
```cpp
// Qt 5:
str.split(" ", QString::SkipEmptyParts)
str.split(" ", QString::SplitBehavior::KeepEmptyParts)

// Qt 6:
str.split(" ", Qt::SkipEmptyParts)
str.split(" ", Qt::KeepEmptyParts)
```

### QFile::open() is [[nodiscard]]
```cpp
file.open(QIODevice::ReadOnly);      // warning: ignored [[nodiscard]]

// Correct — and also improves robustness:
if(!file.open(QIODevice::ReadOnly)) return;   // or return nullptr for non-void
```
For singleton bootstrapping side effects (`DB::inst()` etc.) where you can't return, use:
```cpp
(void)DB::inst();   // suppresses nodiscard warning; documents intent
```

### Static local init is mutex-guarded (C++11)
In Qt 5 (C++03), `static T* x = new T;` inside a function had no mutex. Re-entering the same
static init from the same thread during construction was UB that worked in practice.

In Qt 6 (C++11), re-entry of a static local init on the same thread **deadlocks**.

This bit us with the DB singletons: constructors called `load()`, which accessed `inst()` again.
See `decisions/architecture.md` — DB constructors must NOT call `load()`.

### QScopedPointer with const type param
```cpp
// Wrong — const T breaks QScopedPointerDeleteLater (needs non-const QObject*):
QScopedPointer<const FontSearch, QScopedPointerDeleteLater>

// Correct:
QScopedPointer<FontSearch, QScopedPointerDeleteLater>
```

### QSurfaceFormat MSAA with QQuickWidget
`QQuickWidget` renders into an offscreen FBO. Setting `QSurfaceFormat::setDefaultFormat()` with
`setSamples(N > 0)` causes Windows GPU drivers to hang during context creation (seeking MSAA
support for offscreen FBO — typically not supported). Boot hangs 40 seconds.

**Fix: Do not set a custom default format.** Qt Quick manages antialiasing internally.

---

## MOC / Q_PROPERTY Changes

### Forward-declared types in Q_PROPERTY
Qt 6's MOC requires that pointer types used in `Q_PROPERTY` are either:
- Fully defined (header included), or
- Declared with `Q_DECLARE_OPAQUE_POINTER(T*)`

We centralize all opaque pointer declarations in `*_autoport.h` files.
`#include <QMetaType>` must appear before `Q_DECLARE_OPAQUE_POINTER` calls.

### Q_PROPERTY pointer type must be non-const
```cpp
// Wrong — QScopedPointerDeleteLater needs non-const QObject*:
Q_PROPERTY(const FontSearch* search READ search)

// Correct:
Q_PROPERTY(FontSearch* search READ search)
```

### Q_DECLARE_OPAQUE_POINTER silently breaks QML property chains (the big one)
**This was the real cause of the whole `brg.file.data.dataExpanded.*` chain reading as
`undefined` — confirmed session 13, after rebuild + qRegisterMetaType failed to fix it.**

`Q_DECLARE_OPAQUE_POINTER(T*)` expands to:
```cpp
namespace QtPrivate {
  template <> struct IsPointerToTypeDerivedFromQObject<T*> { enum { Value = false }; };
}
```
For a type that *is* QObject-derived, this **lies** to Qt — it forces Qt to treat `T*` as a
plain opaque value, not a QObject pointer. In QML, reading such a property gives an opaque
value with no sub-properties, so `obj.thatProperty.anything` is `undefined`. Neither
`qRegisterMetaType<T*>()` nor `qmlRegisterUncreatableType<T>()` overrides this — the opaque
template specialization wins.

**Diagnostic tell:** `brg.file` worked (FileManagement is `#include`d in bridge.h, never
opaque-declared) but `brg.file.data` and everything under it failed (SaveFile,
SaveFileExpanded, Player, World, … were all `Q_DECLARE_OPAQUE_POINTER`'d in
`savefile_autoport.h` / `area/area.h`). Same property pattern, opposite include treatment.

**Fix:** for a QObject type you need to traverse in QML, do NOT `Q_DECLARE_OPAQUE_POINTER` it.
Instead fully `#include` its header in every header where its pointer appears in a
`Q_PROPERTY` / signal / slot / `Q_INVOKABLE`. With the full definition visible, Qt's MOC/
metatype detects it as a QObject pointer and QML traverses normally. This is the same
approach `db.h` already used (it `#include`s every sub-DB header).

Cascade: the include must be added at **every** hop QML walks
(`filemanagement.h`→`savefile.h`→`savefileexpanded.h`→`player.h`/`world.h`/`area.h`/…→leaves),
because the chain breaks at the *first* opaque/forward-declared hop.

Q_DECLARE_OPAQUE_POINTER is still fine (and needed) for types you only reach through C++ list
models and never as a QML property chain — e.g. the pokemon-storage fragment types are kept
opaque on purpose to avoid a large include web.

`qmlRegisterUncreatableType<T>()` is still wanted (lets QML name the type / use enums), and
`qRegisterMetaType<T*>()` is harmless, but neither is what makes traversal work.

#### Corollary: only #include the branches QML actually traverses (build-time!)
The fix above (replace opaque decls with full #includes) is correct, but **don't over-apply it.**
Including a child header for traversal makes that header part of the include graph of every TU
that pulls the parent. Because the chain root (`savefileexpanded.h` via `savefile.h` via
`filemanagement.h` via `bridge.h`) is included almost everywhere, a heavy sub-tree added there
fans out into the whole project and **dramatically slows the build** (and multiplies warnings
like the `dllimport`-ignored one across every TU).

Rule: a sub-object only needs a full `#include` if QML reads `dataExpanded.…thatObject.<prop>`.
Everything else should stay forward-declared + `Q_DECLARE_OPAQUE_POINTER` (that's what opaque is
*for* — cheap, no fan-out, non-traversable is fine). To find what's actually traversed:
```
grep -rhoP 'dataExpanded\.\K[a-zA-Z]+\.[a-zA-Z]+' --include=*.qml app/ui | sort -u
```
In this project (session 13c) only `player.{basics,pokedex,items,pokemon}`, `area.general`,
`world.other`, `storage.*` are traversed — so `area.h` includes ONLY `areageneral.h`, `world.h`
includes ONLY `worldother.h`, and `savefileexpanded.h` skips `daycare`/`hof`/`rival`. Everything
else went back to opaque. This kept the chain working AND the build fast. The big offender was
`area/areamap.*` pulling db headers (`mapdbentry.h`) into every TU.

The `'dllimport' attribute ignored` warning on `MapDBEntry` etc. is harmless (Qt + llvm-mingw
shared libs) and is silenced project-wide via `-Wno-ignored-attributes` in the root CMakeLists.

### Q_PROPERTY READ vs Q_INVOKABLE
`Q_PROPERTY(T* name READ funcName)` exposes `obj.name` (property read) but NOT `obj.name()`.
Calling `obj.name()` from QML throws `TypeError: not a function`.

If QML needs to call it as a function, use `Q_INVOKABLE` instead:
```cpp
// QML can read as obj.name but cannot call obj.name():
Q_PROPERTY(FontSearch* startOver READ startOver)
FontSearch* startOver();

// QML can call obj.startOver():
Q_INVOKABLE FontSearch* startOver();
```

### Repeater delegates read through transient-null bindings
A `Repeater`/`ListView` delegate that dereferences a screen property (`boxData.movesAt(index)`,
`monMove.moveType`, …) **will** see that property go `null`/`undefined` transiently — during delegate
build, and especially in `tst_qml_screens`, which injects the data property then calls
`completeCreate()` (so bindings evaluate before/around the inject). A static child bound to the same
property often *doesn't* hit this, which is why converting a fixed set of rows to a `Repeater` can
surface warnings the old layout never did. Two distinct failure shapes, both of which **fail the
zero-warning screen-load test**:
- `TypeError: Cannot read property 'x' of null` — guard **every** deref (`monMove ? monMove.x : …`,
  or gate on a `filled` bool). Note **`Menu`/`MenuItem` bindings evaluate eagerly** (text/enabled), so
  they need guarding too even though the menu isn't open.
- `Unable to assign [undefined] to <T>` — a delegate binding reading a tab **constant through the root
  `id`** (`top.rowH`, `top.rowAlt`, `top.boxData`) gets `undefined` mid-build/teardown. Fix: don't read
  through `top` from inside the delegate — **inline the literal** (`Qt.rgba(0,0,0,0.04)`), use a local
  (`row.height`), or coerce a pointer with a ternary (`top.boxData ? top.boxData : null`, which turns a
  transient `undefined` into an assignable `null`).
The Moves tab (`MovesTab`/`PokemonMoveSel`) hit all of this; see `ui-patterns.md` → "Pokémon details —
Moves tab". (Related: `movesMax()`/`movesCount()` are plain methods, not `Q_INVOKABLE`, so QML can't
call them at all — see "Q_PROPERTY READ vs Q_INVOKABLE" above; the Repeater uses the constant 4.)

### Q_PROPERTY names — no "get" prefix for QML
Q_PROPERTY first argument is the QML-visible name. A `get` prefix makes QML code ugly:
```cpp
// QML sees .getName — annoying:
Q_PROPERTY(QString getName READ getName CONSTANT)

// QML sees .name — clean:
Q_PROPERTY(QString name READ getName CONSTANT)
```

---

## QML Changes

### Connections function syntax (required)
```qml
// Qt 5 style (deprecated, can fail silently in Qt 6 for parameterized signals):
Connections { target: x; onFooChanged: doThing() }

// Qt 6 style (correct):
Connections { target: x; function onFooChanged() { doThing() } }
```

### Borderless ComboBox + scrollable form layout (s13n)
**Borderless combo** (clean look, no frame): override the background with a transparent, borderless
rectangle. `flat: true` alone still leaves a frame/fill in Material; this removes it (arrow stays):
```qml
ComboBox { flat: true; background: Rectangle { color: "transparent"; border.width: 0 } }
```
Applied to all combos (7 `Select*` + `StarterEdit`/`Rival`/`NameFullTileset`).

**Scrollable form** (rows that lay out at full height and scroll, not cram): `ScrollView` with a
`ColumnLayout` whose width is pinned to the view's available width:
```qml
ScrollView { id: sv; anchors.fill: parent; clip: true; contentWidth: availableWidth
  ColumnLayout { width: sv.availableWidth; spacing: 8; /* RowLayout rows */ } }
```
**"Label + field" row that stays aligned at any Material height** (by design "option #2"): a `RowLayout`
of `[shaded label box | control]` where the label box uses `Layout.fillHeight` so it grows to the
field height; the control is `Layout.alignment: Qt.AlignVCenter`; a trailing
`Item { Layout.fillWidth: true }` keeps the field left-sized. See `OverviewTab.qml`'s inline
`component FieldLabel`. This is the proper-layout replacement for the old `ShadedBG` + fixed/negative
`topMargin` rows. (by design standing preference: proper layouts, no margin hacks — see
`context/principles.md` "The Quality Bar".)

### Signal handlers are `onX:`, NOT `function onX()` (except inside Connections) (s13l)
A handler written as a **function** directly on a control is silently dead — it declares an unused
method and the signal never invokes it:
```qml
ComboBox {
  function onActivated() { boxData.type = currentValue }   // ← DEAD, never called
}
ComboBox {
  onActivated: { boxData.type = currentValue }             // ← correct (property-binding handler)
}
```
`function onX()` is ONLY correct **inside a `Connections {}` block** (that's the Qt 6 Connections
syntax). On the emitting object itself, you must use `onX:`. This bug was pervasive in the Pokémon
editor (46 handlers) and is the real cause of "controls don't react / menus won't open / sliders
not clickable" while Connections-driven value sync still worked. Tell: the working controls used
`onX:` and the dead ones used `function onX()`.

**Save-integrity corollary — pick the user-only signal.** When converting, prefer the signal that
fires only on user interaction so the handler doesn't run during the programmatic init/sync:
- `ComboBox.onActivated`, `Slider.onMoved`, `AbstractButton.onClicked`, `MenuItem.onTriggered` —
  user-only. Safe.
- `CheckBox.onToggled` — user-only. **Use this, not `onCheckedChanged`**, which also fires when you
  set `checked` programmatically (e.g. `checked = boxData.isShiny` on open) and would run the
  handler's side effects (here `makeShiny()` → rewrites DV bytes) just from viewing data. See
  `context/principles.md` → "Save File Integrity Is Sacred".
- `onTextChanged` fires on programmatic `text = …` init too, so only use it where the write is
  idempotent (parse-and-set-same-value), as the existing number fields do.

### ID scoping is strict (no longer leaks between files)
In Qt 5, an ID defined in a parent file was accessible in child component files.
In Qt 6, IDs are **strictly scoped to the file they're defined in**.

Fix: pass IDs as explicit properties through the component hierarchy:
```qml
// Parent (FullKeyboard.qml):
DetailView { id: detailView }
PagedPicker { detailView: detailView }  // pass it down

// Child (PagedPicker.qml):
property var detailView: null
SearchRoot { detailView: detailView }
```

### ListView/GridView delegate parent briefly null
In Qt 6, delegate items' `parent` can be `null` during model reset.
```qml
// Crashes: width: parent.width
// Safe:    width: parent ? parent.width : 0
```

### Material Design 3 rounded buttons (Qt 6.5+)
Qt 6.5+ switched to Material Design 3 (rounded buttons with shadows by default).
Use `Button` with `flat: true` and zero insets for flat rectangular buttons:
```qml
Button {
  flat: true
  topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
  display: AbstractButton.IconOnly
}
```

### Material 3 control heights break Qt 5-era hardcoded layouts (the recurring UI theme)
Qt 6.5+ Material 3 made `TextField` / `ComboBox` **taller** than the Qt 5 controls this app was
laid out against (roughly ~48–56px now). Anything that positioned controls with hardcoded pixel
offsets assuming the old shorter height now **overlaps or sits wrong**. Symptoms reported across
sessions: trainer-card Money/Coins/Starter overlapping, playtime row crowding, Pokémon editor
"boxes too tall / overlapping", level box "too far down", item count vertically off.

Fixes, in order of preference:
1. **Anchor relative to the neighbor's edge, not a fixed offset from its top.**
   `anchors.top: prev.bottom; topMargin: 5`  (auto-adapts to whatever the field height is)
   — instead of `anchors.top: prev.top; topMargin: 40`. Done for `CardFront.qml` (s13e).
2. **Size number fields for content + padding:** `width: N*font.pixelSize + leftPadding + rightPadding`.
3. **Center text with `verticalAlignment: TextInput.AlignVCenter`**, not fixed `topPadding` nudges.
4. **If a compact field is wanted everywhere**, the shared base is `fragments/general/DefTextEdit.qml`
   (a Material `TextField`, sets `topPadding: 0`). Reducing its height there would tighten many
   screens at once — but it changes field appearance globally, so it's a design decision (they own the UI).

When fixing a "things overlap / too tall" report, suspect this first. It is layout tuning, not a
data bug, and it is NOT caused by the width-padding fixes (width ≠ height).

### Custom ComboBox popup must cap its height or it can't scroll (s13k)
A hand-written `ComboBox.popup` that sizes itself to the whole list cannot scroll. The `Select*`
combos all did:
```qml
popup: Popup {
  implicitHeight: contentItem.implicitHeight   // ← grows to the FULL list height
  contentItem: ListView {
    implicitHeight: contentHeight              // ← ListView == its own content
    ScrollBar.vertical: ScrollBar { }
  }
}
```
With the popup as tall as the content, the ListView's height equals its `contentHeight`, so there
is **nothing to flick** — it just clips at the screen edge and rubber-bands (exactly the "bounces
but won't scroll, rest is cut off"). The ScrollBar is present but has no overflow to scroll.

**Fix — cap the popup height so it's shorter than the content:**
```qml
popup: Popup {
  height: Math.min(contentItem.implicitHeight + 2, 280)   // +2 = padding:1 ×2
  padding: 1
  contentItem: ListView { implicitHeight: contentHeight; clip: true
    ScrollBar.vertical: ScrollBar { } }
}
```
When the Popup has an explicit `height`, it sizes its `contentItem` to the available height; the
ListView is now shorter than its content → it overflows and scrolls. This mirrors Qt's own default
`ComboBox.qml` (`height: Math.min(contentItem.implicitHeight + …, Window.height - …)`). Combos that
use the **default** Material popup already do this and don't need touching. Avoided `Window.height`
here because the controls are hosted in a `QQuickWidget` (the attached `Window` can be null); a
constant cap (280 ≈ 13 rows) is safe and predictable.

### MultiEffect colorization tints by luminance — black icons stay black (s13k)
`MultiEffect.colorization` blends toward `colorizationColor` **scaled by the source pixel's
luminance**. A solid black monochrome SVG (e.g. FontAwesome `pen.svg`, a `<path>` with no fill =
black, luminance ≈ 0) therefore comes out ≈ black no matter what `colorizationColor` is set to.
```qml
// Renders DARK — black source, nothing for colorization to tint:
layer.effect: MultiEffect { colorization: 1.0; colorizationColor: lightColor }

// Correct — brighten to white first, then recolor:
layer.effect: MultiEffect {
  brightness: 1.0                       // push the (black) source to white → luminance ≈ 1
  colorization: 1.0
  colorizationColor: lightColor
}
```
`brightness` is applied before `colorization` in the effect pipeline, so the order works. This is
the general recipe for recoloring any dark/monochrome icon in Qt 6. (Fixed the hover pen in
`PokemonBoxView.qml`.)

### QML garbage-collects parentless C++ QObjects returned to it (must set CppOwnership)
When C++ returns a `QObject*` to QML (via `Q_PROPERTY` READ or `Q_INVOKABLE`) and that object has
**no QObject parent**, QML assigns it `JavaScriptOwnership` and its garbage collector can **delete
it** at any time mid-session. The DB entries here (`FontDBEntry`, move/species/item entries) live
in `QVector`s with no parent, so they are vulnerable.

**Symptom**: everything works at first, then **breaks after "clicking around"** (which is what
triggers a GC pass), and only an **app restart** fixes it (the DB reloads fresh). In this project
it manifested as: all font/name rendering going blank at once (trainer name, full keyboard,
Pokémon hover tooltips), the player name silently no longer saving (name save walks the same font
store via `FontsDB::convertToCode`), and likely some editor dropdowns going empty.

**Fix**: give the engine `CppOwnership` of those objects so QML never deletes them:
```cpp
engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
```
This codebase already has the machinery — `DB::qmlProtect(engine)` cascades `CppOwnership` to every
entry in all 26 sub-DBs (via `Utility::qmlProtectUtil`). **It just was never called.** Wired into
`MainWindow::injectIntoQML()` (s13f), right after the engine is obtained and before `setSource()`.
Savefile-side objects (PlayerBasics, PokemonBox, …) are safe without this because they have QObject
parents (SaveFileExpanded → Player → …), and parented QObjects default to `CppOwnership`.

**Rule of thumb**: any parentless C++ QObject exposed to QML for the app's lifetime needs explicit
`CppOwnership`. If a feature "decays" the longer the app runs, suspect QML GC first.

#### Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT (the crucial distinction)
QML decides ownership of a returned QObject differently depending on how it got it:
- **Q_PROPERTY READ** → QML assumes **CppOwnership** (safe, never GC'd). This is why the whole
  `dataExpanded.player.basics.*` chain is fine even though those objects are parentless.
- **Q_INVOKABLE / slot return** → if the returned object has **no QObject parent**, QML assigns it
  **JavaScriptOwnership** and **will garbage-collect it** once the QML-side reference is dropped.

So `Q_INVOKABLE Foo* somethingAt(int)` that returns a parentless object is a **landmine**: QML
holds it (e.g. a details editor binds it), then drops it (editor closes), then GC frees it — and
any C++ that still holds the same pointer (e.g. the container's vector) now dangles → crash.

**Concrete crash (s13g)**: clicking a Pokémon → `PokemonStorageModel::getBoxMon()` (Q_INVOKABLE)
returns a parentless `PokemonBox*` to the editor → editor closes → GC frees it → next role read
`data() -> mon->isBoxMon()` reads freed memory → `0xffff...ffff` access violation at
`pokemonstoragemodel.cpp:146`. Fixed by `QQmlEngine::setObjectOwnership(mon, CppOwnership)` in
`getBoxMon`/`getPartyMon`.

**This was systemic — now fixed (s13h).** The save tree had ~13 `…At()` Q_INVOKABLE methods all
returning parentless objects QML could GC: `pokemonbox.movesAt`, `itemstoragebox.itemAt`,
`storage.boxAt`, `pokemonstoragebox.pokemonAt`, `areasprites.spriteAt`, `areawarps.warpAt`,
`areasign.signAt`, `areamap.connAt`, `areapokemon.grass/waterMonsAt`, `halloffame.recordAt`,
`hofrecord.pokemonAt`, `playerpokemon.partyAt`. Each was a latent crash/decay in its editor.

**Fix used (option 1 — `setObjectOwnership`, chosen over parenting):** a shared helper
`pse-savefile/qmlownership.h`:
```cpp
template<typename T> static inline T* qmlCppOwned(T* o) {
  if(o) QQmlEngine::setObjectOwnership(o, QQmlEngine::CppOwnership);
  return o;
}
```
Every `…At()` now does `return qmlCppOwned(vec.at(ind));`. (`Qt6::Qml` is linked into savefile.)
**When you add a new Q_INVOKABLE that returns a QObject, wrap it in `qmlCppOwned()`** — that's the
standing rule now.

**Stronger rule for parentless QObjects exposed to QML (2026-06-08): set CppOwnership in the
constructor, not only at accessors.** Per-accessor `qmlCppOwned()` only protects an object *once it's
handed out through that accessor* — it leaves an exposure window for any other path, and the QML-GC-vs-
event-loop timing makes the resulting use-after-free **intermittent** (the worst kind). `PokemonBox` /
`PokemonParty` / `PokemonMove` are parentless (the `parentMon` is a plain member, not a QObject parent)
and their ctors carried a long-dead commented `TODO` for exactly this. Realised it:
`QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership)` in the `PokemonBox` and `PokemonMove`
ctors (static call, needs no engine; `PokemonParty` and `new PokemonBox()` both chain the default-arg
`PokemonBox` ctor) — now every mon/move self-protects from birth. Symptom this cured: create/open a
stored mon's editor, back out, re-open → intermittent crash in `PokemonStorageModel::hasChecked()`
(`…→el->property()`) **or** `data()`→`PokemonBox::toData()` (reads `species` off a GC'd mon). The
accessor wraps stay as harmless redundancy.

**`public slots:` returning a QObject are GC-exposed too — not just `Q_INVOKABLE` (2026-06-09).** QML
calls a public slot exactly like an invokable, so a slot returning a parentless QObject hands it
JavaScriptOwnership the same way. `PokemonStorageModel::getCurBox()`/`getBox()` are public slots, and
`PokemonBoxView`'s new-mon path calls `theModel.getCurBox().pokemonNew()` from QML — exposing the
**`PokemonStorageBox`** itself to GC. **GC'ing a container cascades:** `~PokemonStorageBox()`
`deleteLater()`s every mon it owns, so the box's whole `pokemon` list dangled regardless of each mon's
own CppOwnership — a later **virtual** call on a freed mon (`isBoxMon()` in `data()`) crashed while
scalar reads (`species`) on the same freed memory still "worked" (clobbered vtable, intact data — the
classic UAF tell). Fixed with CppOwnership in the `PokemonStorageBox` ctor (covers the party via
`PlayerPokemon` inheritance). **Lesson: audit `public slots:` for QObject returns the same as
`Q_INVOKABLE`, and protect the container, not just the leaves — anything handed to QML can take its
children down with it.**

Why not parenting (option 2 — `new PokemonBox(box)` so QObject parent-ownership applies)? It would
have collided with this codebase's existing manual lifecycle: the containers already
`deleteLater()` their children in dtor/reset/remove AND **move children between containers**
(box→box relocate). A parent pointer on top of that means a container could auto-delete a child
that now lives elsewhere → the same dangling-pointer crash. Parenting is the cleaner pattern in the
abstract, but here it would need a full ownership refactor; `setObjectOwnership` leaves the C++
lifecycle exactly as designed and just tells QML "C++ owns this."

### qt_add_qml_module() + app.qrc conflict
Using both `qt_add_qml_module()` in CMake AND listing QML files in `app.qrc` causes the same
files to be registered at two different `qrc:/` paths. The generated module init code hangs
`QQuickWidget::setSource()` (synchronous call). **Remove `qt_add_qml_module()` entirely.**
QRC + `qmlRegisterType()` is sufficient.

### Global static object registries → cross-app use-after-free (`0xfeeefeee` in a QML binding)
A QObject subtree that registers every instance into a **static** container (e.g. `ItemMarketEntry`'s
`static instancesCombined` / `instances`, swept by `totalWorth`/`canAnyCheckout`/`totalStackCount`) is a
trap once the GUI tests run: each test spins up its OWN `GuiApp` (its own Bridge + models + entries), but
the static survives **across apps**. On a later test the aggregate sweeps a row freed with the previous
app → use-after-free. The signature: a crash reached from a **QML binding** (`QmlSignalHandler::call` →
`getterQObject`) whose faulting address is `0xfeeefeee…` / `0xffffffffffffffff`, and it reproduces on the
**second** GUI test (e.g. `tst_gui_navigation`'s `newFile`) but not the first. lldb pins it as
`atomic_load<int>` on a `0xfeeefeee` `this` (a freed QObject's refcount/QPointer).

**Fix:** don't aggregate over a process-global registry that outlives a model. Sweep the **current
model's own live list** instead — here, `ItemMarketEntry::activeList` is set to the model's `itemListCache`
at the top of `ItemMarketModel::buildList()`, and the aggregates iterate `*activeList`. Also prefer a
plain **member** over a **virtual** for anything read during such a sweep (a virtual call on a freed entry
derefs a freed vtable — strictly worse). This bit when the unified buy+sell cart started building store
rows in every mode, so the default view's `canAnyCheckout` footer binding swept them. (Build is "release"
in `build/` → no symbols in the QtTest crash dump; `lldb --batch -o run -o bt` gave the freed-pointer
tell.)

### Disabled control keeps its hover highlight (stuck-hover button)
A Material `Button` (any `QQuickControl`) that becomes **disabled while the cursor is over it** keeps
`hovered == true` — Qt stops delivering hover-*leave* events to a disabled item, so the hover background
never clears and the control sits **stuck highlighted**. This was the long-noted Pokemart **Checkout**
button "eyesore" (it disables the instant the cart can't check out, often right under the pointer).

**Fix:** tie hovering to enablement — `hoverEnabled: enabled`. Disabling then flips `hoverEnabled` false,
which forces `hovered` back to false and clears the highlight immediately; re-enabling restores normal
hovering. Done once on the shared `FooterButton.qml` so every footer button is covered (only the Checkout
button actually toggles disabled today). Don't chase this with `Connections`/manual `down` resets (the
earlier attempt) — `pressed`/`hovered` are read-only; gating `hoverEnabled` is the clean root fix.

---

## JavaScript/QML Runtime

### NaN comparison
```javascript
if (x === NaN)   // ALWAYS false — NaN !== NaN in JS
if (isNaN(x))    // Correct
```

### unsigned int Q_PROPERTY in TextField
```qml
// May show blank in Qt 6 for unsigned int:
text: obj.money

// Safe — explicit toString():
Component.onCompleted: { field.text = obj.money.toString() }
Connections { target: obj; function onMoneyChanged() { field.text = obj.money.toString() } }
```

### Pokédex index is 0-based
`IndRole` in `PokedexModel` returns `*mon->pokedex` which is 0-indexed (Bulbasaur = 0).
`(dexInd+1)` in icon filenames and display is CORRECT. Do not subtract 1.

### QML debug mode loads from filesystem
In debug builds, QML files are loaded from disk at runtime (not embedded QRC).
C++ changes always require rebuild. QML changes take effect immediately.

---

## QtGraphicalEffects → QtQuick.Effects

`QtGraphicalEffects` was removed in Qt 6. It moved to `Qt5Compat.GraphicalEffects` as a
transitional module, but `Qt5Compat` is itself a deprecated compat shim — not for long-term use.

**Final fix: use `QtQuick.Effects` with `MultiEffect`.**

```qml
// Old Qt 5 (broken in Qt 6):
import QtGraphicalEffects 1.14
Colorize {
  source: someImage
  hue: 0.0
  saturation: -1.0
  lightness: -0.30
}

// New Qt 6:
import QtQuick.Effects
Image {
  id: someImage
  layer.enabled: true
  layer.effect: MultiEffect {
    saturation: -1.0
    brightness: -0.30
  }
}
```

Key differences:
- `MultiEffect` attaches as `layer.effect` directly on the source item (no separate sibling element)
- `saturation` range: -1.0 (grayscale) to 1.0 (full)
- `brightness` replaces `lightness`: -1.0 (black) to 1.0 (white)
- Conditional effect: use `saturation: (condition) ? -1.0 : 0.0` instead of `visible: false`
- Remove `Qt6::Core5Compat` from CMakeLists once `QtGraphicalEffects` is fully gone

**In this codebase:** `Pokedex.qml` had three `Colorize` usages — all converted to `MultiEffect`
on the Image's `layer.effect`.

## Per-delegate timers/animations: gate on EFFECTIVE visibility, not `visible`

A `Timer { running: someItem.visible }` (or animation) inside a delegate that's a child of a *closed*
`Popup`/`ToolTip` still runs — an item's `visible` property is its own flag and stays `true` even when
an ancestor (the closed popup) isn't rendering it. So a grid of N delegates each carrying a hidden
animated preview runs N timers at once. In s13x this pegged the CPU (hundreds of font-image renders/sec)
and froze the full keyboard.

Fix: don't let the heavy item exist while hidden. Build it with `Loader { active: tip.opened }` (ToolTip
has an `opened` property) so only the visible one instantiates. Same idea for any expensive per-delegate
content (image providers with `cache:false`, animations): gate creation on the thing actually being on
screen, not on a `visible:` binding.

---

# Case study: player / rival name-edit hang (the two-way-bind feedback loop)

**Status: FIXED in s13w** (root-caused s13v). Reported symptom: clicking/editing the rival
name (and "probably the player name too") makes the app crash/hang; she suspected "a circular reference
from a hack to get the rival and player name variables working." This is the canonical example of why
heavy `WRITE` setters + a manual QML two-way bind + per-keystroke writes are a trap.

## What happened

`PlayerBasics::playerName` is `Q_PROPERTY(QString playerName READ getPlayerName WRITE
fullSetPlayerName NOTIFY playerNameChanged)`. `fullSetPlayerName(val)` did, on **every** write:
(1) `getNonTradeMons()` — scans the whole party + every storage box + every mon, calling
`hasTradeStatus()` per mon; (2) sets the name; (3) `fixNonTradeMons()` — `changeOtData(true,this)` on
every non-trade mon (rewrites OT name bytes); (4) emits `playerNameChanged()` **unconditionally — no
`if(val == playerName) return;` guard.**

The QML two-way bind (`PlayerNameEdit.qml`) wrote `playerName = str` on `onStrChanged` and a
`Connections.onPlayerNameChanged` wrote back `str = playerName`. So **each keystroke** triggered a
full save-wide OT scan + rewrite. On a populated save that looks like a freeze — and it **writes OT
bytes on every keystroke**, violating byte-fidelity (`../context/principles.md` → "Save File Integrity
Is Sacred"). `Rival::name` was a plain `MEMBER` property (cheap), so the rival editor itself was light.

## The fix (s13w)

**C++:** added an **equality guard** at the top of `fullSetPlayerName`/`fullSetPlayerId`
(`if(val == playerName) return;`) — no-op when unchanged → no rescan, no OT writes, no signal, and it
kills the two-way bind's feedback loop. Made `PokemonBox::changeOtData`'s adopt-player-OT branch
**idempotent** (only assign + emit for a field that actually differs).

**QML — commit on finish, not per keystroke (the real cleanup):** `NameDisplay` gained a
**`committed(string val)`** signal, emitted when an edit session *finishes* (the quick-edit popup or
the full keyboard closing; a `suppressNextCommit` flag avoids a double-write on popup→keyboard
hand-off). `PlayerNameEdit.qml` / `Rival.qml` persist on **`onCommitted`** (atomic) with null guards on
the `dataExpanded` chain; `PlayerIdEdit.qml` does the same via `onEditingFinished`.

**Why per-keystroke was not just slow but WRONG:** the OT cascade captures "owned" mons by comparing
their OT to the *current* player name. Typing char-by-char meant an intermediate value (e.g. "AB"
while typing "ABC") could momentarily equal a **traded** mon's OT and sweep it into the owned set,
permanently rewriting that traded mon's OT — a byte-fidelity violation. One atomic commit eliminates
this. (The editors are modal, so commit-on-close always lands before any save.)

**Shared-component note:** `NameDisplay` is shared by player / rival / nickname. The nickname
(`OverviewTab.qml`) deliberately still writes on `onStrChanged` — its `nickname` setter is a cheap
`MEMBER` with no cascade. Only player/rival moved to `committed`. The player **ID** now applies on
Enter/focus-out instead of live per digit (revertible to `onTextChanged`, but that reintroduces the
rare intermediate-collision risk).
