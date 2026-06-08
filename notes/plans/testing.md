# Testing Strategy — Comprehensive Plan

_Created 2026-06-07. Status: **Phases 1–2 built, run & ALL GREEN on the real Qt 6.11 kit (3/3 suites,
31 field assertions) — 2 real bugs found and fixed** (Daycare empty-destructor crash; bank-2 checksum
off-by-one). Tests live under `projects/tests/`; later phases here are still the design to build
against. QML/UI testing is **deferred pending Twilight's decision** — see "QML / UI testing" below._

> **Phase 2 added (2026-06-07): `tests/savefile/tst_fields.cpp` — per-field expand/flatten coverage,
> all green.** Two styles per field: VALUE round-trip (set on the model → flatten → re-expand → assert
> the value survived, incl. boundary values) and BYTE isolation (edit one field → only that field's
> bytes + main checksum `0x3523` change). Covered: money (0/1/typical/999999), coins (0/1/9999),
> player ID (0/1/0x1234/0xFFFF), starter (0/1/153/255), player name (empty/1/typical/7-char), badges
> (pattern/all-on/all-off, both the primary `0x2602` and duplicate `0x29D6`), pokédex (independent
> owned/seen patterns across all 151), rival (name+starter), and a bag item (index+amount). Offsets
> taken from `playerbasics.cpp` (second oracle) cross-checked with the `.bt`. Name/ID round-trips set
> the member directly to avoid the intentional OT-rewrite side effect of `fullSet*`.

> **Implemented so far (2026-06-07, phase 1 — needs a build + run on Twilight's Qt kit):**
> - `projects/tests/` wired into CMake via `add_subdirectory(tests)` + `enable_testing()` (root
>   `CMakeLists.txt`, guarded by `option(PSE_BUILD_TESTS ON)`; `Test` added to the Qt6 components).
>   Each test is a `qt_add_executable` registered with `add_test` (run via `ctest`). Fixtures are
>   located through a `PSE_ASSETS_DIR` compile def → `<repo>/assets` (read-only; tests copy in memory).
> - `tests/helpers/savefilefixture.h` — read a fixture, load into `SaveFile`, snapshot raw bytes, diff.
> - `tests/savefile/tst_roundtrip.cpp` — (a) **load→flatten→recalc identity** on both fixtures
>   (open-then-save changes zero bytes), (b) **money single-edit isolation** (only the money bytes +
>   main checksum change; value round-trips). Offsets (`money 0x25F3`, `main checksum 0x3523`) taken
>   from the `.bt` oracle; tests print the actual changed offsets on failure so a wrong constant is
>   self-correcting on first run.
> - `tests/db/tst_db_integrity.cpp` — DB boots, ≥151 species, every non-glitch species deep-links its
>   primary type.
>
> **Key pipeline fact baked into the tests:** `SaveFileExpanded::save()` (i.e. `flattenData()`) does
> **not** recompute checksums; `recalcChecksums()` runs in `FileManagement::writeSaveData()` at file-write
> time. So a faithful "open→save" test must call `flattenData()` **then** `toolset->recalcChecksums()`.
>
> **Built & run on the real Qt 6.11 / llvm-mingw kit (2026-06-07, via PowerShell on Twilight's PC):**
> configure + build clean; `ctest` results:
> - `tst_db_integrity` — **PASS** (DB boots, ≥151 species, every non-glitch species deep-links its type).
> - `loadFlatten_isIdentity(new game)` — **PASS** (a fresh save round-trips byte-perfectly, 0 changes).
> - `moneyEdit_touchesOnlyMoneyAndChecksum` — **PASS** (confirms `money 0x25F3` / `checksum 0x3523`;
>   only those bytes change; value round-trips).
> - `loadFlatten_isIdentity(progressed)` — **PASS after fix.** Originally failed: `flatten` changed
>   **0 bytes** (engine byte-perfect), but `recalcChecksums()` wrote `0x5A4B 00→C5` — the **bank-2
>   box-checksum off-by-one**. After correcting it to `0x5A4C`/`0x1A4C`, `BaseSAV.sav` round-trips
>   byte-perfect. **All tests now green (100%).**
>
> **Two real bugs found on day one and fixed** (the whole point): (1) `Daycare::~Daycare()` null-deref
> crash on an empty Day Care (`daycare.cpp`); (2) bank-2 box-checksum off-by-one in `recalcChecksums()`
> (`savefiletoolset.cpp`) — corrected to `0x5A4C`/`0x1A4C`, verified by `tst_roundtrip`. Both in
> `reference/fix-patterns.md`; checksum detail in `status.md`.
>
> Reproduce: configure with `C:/Qt/Tools/CMake_64/bin/cmake.exe -S projects -B <build>`, build targets
> `tst_roundtrip tst_db_integrity`, run `ctest --output-on-failure` (PATH needs the Qt + lib-output dirs
> for DLLs). Tests use `QTEST_GUILESS_MAIN` and ran fine headless (no GUI dependency in DB load).

This is the master plan for full, professional test coverage of the project. The goal is not
"some tests" — it is **comprehensive** coverage: every function exercised across normal, boundary,
and invalid inputs, with both return values and side effects asserted; every error path verified
to degrade gracefully; saving/loading proven correct in every context; randomization proven to
hold its invariants; and every historical bug locked out by a permanent regression test. Coverage
tooling makes "comprehensive" a **measured fact**, not a hope.

---

## Why testing matters specifically here

Two project values make automated testing unusually high-value:

1. **Save File Integrity Is Sacred** (`../context/principles.md`). The byte-fidelity contract —
   flatten changes *only* the bytes an edit requires, every other byte untouched — was verified by
   hand over dozens of hours. Tests turn that manual verification into something that runs in a
   second on every build, forever. Corrupting a save is the worst outcome the app can produce;
   round-trip tests are the guardrail.
2. **Graceful Degradation** (the Sims-2 philosophy). "Never crash, never block, never strand the
   user" is only true if it's *tested*. Negative/error-path tests assert the degradation actually
   happens on every failure mode.

The whole class of bug that consumed sessions 13f–13h — QML garbage-collecting parentless C++
QObjects → use-after-free — is exactly what a **sanitizer build** catches automatically. That
alone may justify the harness.

---

## What makes this project testable

The layered architecture (`common → db → savefile → app`, see `../systems/overview.md`) is a gift
for testing: `common`, `db`, and `savefile` are libraries with **zero UI dependency**. A headless
test executable links them directly and exercises the entire byte engine, the game databases, and
all save logic without ever creating a window. ~90% of the project's risk lives in those three
layers, and 100% of it is reachable headless.

Only `app` (the Bridge, models, QML) needs the Qt GUI/Quick stack to test — and even that can run
on the `offscreen` platform plugin with no display.

---

## Test taxonomy — every type, mapped to the layers

| Type | What it proves | Primary layers | Tooling |
|------|----------------|----------------|---------|
| **Unit** | Each function correct across normal/boundary/invalid inputs; return values AND side effects | common, db, savefile, app(C++) | QtTest |
| **Boundary / edge case** | 0, max, empty, full, first/last index, -1 "unset" sentinels | all | QtTest (data-driven) |
| **Negative / error path** | Graceful degradation: bad input → defined error, no crash, save untouched | savefile, app, db(asset load) | QtTest |
| **Round-trip / golden** | load→flatten == identity; single edit changes ONLY intended bytes | savefile | QtTest + committed fixtures |
| **Integration** | Layers cooperate (db↔savefile expansion, FileManagement↔SaveFile cycle) | db+savefile, app | QtTest |
| **End-to-end / system** | Full journey: open → edit sequence → save → reopen → assert persisted + valid | savefile (+app) | QtTest |
| **Randomization** | Every playability invariant holds across many seeds | savefile, db | QtTest + seeded Random |
| **Property-based / fuzz** | Invariants survive random edits + random/garbage blobs | savefile | QtTest (generative helpers) |
| **Regression** | Each historical bug stays fixed forever | wherever the bug lived | QtTest (named per bug) |
| **Compatibility** | Red vs Blue; fresh / mid-game / post-E4 saves | savefile | QtTest + fixture matrix |
| **Performance / benchmark** | load/expand/flatten/randomize stay fast; no hangs | savefile | QtTest `QBENCHMARK` |
| **Memory / sanitizer** | No use-after-free / leak / UB (the QML-GC bug class) | all (whole suite) | ASan + UBSan build, Valgrind |
| **Coverage** | Proves comprehensiveness; finds untested lines/branches | all | llvm-cov (llvm-mingw) |
| **GUI / QML** | UI flows behave (commit-on-blur, tab reactivity, popup dismiss) | app/ui | Qt Quick Test (offscreen) — **deferred** |

### Concrete examples per layer

**common** — `var8`/`var16` width and overflow behavior; `Random` distribution + determinism under
a fixed seed; every `Utility` helper; the QML-ownership guards return what they're handed.

**db** — asset integrity: 151 Pokémon load, every deep-link resolves (no dangling refs), no required
field empty/null; every getter returns expected values vs the JSON; `-1` "unset" handled
(`getWidth() >= 0`, not truthiness); search APIs (`MapSearch`, `FontSearch`) across hits, misses,
multi-match, empty; `FontsDB::convertToCode` / `splice` / `expandStr` on normal, empty, and the
variable-tile / lone-variable-tile cases (the s13y infinite-loop bug).

**savefile** — the heart of the suite:
- *Identity*: load fixture → `expandData()` → `flattenData()` → `memcmp` == 0.
- *Single-edit isolation*: change one field → flatten → diff blobs → assert changed-offset set ==
  exactly {field bytes} ∪ {checksum}. Repeat per field (money, name, party species/level/DVs/EVs/
  moves/PP, items, badges, pokédex seen/caught, playtime, OT/ID).
- *Per-field expand/flatten*: known raw bytes ↔ known expanded value, both directions, table-driven.
- *Encodings*: BCD money, the custom text charset (incl. the simulated-tileset paths), party layout.
- *Checksum*: after an edit the Gen-1 checksum is recomputed correctly (the one region that *should*
  change). See `../reference/gen1-knowledge.md`.
- *Negative*: `setData(null)`; files < 32 KB rejected; larger files load first 32 KB; locked/missing
  files; truncated/garbage blobs → defined error, never a crash, never a mutated source.
- *Reset / erase / randomize* verbs each tested.

**app (C++)** — Bridge exposes the expected object graph; list models (`pokedexModel`,
`mapSelectModel`, …) map C++ → item-model rows correctly (rowCount, roles, data at index, empty
state); `FileManagement` open/save/recent-prune logic; `Router` screen set.

---

## Randomization testing (flagship feature)

Run the randomizer across a large seed sweep and assert **every** promise from
`../context/principles.md` → "The Randomization Feature" holds on **every** run:

- Save is structurally valid and would load (checksum ok, sizes intact).
- At least one **HM-capable** Pokémon in the team (escape slave).
- **No glitch** Pokémon, **no glitch** items.
- Levels within a balanced range (no instant-kill lv100 walls).
- Maps have **valid warps** — player can leave the start.
- No beaten trainers / completed events — clean start.
- Money/items/team read as "starting conditions," not cheats.

Because `Random` wraps `QRandomGenerator`, tests seed it deterministically so any failure is
reproducible from the logged seed. This doubles as fuzz coverage of the flatten path.

---

## Coverage — how we prove "comprehensive"

llvm-mingw ships `llvm-cov` + `llvm-profdata`. The test build compiles with
`-fprofile-instr-generate -fcoverage-mapping`; after `ctest`, generate a line+branch report.

Targets (initial, tunable):

| Layer | Line target | Branch target |
|-------|-------------|---------------|
| common | 100% | ≥ 95% |
| db | ≥ 95% | ≥ 90% |
| savefile | 100% | ≥ 95% |
| app (C++) | ≥ 80% | ≥ 70% |

"Comprehensive" = the report shows no untested function and no untested error branch in
common/savefile. Coverage gaps become the to-do list for filling tests.

---

## Infrastructure & tooling

- **QtTest** (`Qt6::Test`) — bundled, no third-party dep. One `QObject` per suite; each `private
  slot` is a case; `_data()` slots + `QFETCH` give table-driven cases; `QCOMPARE`/`QVERIFY`
  assertions; `QBENCHMARK` for perf.
- **Qt Quick Test** (`qmltest`) — for QML, runs on `QT_QPA_PLATFORM=offscreen`. **Deferred.**
- **CTest** — `enable_testing()` in root CMake; each test exe registered via `add_test`; whole suite
  runs with one `ctest` command. Honors the existing CMake/Qt 6.11/llvm-mingw setup.
- **Fixtures (these exist — see `assets/`):**
  - `assets/BaseSAV.sav` — **the default/primary fixture.** A regular save progressed to a point;
    this is what most tests load (round-trip, per-field, E2E) because it has populated
    party/items/dex/flags — realistic, representative data. Exact progress not recorded; characterize
    it once and pin the values in a test so the fixture is self-documenting.
  - `assets/BaseSAV.new.sav` — **secondary, used sparingly.** A save at the very start of the game
    (hence "new"). Useful for the specific cases that want a clean/minimal baseline (e.g. proving an
    edit on near-empty data, or empty-party/empty-box edge cases) — not the everyday fixture.
  - Both are exactly `0x8000` bytes. The negative-test corpus (truncated / oversized / locked /
    garbage) still needs to be **generated** (trivially, by slicing/padding a copy).
  - Plan: copy these into `tests/fixtures/` (don't have tests mutate the originals in `assets/`), or
    reference them read-only and always operate on an in-memory/temp copy.
- **The byte-map oracle — `assets/savefile-structure.bt`.** A 010 Editor binary template authored by
  Twilight, independent of the app's C++, mapping every field/offset/bit-field of the save. This is
  the **independent oracle** that breaks test circularity: per-field offset tests and golden
  assertions validate expand/flatten against *the .bt's* offsets, not against the same code being
  tested. A small parser (or a hand-transcribed offset table derived from it) feeds the data-driven
  field tests. Banks are `0x2000` each (bank0 `0x0000`, bank1/main `0x2000`, bank2/boxes1-6 `0x4000`,
  bank3/boxes7-12 `0x6000`); the .bt confirms e.g. player name at bank1+`0x598`, the main-data
  checksum and the per-box checksum layout — cross-check these against
  `../reference/gen1-knowledge.md`.
- **Sanitizers** — a dedicated CMake config building the suite with ASan + UBSan; (Valgrind optional
  on Linux). The QML-GC use-after-free class is caught here automatically.
- **Coverage build** — separate config, llvm-cov reporting.
- **CI (later)** — GitHub Actions running `ctest` + sanitizer + coverage on every push. Directly
  addresses the "is the working state actually safe to rely on?" anxiety from `status.md`: green
  suite == provably good state. (Aligns with the standing "COMMIT/BACK UP" lesson.)

### ⚠️ Project rules the harness must respect

- **Do NOT use `qt_add_qml_module()`** (conflicts with `app.qrc`, hangs). QML tests load qrc-listed
  types the existing way.
- **Singletons via `inst()`** — tests must never `new` a DB; use `DB::inst()`. Note `DB::inst()`
  bootstraps load/index/deeplink, so db/savefile tests that need game data call it once in
  `initTestCase()`.
- **No `load()` in constructors** assumption holds — tests rely on explicit boot order.
- **Never write a save byte not under test.** Round-trip tests operate on *copies* of fixtures and
  assert the on-disk fixture is never mutated.
- **Windows JIT-debugger popup on crashes (operational).** `AeDebug\Auto=1` registers Qt Creator's
  `qtcdebugger.exe`, so a crashing test exe (access violation) pops a "open a debugger?" dialog and
  **hangs** until dismissed — running a batch of crash-prone exes (e.g. an ASan build, or a
  newly-found crash) spams these and blocks. When running tests from a script, first set the process
  error mode so crashes fail fast instead: in PowerShell `Add-Type` a `SetErrorMode` P/Invoke and call
  `SetErrorMode(0x0003)` (`SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX`) before launching exes
  (session-scoped, inherited by children, no registry change). Run unknown/crash-prone exes one at a
  time and kill on a non-zero crash code.

---

## Repository layout (proposed)

```
projects/
  tests/
    CMakeLists.txt              ← enable_testing + add_subdirectory per suite
    fixtures/                   ← committed .sav files (good + corrupt) + golden blobs
    common/    tst_random.cpp, tst_utility.cpp, tst_types.cpp
    db/        tst_pokemondb.cpp, tst_fontsdb.cpp, tst_search.cpp, tst_db_integrity.cpp
    savefile/  tst_roundtrip.cpp, tst_expand_flatten.cpp, tst_fields.cpp,
               tst_negative.cpp, tst_randomizer.cpp, tst_e2e.cpp, tst_regressions.cpp,
               bench_savefile.cpp
    app/       tst_bridge.cpp, tst_models.cpp, tst_filemanagement.cpp
    qml/       (deferred) tst_*.qml + a quicktest runner
```

Mirrors the source tree so "is function X tested?" maps to an obvious file.

---

## Conventions

- One suite per source unit; test fn named for the behavior (`money_roundtrips`,
  `loadingTruncatedFile_raisesError_leavesSourceUntouched`).
- Prefer **data-driven** (`_data()`) over copy-pasted cases — byte-offset tables are the natural fit.
- Tests are **deterministic**: seed `Random`; never depend on wall-clock/filesystem state beyond
  fixtures; copy fixtures to a temp dir before any write.
- Every fixed bug gets a regression test named `regression_<shortdesc>` with a comment linking the
  commit/session.
- New test files are added to `tests/CMakeLists.txt` (the qrc rule is QML-only; C++ tests are plain
  CMake targets).

---

## Regression suite — seed list (from known history)

Each becomes a permanent named test:

- Hidden-items overwrite from a save-location typo (`e20c167`).
- Party data mangled on write (`cb6fc99`).
- Missables saved back wrong (`ff76662`).
- `FontsDB::splice` lost `out.remove()` → infinite loop on a variable tile (s13y).
- Tileset last tile not clickable — off-by-one, `fontAt` is 1-based (s13y2).
- File-load crash: `setData` memcpy from null on missing/unreadable recent (s14).
- QML-GC use-after-free on parentless QObjects (s13f–h) — covered structurally by the sanitizer build.
- Player/rival/ID edit hang + OT corruption (s13w).

---

## Phased rollout

Each phase is independently valuable; the suite is useful from phase 1.

1. **Harness + first guardrail.** `tests/` tree, CTest wiring, one fixture; the round-trip *identity*
   + *single-edit isolation* tests building and green. Proves the whole loop.
2. **savefile to target coverage.** Per-field expand/flatten, encodings, checksum, all verbs.
   _Started 2026-06-07. `tst_fields.cpp`: trainer basics (money/coins/ID/starter/name), badges,
   pokédex, rival, one bag item — value round-trips + byte isolation, all green. `tst_pokemon.cpp`:
   party AND PC-box Pokémon records (BOX_MON/PARTY_MON) — every stored field round-trips: species,
   level, status, hp, catch rate, OT id/name, exp, the 5 stat-exp ("EVs"), the 4 DVs, all 4 move
   slots (id/pp/ppUp), nickname, and the party-only battle stats. (Surfaced that the engine clamps PP
   to each move's max, faithfully to the game.) Still to do: full item LISTS (add/remove/move/sort,
   terminator), world/area/world-event fields, playtime, and the verbs reset()/eraseExpansion()/
   randomize()._
   _Verbs (`tst_verbs.cpp`): `resetData()` (zeros the buffer + blanks the model) and `eraseExpansion()`
   (blanks the model, touches **zero** save bytes) — both green. `randomizeExpansion()` is `QSKIP`'d:
   it crashes on a progressed save (found+fixed a `MapSearch::isType()` null-deref; still crashes in
   `SpriteData::load()` via `AreaSprites::randomize()`) — deferred to phase 7 (randomizer is WIP)._

   **Flaky-test note (2026-06-08):** `tst_market_model` intermittently segfaulted (~1/3 runs, exit
   0xC0000005) via a queued slot invocation (`QMetaMethod::invokeImpl`) on a torn-down object. Cause:
   the test created+destroyed a `Bridge` PER test method; the Bridge has many cross-object signal
   connections + the static `ItemMarketEntry` pointers, and the churn left a queued call targeting a
   freed object. The real app never churns Bridges (one lives for the app's lifetime), so this was
   test-induced, not a shippable bug. Fix: market test uses ONE Bridge (initTestCase/cleanupTestCase);
   its assertions are all relative (money up/down vs each test's own baseline) so a shared fixture is
   order-independent. The other bridge-based model tests (storage/item-storage/bridge) don't churn
   into a crash (verified 0/20 each), but prefer a single shared Bridge for new app-model tests.

   **Bugs found by phase-2 tests so far (all real): Daycare empty-destructor crash (fixed), bank-2
   checksum off-by-one (fixed), `MapSearch::isType()` null-deref (fixed), randomizer sprite-path crash
   (open, phase 7), `PlayerPokedex::reset()` memset value/count swap (fixed 2026-06-08) — it passed
   `pokemonDexCount` as the fill VALUE, so reset() filled every seen/owned byte with 151 (= true),
   marking the whole dex seen+owned instead of blanking it; randomize() masked it by assigning every
   entry. Caught by `tst_pokedex` markAll_and_reset.**
   **DB store-accessor bounds (found 2026-06-08 by the `tst_db_stores` sweep, all fixed):
   `CreditsDB::getStoreAt()` had a fully **inverted** guard (`store.size() >= ind`) — it returned
   nullptr for every valid index (so the credits list could never resolve a row) and ran
   `store.at(ind)` for out-of-range indices (crash). Eight more DBs (Events, Fly, Fonts, GameCorner,
   Items, Maps, Missables, EventPokemon) guarded positive overflow but **not negative** indices, so
   `getStoreAt(-1)` (which QML passes for "nothing selected") did `store.at(-1)` → crash. All now use
   the canonical `if(ind < 0 || ind >= store.size()) return nullptr;` the other ten DBs already had.
   `AbstractRandomString::getStoreAt()` (the base for the random name/example sources) had the same
   missing negative guard — also fixed (found 2026-06-08 by `tst_db_random_strings`).**
3. **common to 100%; db integrity + getters + search + fonts.**
   _Done 2026-06-07: `tst_common.cpp` (type widths/sign, Random bounds + degenerate ranges + coin
   variation, Utility URL encode/decode round-trip) and `tst_toolset.cpp` (the byte primitives:
   BCD<->int, 16-bit word + byte order, byte, per-bit set/clear + isolation, Gen 1 checksum vs a
   hand-computed value). Both green (13 + 17)._
   _Db deep integrity (`tst_db_integrity.cpp`, extended): moves load + every non-glitch move deep-links
   its type; types load with names; items load; and the `MapsDB` search chain runs (`isGood()`, and
   `isType()` as a live regression guard for the null-deref fix). 9 cases green. **Finding:**
   `isType("Cave")` matches 0 maps (the cave tileset type string isn't literally "Cave") — the
   randomizer relies on `isType("Cave")->pickRandom()`, so this is tracked for phase 7._
4. **Negative / error-path** across savefile + db asset load (graceful degradation proven).
   _Done 2026-06-07 (`tst_errors.cpp`): missing-file and truncated (<32KB) loads via `FileManagement`
   fail cleanly (return false, surface `lastErrorMessage`, and leave the live save **byte-for-byte
   untouched**); an oversized (>32KB) file loads its first 32 KB; `SaveFile::setData(nullptr)` is a safe
   no-op (the s14 crash fix). 7 cases green. (QSettings isolated under a test org/app name.)_
5. **Integration + E2E** (db↔savefile, FileManagement cycle, open→edit→save→reopen).
   _Done 2026-06-07 (`tst_e2e.cpp`): open fixture → edit money/coins/ID/name/badges/playtime → `saveFile()`
   to a new temp file → reopen in a fresh FileManagement → all edits persisted; the written file is
   exactly 32 KB with a valid main checksum. A second case round-trips a party mon (species/level/DV/
   move/nickname) through actual disk I/O. 4 cases green._
6. **Randomizer invariants + fuzz**; compatibility fixture matrix (Red/Blue, game states).
   _Partly done 2026-06-07 (`tst_randomizer.cpp`): the working randomizer COMPONENTS are invariant-tested
   — trainer basics (money 100-6000, coins 0-100, 4 HM badges on / other 4 off, valid 1-of-3 starter,
   non-empty name; 50 iterations), pokedex (counts stay 0..151), and `newPokemon(Random_Starters3)`
   (valid starter species, sane level). **Full `randomizeExpansion()` now runs end-to-end and is tested**
   (`tst_randomizer` 10-iteration invariants + `tst_verbs` byte-fidelity, both green) after the
   map/area randomizers were disabled (Twilight-authorised; maps WIP) and the `HoFPokemon::load()`
   null-deref was fixed. Re-enabling map randomize is future map-feature work (see status.md). Random-edit
   fuzz + Red/Blue compatibility fixtures still to do._
7. **Sanitizer build + coverage reporting**, then coverage-gap fill to targets.
   _Coverage measured 2026-06-07 via a separate instrumented build (`projects/build/coverage`,
   `-fprofile-instr-generate -fcoverage-mapping`; merge with `llvm-profdata`, summarise with
   `llvm-cov export -summary-only`; per-module totals parsed from the JSON). **Re-measured 2026-06-08
   over the full 34-suite run (`build-cov`), line / function / region:**

   | module       | line  | func  | region |
   |--------------|-------|-------|--------|
   | common       | 79.1% | 77.3% | 82.1%  |
   | db           | 62.0% | 58.2% | 55.3%  |
   | savefile     | 72.9% | 73.6% | 65.2%  |
   | app/appcore  | ~50-58% | ~50-65% | -- |  ← noisy; see below

   _**Coverage-gap-fill pass started 2026-06-08 (session: new chat, build/test/coverage loop
   re-verified end-to-end on the real Qt 6.11 llvm-mingw kit — clean rebuild + 41/41 ctest green
   independently reproduced).** Authoritative savefile real-source line gap list (worst first, by
   missed lines): `pokemonbox.cpp` 72% (314), `spritedata.cpp` 46% (234), `filemanagement.cpp` 64%
   (87), `areamap.cpp` 62% (83), `areapokemon.cpp` 61% (75), `playerbasics.cpp` 67% (57),
   `areatileset.cpp` 62% (51), `signdata.cpp` 40% (50), `savefileiterator.cpp` 57% (47), `item.cpp`
   62% (47), `areawarps.cpp` 70% (46)… First file done: **`tst_sign_data.cpp`** drives the
   DB-population + randomize paths (setTo/setToAll/randomize/randomizeAll/load-null) that the
   area-fragment list-ops test never reached — **signdata.cpp 39.8% → 100.0% (83/83)**, 7 cases green;
   savefile real-source overall 72.9% → **73.8%**. Method note: `MapDBEntrySign` has a protected ctor,
   so DB-defined signs are sourced from the first `MapsDB` map with ≥2 signs; `randomizeAll` is asserted
   as a position permutation (RNG-independent), which also covers the non-null branch of `randomize()`._

   _Second file(s): **`tst_map_fragments.cpp`** — the map-edge fragments' DB-population/resolve paths:
   `WarpData::load(MapDBEntryWarpOut*)` + ctor + `randomize()` + `toMap()`, and
   `MapConnData::loadFromData(MapDBEntryConnect*)` + `toMap()` + load-null. **warpdata.cpp 54.5% →
   98.5%, mapconndata.cpp 71% → 100%**; savefile real-source overall → **74.6% (4408/5905)**, 6 cases
   green (43/43 full suite).

   _Third file: **`tst_iterator.cpp`** — exhaustive coverage of `SaveFileIterator`, the auto-advancing
   byte cursor (every fragment uses it). Each accessor checked for BOTH value round-trip (through the
   toolset) AND cursor side-effect: byte/word/BCD/hex/str/range/bitfield advance by size+padding; getBit/
   setBit do **not** advance; navigation (offsetTo/By, inc/dec, skipPadding) and the push/pop offset stack
   (incl. empty-pop → 0x0000) asserted directly against the public `offset` field. **savefileiterator.cpp
   57.3% → 100.0% (110/110)**; savefile real-source overall → **75.6% (4467/5905)**, 9 cases green
   (44/44 full suite). Notes for future iterator-style tests: `offset` is public (assert it directly) but
   `state` is protected (verify push/pop via `offset` only); and the hex get/set byte convention is the
   toolset's contract (asserted in tst_toolset) — at the iterator layer assert idempotency, not a
   canonical hex string._

   _Fifth file: **`tst_item.cpp`** — the Item inventory-slot fragment: all 4 ctors (null-iterator/
   index+amount/name+amount/random), the load() overloads (incl. invalid-name guard), randomize()
   (50-iter legality sweep: non-glitch, non-once, amount 1-5), toItem() resolution, the full buy/sell
   pricing surface (one + all × money + coins) validated against the resolved ItemsDB entry, the
   null-item → 0 / canSell=false guards, and setAmount() clamping (1..99). **item.cpp 61.8% → 96.7%
   (119/123)**; savefile real-source overall → **76.4% (4510/5905)**, 8 cases green (45/45 full suite)._

   _Sixth file: **`tst_player_basics.cpp`** — the PlayerBasics methods past the field round-trips:
   badge accessors (count/at/set), toStarter() resolution, the individual randomizers
   (coins/money/id/starter, 40-iter range checks), and the **OT-rewrite machinery** —
   `getNonTradeMons()` (incl. the null-file guard), `fixNonTradeMons()`, and `fullSetPlayerName`/
   `fullSetPlayerId`: verified that the value-unchanged path is a true no-op (no owned-mon OT touched —
   the fidelity + anti-hang guard) AND that an actual change rewrites an owned mon's OT name/id (used
   `getNonTradeMons()` itself to grab an owned mon). **playerbasics.cpp 67.4% → 98.9% (173/175)**;
   savefile real-source overall → **77.4% (4568/5905)**, 9 cases green (46/46 full suite)._

   _Seventh file: **`tst_filemanagement.cpp`** — FileManagement's non-GUI logic: the recent-files list
   (add/de-dupe/trim/cap, swap, remove, clear, the persisted ';'-blob `expandRecentFiles` + startup
   `pruneRecentFiles` of unopenable entries, driven via the constructor with a seeded isolated QSettings)
   and the load-error reporting path (cannot-open vs too-short plain-English messages) reached through
   `openFileRecent` without a file dialog. **filemanagement.cpp 63.6% → 77.0% (184/239)** — the remaining
   ~55 lines are the native open/save **file dialogs** + the to-disk save path, which can't run headless
   (the save→reopen cycle is covered by tst_e2e). savefile real-source overall → **77.9% (4600/5905)**, 9
   cases green (47/47 full suite)._
   _**Off-by-one found AND fixed 2026-06-08 (Twilight-approved after confirming intent):**
   `processRecentFileChanges()` capped with `append(file); if(size > MAX_RECENT_FILES) break;` —
   appending *then* breaking retained **MAX+1** (6, not 5). Twilight asked to verify the extra slot
   wasn't an intentional sentinel first; it wasn't — `mainwindow` bounds its menu loop at
   `MAX_RECENT_FILES` (and its shortcut array is sized MAX, so a 6th is never read), and
   `RecentFilesModel` uses `recentFilesCount()` directly (its `+1` is just the row-0 "Clear Recent Files"
   header). So the 6th slot only leaked one extra path into the QML list. Fixed to `>= MAX_RECENT_FILES`
   → exactly 5. `tst_filemanagement::recents_capBoundsTheList` is now the regression guard (asserts ==5).
   See `reference/fix-patterns.md`._

   _Eighth file: **`tst_storage.cpp`** — the PC (Storage) beyond load/save round-trips: the flattened
   0..11 box space (boxCount/boxAt across both 6-box sets), freeSpace()'s room invariant, depositPokemon()
   success + the all-full→false path, and the randomize verbs (randomize/randomizeItems/randomizePokemon)
   running clean. **storage.cpp 78.6% → 98.0% (96/98)**; savefile real-source overall → **78.0%**, 7 cases
   green (48/48 full suite)._

   _Ninth file: **`tst_pokemonbox.cpp`** — the biggest single gap: the PokemonBox/PokemonMove logic the
   two existing pokemon suites don't reach (tst_pokemon = field round-trips, tst_pokemon_logic =
   DV/EV/level/heal/evolve/shiny/stats/nature). Covers PokemonMove PP-Up controls (max/raise/lower/reset
   + clamping), restorePP, changeMove, moveType (valid/glitch/empty), isInvalid, allValidMoves/
   validMovesLeft/isDuplicateMove, correctMove; and PokemonBox's constrained **randomize()** (the big
   ~70-line method, 8-seed invariant sweep), **newPokemon** across all four PokemonRandom_ scopes, resetExp,
   expLevelRangePercent, **setNature** (incl. the level-range clamp + same-nature no-op), hasTradeStatus,
   changeOtData/changeTrade (both directions + idempotent no-op + null-basics guard), cleanupMoves/
   correctMoves/update(correctMoves), changeMove(ind,…), the manual* UI hooks, reRollEVs/maxPpUps, dexNum/
   speciesName, isPokemonReset/isCorrected, isBoxMon, and a dedicated invalid-mon (species 0) safe-default
   sweep (dexNum=-1, hpStat=1 floor, levelToExp=0, expLevelRange* fall back to raw exp, resetExp no-op).
   **pokemonbox.cpp 72% → 94.4% (1071/1134, 63 missed)**; savefile real-source overall → **82.5%
   (4879/5916)**, 19 cases green (49/49 full suite). Three latent bugs were brought to Twilight and,
   with her approval, **FIXED in pokemonbox.cpp this pass** (the tests now assert the corrected behaviour
   and stand as regression guards): (1) `update()` with `resetType=false` ran its bare `else type2 =
   toType1->ind` on every call, overwriting a **dual-type** mon's type2 with type1 (silently dropping the
   second type; emitted no `type2Changed()`) — reachable via maxLevel()/maxEVs()/resetEVs()/reRollEVs()/
   manualLevelChanged(). Fixed: the type2 (re)derivation is now wrapped in `if(resetType)`, so a non-reset
   update leaves type2 untouched. (2) `isCorrected()` disagreed with `update()` for a species whose DB
   `toType2`==`toType1` (update collapses to type2=0xFF; isCorrected demanded type2==toType2->ind). Fixed:
   isCorrected now treats a record as dual-type only when toType2 genuinely differs from toType1, and
   accepts either 0xFF or type1 for a single type (faithful to the DB's mixed 0xFF-vs-duplicate storage).
   (3) `isPokemonReset()` could only ever be true for a species with 4 initial moves AND wrongly required
   `isMaxPpUps()` when a reset mon has 0 PP-Ups. **Root cause traced (Twilight asked it not be squashed
   aside): the empty-slot bug lived in `PokemonBox::isMaxPP()`/`isMaxPpUps()`** — they looped all four
   slots and counted an EMPTY slot (moveID 0) as "not maxed", which propagates into **`isHealed()`** so any
   mon with fewer than four moves could never read as healed (a user-facing wrong result on the heal
   indicator, not just isPokemonReset). Fixed at the source: `isMaxPP()`/`isMaxPpUps()` now skip empty
   slots (mirroring `isMaxedOut()`'s existing guard), so `isHealed()` is correct for any move count;
   `isPokemonReset()` then simplifies to iterate the real initial moves (empty after), check `ppUp==0`, and
   reuse `isHealed()`. Regression-guarded by `box_healedWithFewerThanFourMoves` (a <4-move mon at full
   HP/PP now reads isMaxPP + isHealed). **Also flagged for Twilight (NOT fixed, tracked in
   next-steps.md):** `isMinEvs()` uses `||`, so it returns true if ANY single EV is 0 (reads like it
   should be `&&` / all-zero); and the type2 single-WRITE truth (0xFF vs duplicate) is an open decision —
   the load-side tolerance is official, the save-side canonical form is a tracked temporary exception._

   **Cumulative savefile progress this pass: 72.9% → 82.5% across signdata(100%), warpdata(98.5%),
   mapconndata(100%), savefileiterator(100%), item(96.7%), playerbasics(98.9%), filemanagement(77%,
   headless ceiling), storage(98%), pokemonbox(94.4%) — plus the recent-files cap off-by-one and four
   pokemonbox type/reset/heal bugs fixed (Twilight-approved: type2-clobber, isCorrected dual-type,
   isMaxPP/isMaxPpUps/isHealed empty-slot, isPokemonReset), with the isMinEvs `||` and type2 single-write
   truth tracked for decision.** Next gap
   targets (worst remaining by missed lines): `spritedata.cpp` 46% (234 — note
   the disabled-randomizer sprite-link crash, test the safe paths), `areamap.cpp` 62% (83 — partly the
   disabled Maps `loadFromData`/`setTo`), `areapokemon.cpp` 61% (75), `areatileset.cpp` 62% (51),
   `pokemonbox.cpp` (68 — residual glitch/boundary branches: the move-randomize do-while reject, correctMove's
   single-row replacement, isMaxedOut's invalid-mon branch, setNature's boundary ±25 paths).

   **Key discovery (worth knowing for all map-DB tests):** `DB::deepLinkAll()`
   does **not** call `MapsDB::deepLink()` — map warps/sprites/connections are left unresolved at boot
   (it's only needed by the disabled map randomizer), so `getToMap()` is null everywhere until a test
   calls `MapsDB::inst()->deepLink()` itself. That call is stable headless (no crash; the warp deeplink's
   null-`toMap` crash is guarded by `#ifdef QT_DEBUG`, which is active in the Debug test build). The one
   warpdata line still missed is a minor branch, not chased._

   _**How the app/appcore number is measured (and why the earlier 56% was wrong):** unlike
   common/db/savefile (each a shared `.dll`, so per-test coverage merges cleanly), `appcore` is a
   **static** lib embedded into every app-test exe. Merging all `.profraw` and exporting against a
   single exe double-counts/garbles and gave an inflated ~56%. The reliable method is a **per-file
   max union**: measure each app-test exe against ITS OWN profile, then for each app source file take
   the best line coverage any test achieved and sum — that yields the honest **43.6%**. So the app
   layer is the real laggard and the current focus. (For a one-shot accurate number in future, build
   appcore SHARED in the coverage config.)_
   _**Confirmed attribution artifact (2026-06-08):** `tst_market_model` runs all 7 cases with 0 skips
   and its buy/sell checkout demonstrably change the player's money — yet llvm-cov reports the
   `mvc/itemmarket/itemmarketentry*.cpp` files (283-line `storeitem` etc.) at **0%**. The code is
   provably executed; llvm-cov on this llvm-mingw kit simply fails to attribute coverage for some
   statically-linked appcore translation units. So 43.6% is an under-count and the app % can't be
   trusted on this toolchain — judge app coverage by the passing functional suites, not the number.
   Making appcore a shared lib (at least for the coverage build) would fix the measurement._
   _**RESOLVED 2026-06-08:** added `option(PSE_SHARED_APPCORE)` to `app/CMakeLists.txt` — pass
   `-DPSE_SHARED_APPCORE=ON` for the coverage build and appcore builds as a single shared `.dll`
   (WINDOWS_EXPORT_ALL_SYMBOLS auto-exports, no per-class macros). All test exes then share one
   instrumented appcore, so coverage merges cleanly: **app/appcore is actually 58.2% line / 64.7%
   func** (the 0%/43.6% figures were the static-lib artifact; the itemmarket entries now correctly
   show 50-55%). The default build is unchanged (option OFF → STATIC, as shipped)._
   _**BUT the app number is still noisy** even with shared appcore: repeated full-suite measurements
   of the SAME code have read 50%, 56%, 58% (and the static build gave 44/47%). llvm-cov on this
   llvm-mingw kit doesn't merge appcore coverage deterministically across the ~14 app-test exes. The
   three library layers (common/db/savefile) are stable to the decimal across every run, so trust
   those; treat the app % as a rough ~50-58% band and judge app coverage by the passing functional
   suites (every model, both list modes, all four market modes, bulk ops, bridge, settings, engine
   math)._
   _**Engine providers + QML behavioural now covered too (2026-06-08):** `tst_engine_providers` is a
   GUI test (QTEST_MAIN, offscreen) that compiles the app qrc and drives TilesetEngine's
   resource-backed builders, TilesetProvider, and FontPreviewProvider (incl. malformed-id fallbacks);
   `tst_qml_brg` boots a real Bridge as the QML `brg` context property and drives the C++<->QML
   property chain from QML (the undefined-chain guard). So no app tier is wholly untested now — the
   only remaining QML work is full screen-flow tests (commit-on-blur etc.), which stay optional /
   low-ROI per Twilight's call._

   Trend across the effort (line): common 65 → **79** (Random chance helpers), savefile 63 → 67 → 68 →
   **72** (area, area-fragments, pokedex, items-logic, misc-regions), db 51 → 53 → **56** (db-entries +
   db-stores), **app 0 → 3.5 → 20 → 39 → 56** (bridge, then storage/market models + tileset engine).
   Method: instrument all libs + appcore, run every test exe with a unique `LLVM_PROFILE_FILE`,
   `llvm-profdata merge`, then `llvm-cov export -instr-profile merged.profdata -summary-only
   <lib.dll|tst_bridge.exe> <src-dir>` per module (app measured against `tst_bridge.exe`, which
   statically links all of appcore). db then climbed 56 → **63** (db-entry-getters, db-stores, fonts,
   mapsearch, map-subentries, random-strings). Biggest remaining climbs: app `engine/` providers +
   Router edge paths (app is now the lowest line layer), savefile leaf getters, and the remaining
   per-entry getters in db._
   _**AddressSanitizer: not viable on this Windows/llvm-mingw kit.** The ASan build compiles
   (`projects/build/asan`, `-fsanitize=address`), but every instrumented exe crashes at startup with
   `interception_win: unhandled instruction` (0xC0000005) before any test runs — a known ASan-on-Windows
   limitation in its API-interception engine, NOT a bug in our code (the same tests pass under the normal
   + coverage builds). Real memory-sanitizer coverage (ASan/UBSan/Valgrind) should run in a **Linux CI
   job** (phase 9), where ASan is mature. The use-after-free risk is meanwhile addressed by the
   qmlProtect/qmlCppOwned fixes and the null-deref bugs this suite already found + fixed._
8. **CI** (ctest + sanitizer + coverage on push).
   _Workflow written 2026-06-07: `.github/workflows/tests.yml` — a **linux-asan** job (installs Qt via
   aqt, builds with ASan+UBSan which work on Linux, runs `ctest` headless/offscreen) and a **windows**
   job (Qt llvm-mingw, matches Twilight's kit). Not yet exercised on GitHub — the Qt module/arch names
   may need a first-run tweak (noted in the file). This is the right home for ASan, which can't run on
   the local Windows kit._
   _**Cross-platform gotcha (CI first-run shakeout, 2026-06-08):** the local kit is Windows
   (case-INsensitive filesystem); the linux-asan CI is case-SENSITIVE. A few db headers are
   mixed-case — `spriteSet.h`, `starterPokemon.h`, `tmHm.h`, `hiddenItemsdb.h` — so an `#include
   <pse-db/spriteset.h>` compiles locally but fails on Linux with "No such file or directory". Match
   header case exactly in includes. (Also surfaced: the Qt module fix, qtdeclarative/qtsvg are base
   not add-ons; and `tests_all` aggregate so new suites are always built.)_
9. **app C++** (Bridge, models, FileManagement) headless.
   _Unblocked 2026-06-07 (Twilight-approved refactor): the app's logic — `bridge/`, all `mvc/` models,
   `engine/` — was extracted from the executable into a **static library `appcore`** (`app/CMakeLists.txt`);
   the exe is now just main/boot/mainwindow/QML-resources linking `appcore`. Tests can now link the app
   layer. First app-layer tests: `tests/mvc/tst_models.cpp` (TypesModel, NatureSelectModel — rows/roles/
   data/round-trip), green. The app exe still builds. **Gotcha fixed:** splitting into a lib broke the
   single-target "unity MOC" that had been making `FileManagement` complete for `mainwindow.h`'s
   `Q_PROPERTY(FileManagement*)` — added the explicit `#include` (NOT an opaque pointer; brg.file is
   QML-traversed)._
   _Extended 2026-06-08 (`tst_models.cpp` now 9 models) and **`tst_bridge.cpp`** — an integration suite
   that constructs the real `Bridge` over the BaseSAV fixture and covers the rest: Pokedex, both
   `ItemStorageModel`s, `ItemMarketModel`, both `PokemonStorageModel` halves + box selectors,
   `RecentFilesModel`, `FontSearchModel`, and `Router` navigation. 16/16 ctest green._
   _**Two app-layer test landmines (NOT bugs — call conventions to respect in tests):**_
   _1. `Router::screens`/`stack` are **static**, populated by `Router::loadScreens()` at app boot
      (`boot.cpp`), not by the constructor. A test that navigates must call `Router::loadScreens()` first,
      or `changeScreen()` dereferences a null `Screen*` and crashes._
   _2. `PokemonStorageModel::getBoxMon()/getPartyMon()` do an **unchecked `.at(index)`** — their contract
      is that QML only calls them for already-rendered rows. In a test, only call them with an index `<`
      the box's actual `pokemon.size()` (load a non-blank fixture if you want a non-empty box/party)._
10. **QML/UI** — Twilight opted in. _Harness established 2026-06-07: `tests/qml/` (Qt Quick Test,
    `QUICK_TEST_MAIN`) builds and runs headless via the `offscreen` platform; `cases/tst_smoke.qml`
    is green (6 cases). This proves the QML test path works. **Screen-level tests still to do** — the
    app's screens bind to `brg.*`, so testing them needs the Bridge + DB + registered types booted
    into the test engine (a larger harness); behavioural smoke tests (name commit-on-blur, editor
    reactivity, popup dismiss) come after that._

---

## QML / UI testing — DEFERRED (Twilight to decide)

**It is technically doable.** Qt Quick Test drives QML components, simulates clicks/keystrokes, and
asserts on properties, running headless via the `offscreen` platform plugin (so it works in CI).

**Why it's deferred — honest cost/benefit:**

- **Lower ROI here.** The bugs that actually threaten this project are byte-level (save corruption)
  and lifetime-level (use-after-free) — both fully covered by C++ tests + sanitizers. UI bugs are
  mostly cosmetic/layout, which Twilight already catches fast in live hot-reload iteration.
- **Highest maintenance burden.** UI tests are the most brittle kind: every layout tweak (and this
  project is in a heavy, ongoing UI-iteration phase — see `status.md`) risks breaking them, creating
  churn that competes with the actual design work.
- **Pixel/layout assertions are a trap.** Only *behavioral* flows are worth testing
  (commit-on-blur actually commits; popup actually dismisses; editor tab actually reacts), never
  positions/sizes — those are Twilight's live-owned design decisions.

**If greenlit, scope would be narrow:** a handful of behavioral smoke tests on the highest-value
flows (name commit-on-finish, Pokémon editor reactivity, popup open/dismiss), run offscreen, kept
deliberately small. Decision pending; revisit once the UI-polish phase settles.

---

## Open questions / decisions needed

- **QML/UI scope** — deferred above; Twilight to decide later.
- ~~**Fixtures**~~ — RESOLVED 2026-06-07: `assets/BaseSAV.new.sav` (fresh) + `assets/BaseSAV.sav`
  (progressed) exist, plus `assets/savefile-structure.bt` as the independent offset oracle. Still TODO:
  generate the negative-test corpus, and add Red-vs-Blue / additional game-state fixtures if we want a
  fuller compatibility matrix.
- **Characterize `BaseSAV.sav`** — its exact progress (party, badges, money, dex) isn't recorded; do
  one characterization pass and lock the values into a test so the fixture is self-documenting.
- **`.bt` consumption** — for phase 1, offsets are **transcribed as named constants** in the test
  (citing the `.bt`), e.g. `kMoneyOffset 0x25F3`, `kMainChecksum 0x3523`. Revisit if the offset set
  grows large: a small `.bt` parser or a shared generated offset table would track edits to the `.bt`
  automatically (constants can drift).
- **Minor .bt discrepancy to confirm (don't silently fix):** the `PLAY_TIME` struct lists all five
  members as `char hours` (the display `<name=...>` tags say Hours/Maxed/Minutes/Seconds/Frames). Looks
  like a copy-paste in the template, not a save-format claim — flag for Twilight, leave the .bt as-is.
- **Coverage gate strictness** — are the initial targets acceptable, and should CI *fail* below them
  or just report?
- **CI host** — GitHub Actions assumed; confirm the llvm-mingw/Qt 6.11 toolchain is reproducible
  there (or run CI on a self-hosted runner matching the dev box).
