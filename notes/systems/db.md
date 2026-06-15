# System Map: `db` layer

The game's static reference data (`projects/db/src/pse-db`). Everything the games
"just know" -- every Pokemon, move, item, map, font glyph, trainer, type matchup --
loaded from JSON assets into singleton databases. It is **read-only** and
independent of any save file; the savefile layer consults it to resolve ids into
names, prices, types, etc. See the macro picture in [overview.md](overview.md).

## Shape

| Part | What |
|------|------|
| `db.h` | **DB** -- the aggregate. Exposes all 26 sub-databases as `Q_PROPERTY` (QML reaches them as `db.pokemon`, `db.moves`, ...) and runs the bootstrap. |
| `*db.h` / `*.h` (28) | The individual singleton databases (CreditsDB, PokemonDB, MovesDB, ItemsDB, MapsDB, FontsDB, TrainersDB, ...). |
| `entries/*.h` (~26) | The entry structs each database stores (CreditDBEntry, the `MapDBEntry` family, ItemDBEntry, PokemonDBEntry, ...). |
| `util/` (3) | Helpers: `GameData` (the parsed JSON source), `FontSearch`, `MapSearch`. |
| `db_autoport.h` | The library import/export macro + the central opaque-pointer list for DB entry types. |

## The bootstrap (critical order)

`DB::inst()` is the single entry point; one call brings the whole layer up, in
order:

```
DB::inst()
  -> (construct all 26 sub-DB singletons)
  -> initRes()       // embedded JSON resources
  -> qmlRegister()   // register every DB + entry type with QML
  -> loadAll()       // each DB loads its entries from JSON
  -> indexAll()      // build per-DB key lookups
  -> deepLinkAll()   // resolve cross-DB references
```

`bootDatabase()` (app layer) is literally just `(void)DB::inst();`. **No DB may be
accessed before DB::inst() runs.**

> **Why constructors must not load:** an early design had each DB call load() in
> its constructor; in Qt 6 that re-enters the singleton mutex and deadlocks.
> Loading was moved out to DB::loadAll(). See
> [../decisions/architecture.md](../decisions/architecture.md).

## The two conventions (read one, you've read them all)

**DB singleton** (canonical example: `CreditsDB`): `static T* inst()` + private
ctor; a `QVector<XxxDBEntry*> store`; `load()` (called by DB::loadAll); QML access
via `getStore()` / `getStoreSize` / invokable `getStoreAt(ind)`; qmlProtect/
qmlRegister; the entry struct is a `friend`. Richer DBs add a key->entry index.

**DB entry** (canonical example: `CreditDBEntry`): a QObject struct with
**protected** write-once fields exposed through public getters (`Q_PROPERTY READ`)
-- always read via the getter, never the field. Protected constructors take the
JSON; a static `process()` parses JSON into the DB's store; the DB is a `friend`.
Entries that reference other DBs add a **`deepLink()`** run in the deep-link pass
(e.g. a move resolving its type, an evolution its target species).

Both conventions are documented in code on `CreditsDB` / `CreditDBEntry` and
referenced by the rest.

## Opaque pointers (`db_autoport.h`)

Unlike the savefile tree (whose objects QML traverses, so they must be complete
types), DB entry pointers are declared **opaque** once in `db_autoport.h` and
shared library-wide. QML reaches entries through invokable accessors
(`getStoreAt`, `...At`) rather than a deep `Q_PROPERTY` chain, so opaque is fine
and keeps MOC/compile costs down. (Contrast with
[savefile.md](savefile.md) and [../reference/qt-patterns.md](../reference/qt-patterns.md).)

> Documentation status: spine done -- `db.h`, `db_autoport.h`, and the convention
> setters `creditsdb.h` / `creditdbentry.h`. The remaining databases, entries, and
> `util/` are in progress; tracked in
> [../reference/documentation.md](../reference/documentation.md).
