# System Map: Macro Overview

How the whole machine fits together, grounded in the actual code. For the quick
version see [../context/architecture.md](../context/architecture.md); this is the
in-depth account.

## What the program is, in one breath

A Game Boy Pokemon Red/Blue save file is a fixed 32 KB (`0x8000`) blob of packed
bytes. This app loads that blob, **expands** it into a tree of friendly C++
objects, lets the user edit those objects through a Qt Quick (QML) UI, then
**flattens** the tree back into the blob -- touching only the exact bytes an edit
requires. The editing happens on the expanded tree; the raw blob is the source of
truth on disk.

## The four layers (build order)

```
common  ->  db  ->  savefile  ->  app
```

Each lower layer is a shared library the next one links against. Dependencies only
ever point left.

| Layer | Role | System map |
|-------|------|-----------|
| **common** | Foundation types and helpers: fixed-width integer aliases, `Random`, `Utility`, the QML-ownership guard. | [common.md](common.md) |
| **db** | The *game's* static data (every Pokemon, move, item, map, font glyph, ...) loaded from JSON assets into singleton databases. Read-only reference data, independent of any save file. | db.md _(pending)_ |
| **savefile** | The byte-exact engine: parse the raw save, expand it into objects, flatten it back. Knows nothing about UI. | savefile.md _(pending)_ |
| **app** | The Qt executable: boot sequence, the `Bridge` that exposes everything to QML, image/engine providers, QML list models, and the QML UI itself. | app.md _(pending)_ |

## The boot sequence

`main()` is deliberately tiny -- it delegates to `boot()` and execs the returned
app (`app/src/main.cpp`, `app/src/boot/boot.cpp`):

```
main()
  -> boot()
       -> bootDatabase()      // DB::inst() : construct + load + index + deep-link every game DB
       -> bootQmlLinkage()    // register every C++ type/enum with the QML meta-system
       -> Router::loadScreens()  // pre-load the QML screen set
       -> createApp()         // QApplication, then `new MainWindow()` -> show()
  -> app->exec()
```

Order matters: the databases must be fully built before any QML type that reads
them is registered, and registration must precede the window that instantiates QML.

- **bootDatabase()** is a one-liner: `(void)DB::inst();`. The whole game-data graph
  comes up behind that single call, in dependency order, inside [the DB
  aggregate](../context/architecture.md). The "don't call load() in DB constructors"
  rule that makes this safe in Qt 6 is in
  [../decisions/architecture.md](../decisions/architecture.md).
- **bootQmlLinkage()** is long and deliberately repetitive: Qt's meta-system refuses
  templates/helpers here, so every type is spelled out three times. It does two jobs:
  `qRegisterMetaType<T*>()` for every pointer that appears in a `Q_PROPERTY` chain
  (or QML reads it as `undefined`), and `qmlRegisterType` / `qmlRegisterUncreatableType`
  for every enum (creatable) and object (uncreatable) QML touches.

## The runtime object spine

Once running, a single context property `brg` is QML's doorway to all of C++. The
core chain QML traverses:

```
brg                                  Bridge          (app/src/bridge/bridge.h)
  .file                              FileManagement  (savefile) - open/save/recent files
    .data                            SaveFile        - holds the raw 32 KB + the expansion
      .data            var8*         raw save bytes (the on-disk source of truth)
      .dataExpanded    SaveFileExpanded - the editable object tree:
        .player.basics.money         PlayerBasics::money  (a Q_PROPERTY leaf)
        .player / .area / .world / .storage / ...
      .toolset         SaveFileToolset - low-level byte read/write over the raw data
```

`Bridge` also hangs many **non-save** objects off the same `brg` root: the game
databases used by pickers (`fonts`, `randomPlayerName`, ...), the `Settings`, the
`Router`, and a fleet of QML list models (`pokedexModel`, `mapSelectModel`,
`itemMarketModel`, ...) that adapt C++ data into Qt item-model form for QML views.

For QML to read straight through that pointer chain, every QObject type in it must
be a **complete type** at the property's MOC translation unit -- fully included, not
forward-declared, not `Q_DECLARE_OPAQUE_POINTER`'d. That single rule (and why it was
the cause of the long-standing "undefined chain" bug) is documented in
[../decisions/architecture.md](../decisions/architecture.md) and
[../reference/qt6-patterns.md](../reference/qt6-patterns.md).

## The data lifecycle (raw <-> expanded <-> UI)

```
   disk file (32 KB)
        |  readSaveData()                      FileManagement
        v
   raw var8* data  ----expandData()---->  SaveFileExpanded tree
        ^                                        |
        |  flattenData()  (writes only the       |  Q_PROPERTY bindings
        |   strictly-necessary bytes)            v
   raw var8* data  <----------------------  QML edits the objects
        |  writeSaveData()
        v
   disk file (32 KB)
```

`SaveFile` (`savefile/src/pse-savefile/savefile.h`) owns the three pieces -- the raw
`data`, the `dataExpanded` tree, and a `SaveFileToolset` for byte-level access --
and exposes the verbs: `expandData()`, `flattenData()`, `resetData()`,
`eraseExpansion()`, and `randomizeExpansion()` (the constrained "still playable"
randomizer).

## The byte-fidelity contract (the project's prime value)

`SaveFile::flattenData()` is documented in-code as: *"Flatten expansion back to the
save file, overwriting its current contents with only data that's strictly
necessary. A critical rule."* That is the heart of the project: an edit flips only
the exact bytes it must and leaves every other byte untouched -- no normalizing,
no repacking, no rewriting regions that didn't change. Corrupting an otherwise-valid
save is the worst outcome the app can produce. See
[../context/principles.md](../context/principles.md) -> "Save File Integrity Is
Sacred".

## Cross-cutting patterns (used everywhere)

- **Singletons via `inst()`** -- every game DB and the common helpers are
  `static T* inst()` singletons with private constructors. Never `new` them. The
  Qt 6 static-init deadlock that shaped this is in
  [../decisions/architecture.md](../decisions/architecture.md).
- **QML ownership protection** -- parentless C++ QObjects handed to QML get
  garbage-collected unless pinned. `Utility::qmlProtectUtil()` (see [common.md](common.md))
  is the shared guard; `Q_INVOKABLE` returns additionally use `qmlCppOwned()`
  (`savefile/src/pse-savefile/qmlownership.h`). Background in
  [../reference/qt6-patterns.md](../reference/qt6-patterns.md).
- **Fixed-width integers** -- all save math uses the `var8`/`var16`/... aliases
  ([common.md](common.md)) so byte widths are never left to platform defaults.
