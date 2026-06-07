# Architecture

> For the in-depth, code-grounded version of everything below, see the system map:
> [`../systems/overview.md`](../systems/overview.md).

## Sub-libraries (build order)

```
common  →  db  →  savefile  →  app
```

| Library | Header prefix | What it does |
|---------|--------------|--------------|
| `common` | `pse-common/` | Shared types (`var8`, `var16`), `Random` (wraps `QRandomGenerator`), `Utility` |
| `db` | `pse-db/` | Game data — all 151 Pokemon, maps, items, moves, etc. Loaded from JSON assets |
| `savefile` | `pse-savefile/` | Save file parsing, expansion into C++ objects, and flattening back |
| `app` | (executable) | Qt/QML UI. Bridge pattern: C++ singletons exposed to QML via `Bridge` object |

## DB Layer — Singleton Pattern

Every DB class uses a `static T* inst()` singleton:

```cpp
PokemonDB::inst()->getIndAt("pikachu")     // lookup by string key
MovesDB::inst()->getStore()                // full vector
MapsDB::inst()->getStoreSize()             // count
MapsDB::inst()->search()->isGood()...      // fluent query API
FontsDB::inst()->convertToCode(str, len)   // non-static utility method
```

**Critical**: `DB::inst()` bootstraps everything. It calls `loadAll()`, `indexAll()`, `deepLinkAll()`
in order. No DB singleton should be accessed before `DB::inst()` runs.

**Do not call `load()` from DB constructors** — this caused a static-init deadlock in Qt 6.
See `decisions/architecture.md`.

## DB Entry Types

Entry structs (e.g. `MapDBEntry`, `MoveDBEntry`, `ItemDBEntry`) have **protected** data.
Always use public getters:

```cpp
entry->getName()     // not entry->name
entry->getInd()      // not entry->ind
entry->getWidth()    // not entry->width (was also std::optional, now plain int)
```

Check `if (entry->getWidth() >= 0)` — not `if (entry->getWidth())` — since -1 means unset.

## Central Opaque Pointer Files

Qt 6 requires `Q_PROPERTY` pointer types to be either fully defined or declared with
`Q_DECLARE_OPAQUE_POINTER`. We centralize these:

- `db/src/pse-db/db_autoport.h` — all DB type opaque pointer declarations
- `savefile/src/pse-savefile/savefile_autoport.h` — all savefile type declarations
  - Must have `#include <QMetaType>` before the macro calls

## db.h Includes All Sub-DB Headers

Qt 6 MOC requires complete types for `Q_PROPERTY`. `db.h` includes every sub-DB header
directly so MOC can see full class definitions.

## QML Bridge Pattern

The app uses a `Bridge` object as the single point of contact between QML and C++.
It's exposed to QML as `brg`. The access chain looks like:

```
brg                              → Bridge (C++)
  .file                          → FileManagement
    .data                        → SaveFile
      .dataExpanded              → SaveFileExpanded
        .player.basics.money     → PlayerBasics::money (Q_PROPERTY)
```

For QML to traverse this chain, every QObject type in it must be a **complete type** at the MOC
TU that declares the property — i.e. fully `#include`d, NOT forward-declared and NOT
`Q_DECLARE_OPAQUE_POINTER`'d (the opaque macro forces Qt to treat the pointer as a non-QObject,
returning `undefined` in QML). `qRegisterMetaType` / `qmlRegisterUncreatableType` do not change
this. Include only the branches QML actually traverses, to keep build times sane. See
`decisions/architecture.md` → "QML Property-Chain Traversal" and `reference/qt6-patterns.md`.

## Project Layout (files)

```
projects/
  CMakeLists.txt              ← root CMake
  common/src/pse-common/      ← types.h, utility.h, random.h
  db/src/pse-db/              ← *.h/*.cpp for each DB class
    entries/                  ← entry structs (MapDBEntry, etc.)
    util/                     ← FontSearch, MapSearch, GameData
  savefile/src/pse-savefile/
    expanded/                 ← C++ "expanded" save objects
      area/                   ← AreaAudio, AreaMap, etc.
      fragments/              ← PokemonBox, Item, SignData, etc.
      player/                 ← PlayerBasics, PlayerPokemon, etc.
      world/                  ← WorldEvents, WorldMissables, etc.
  app/src/
    boot/                     ← boot.cpp, bootDatabase.cpp, bootQmlLinkage.cpp
    bridge/                   ← bridge.h, router.h, settings.h
    engine/                   ← font preview, tileset providers
    mvc/                      ← Qt models for ListView/etc.
  app/ui/
    app/                      ← QML source tree
      screens/                ← full screen QML files
      fragments/              ← reusable QML components
```

## Build System

- CMake 3.21+, Qt 6.11, llvm-mingw_64
- Build directory: `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/`
- Old `.pro` files exist but are unused
- **Do not use** `qt_add_qml_module()` — it conflicts with `app.qrc` (causes QML hang)
- QML files are listed in `app.qrc` and registered via `qmlRegisterType()` in `bootQmlLinkage.cpp`
