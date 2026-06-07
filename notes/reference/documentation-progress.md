# Documentation Progress Ledger

Tracks the project-wide doc-comment pass (started 2026-06-06). The goal: every
component documented to the [house style](doc-comment-style.md). This is a
deliberately multi-session effort -- ~374 source files, ~43k lines. This ledger
is the resume point; update it as each module is finished and verified.

## Scope

| Layer | Path | .h | .cpp | qml | Status |
|-------|------|----|----|-----|--------|
| common   | `projects/common/src/pse-common`   | 4  | 2  | -- | **DONE + verified** |
| savefile | `projects/savefile/src/pse-savefile` | 50 | 48 | -- | DONE -- all 50 headers + 48 `.cpp` documented & verified comments-only |
| db       | `projects/db/src/pse-db`           | 57 | 56 | -- | DONE -- all 57 headers + 56 `.cpp` documented & verified comments-only |
| app      | `projects/app/src`                 | 29 | 33 | -- | DONE -- all 29 headers + 33 `.cpp` documented & verified comments-only |
| qml      | `projects/app/ui`                  | -- | -- | 95 | **DONE (2026-06-06)** -- all 95 `.qml` carry a filename header block + inline notes; insertion-only edits, code untouched |

## System map

The architecture write-up lives in [`../systems/`](../systems/README.md). The macro
overview ([overview.md](../systems/overview.md)) and the `common` layer deep-dive are
written; per-layer deep-dives (savefile, db, app, qml) are authored as each layer is
documented.

## Done

### common (2026-06-06)
All 6 files documented and verified comments-only:
`types.h`, `random.h`, `random.cpp`, `utility.h`, `utility.cpp`, `common_autoport.h`.
This layer is the **style reference** -- new layers should match its voice and depth.

### savefile -- core spine (2026-06-06)
Documented + verified comments-only: `savefile.h`, `savefiletoolset.h`,
`savefileiterator.h`, `filemanagement.h`, `savefile_autoport.h`, `qmlownership.h`.
System map: [../systems/savefile.md](../systems/savefile.md).
**Done in `expanded/`:** the root (`savefileexpanded.h`) + the full `player/` subtree
(`player.h`, `playerbasics.h`, `playerpokedex.h`, `playerpokemon.h`) -- verified
comments-only. The shared "expanded node" convention (load/save/reset/randomize) is
documented once on `SaveFileExpanded` and referenced by the rest.
Also done: the **entire `fragments/` group (12)** -- the Pokemon fragments (pokemonbox,
pokemonstoragebox, pokemonparty, pokemonstorageset) plus item, itemstoragebox, signdata,
spritedata, warpdata, mapconndata, hofpokemon, hofrecord. All verified comments-only.
Also done: the **entire `expanded/area/` group (12)** -- area + audio, general, loadedsprites,
map, npc, player, pokemon, sign, sprites, tileset, warps (all verified comments-only).
Also done: the **entire `expanded/world/` group (11)** -- world + completed, events, general,
hidden, local, missables, other, scripts, towns, trades (all verified comments-only).
Also done: the **4 `expanded/` root siblings** (daycare, halloffame, rival, storage).

**ALL 50 savefile headers are documented and verified comments-only** (sweep confirmed every `.h`
carries doc comments).

**The 48 `.cpp` are also done** (2026-06-06): each carries an `@file` block identifying/summarising
the implementation; existing inline comments preserved; verified comments-only via strip-diff. The
per-method API docs live in the headers, so the `.cpp` were not padded with line-by-line narration
of mechanical byte loops.

### >>> savefile layer COMPLETE: 50 headers + 48 .cpp = 98 files, all verified. <<<

### db -- spine + conventions (2026-06-06)
Documented + verified comments-only: `db.h` (the aggregate + bootstrap order),
`db_autoport.h` (export macro + opaque list), and the convention setters
`creditsdb.h` (DB-singleton pattern) + `entries/creditdbentry.h` (DB-entry pattern).
System map: [../systems/db.md](../systems/db.md). The two conventions are documented
once on those files and referenced by the rest.
**Remaining in db:** the other ~27 databases, ~25 entries, `util/` (3), and all `.cpp`.

### >>> db layer COMPLETE (2026-06-06): 57 headers + 56 .cpp = 113 files, all verified. <<<

### >>> app layer COMPLETE (2026-06-06): 29 headers + 33 .cpp = 62 files, all verified. <<<
boot + bridge/router/settings + engine providers + all 17 mvc models (incl. the
itemmarket hierarchy). System map: `../systems/app.md`. Every `.cpp` has an `@file`
block; all verified comments-only. NOTE: `MainWindow` (app/ui/window) is C++ but
outside the Doxyfile INPUT -- flagged in app.md.

**C++ documentation is now COMPLETE: common + savefile + db + app = 4 layers, all verified.
Only the QML UI layer remains** (human comments, intentionally not Doxygen-generated).
All 28 databases, 26 entry structs, 3 util helpers documented (conventions on
creditsdb/creditdbentry; system map in `../systems/db.md`); every `.cpp` carries an
`@file` block; all verified comments-only via strip-diff.

### >>> qml layer COMPLETE (2026-06-06): all 95 .qml documented. <<<
Every QML file now opens with a `// <Filename>.qml -- ...` header block describing
what it is, what it binds to on `brg`, and any gotchas; existing Twilight comments
were preserved (filename prepended, never replaced). Edits were **insertion-only**
(whole comment lines before the code), so `git diff` is pure additions and no code
line changed -- confirmed by a sweep (`head -20` of all 95 files shows a header; 0
missing). Coverage: roots (App, AppWindow), 10 non-modal screens, 4 modal screens,
and all 80 `fragments/` (header, modal, general, controls/{menu,name,name-full,
selection}, screens/{home,bag,pokemon,pokemon-details/{,stats},trainer-card}).
Recurring invariants flagged in-place where someone would edit them: the `(dexInd+1)`
/ `(dexNum+1)` / `(itemDex+1)` 0->1 dex conversions (do NOT simplify), the
commit-on-finish (atomic) name/ID writes, and the shiny `onToggled` guard.
System map: [../systems/qml.md](../systems/qml.md).

## Suggested order for the remaining layers

1. **savefile** -- the heart of the app (byte-exact parsing/expansion). Document the
   spine first: `savefile.h`, `savefileexpanded.h`, `filemanagement.h`, then the
   `expanded/{player,area,world,fragments}` objects.
2. **db** -- the singleton reference databases + entry structs. Document `db.h`, the
   `DB` aggregate, the singleton/inst() pattern once (link it), then each DB.
3. **app** -- `boot/`, `bridge/`, `engine/`, `mvc/`. The bridge + boot sequence are
   the QML<->C++ wiring; document those carefully.
4. **qml** -- same quality, human comments (not generated; see the doc decision).

## Per-module checklist (repeat for each)

- [ ] Read the module's headers + cpp; understand its role and wiring.
- [ ] Capture the understanding in the system-map notes (see `notes/systems/`).
- [ ] Add `@file` + class + member docs (headers) and `//` impl notes (cpp).
- [ ] ASCII-only; comments only.
- [ ] Verify comments-only (gcc strip-diff; trust the Read tool if bash is stale).
- [ ] Tick the row above and note the date.
