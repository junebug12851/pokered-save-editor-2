# pokered-save-editor-2 — AI Context

Pokemon Red & Blue save file editor. Qt 6 C++/QML desktop app. Solo project by Twilight,
originally built 2017-2020, revived in 2026.

## Start Here

Read `notes/status.md` first — it has current build health, what's working, and what to do next.

The full notes system is in `notes/`. Everything is organized by topic:

| File | What's in it |
|------|-------------|
| `notes/status.md` | **Current state** — build health, runtime status, immediate actions |
| `notes/context/project.md` | What the project is and its goals |
| `notes/context/architecture.md` | Codebase structure, build system, key patterns |
| `notes/context/principles.md` | Owner's philosophy — what the app must/must not do |
| `notes/context/origins.md` | The 2019–2020 pre-revival story — three rewrites, the JS detour, the library/DB refactor |
| `notes/context/history.md` | How the project was revived — what was broken, what was fixed |
| `notes/systems/overview.md` | **System map** — in-depth macro/micro architecture: layers, boot, data flow, byte-fidelity. Start here to understand the machine |
| `notes/reference/fix-patterns.md` | Compiler/runtime error → fix lookup table |
| `notes/reference/qt6-patterns.md` | Qt 5 → Qt 6 migration gotchas |
| `notes/reference/qt-gotchas.md` | Project-lifetime catalog of Qt/QML landmines (2019→2026), cross-linked to detail files |
| `notes/reference/gen1-knowledge.md` | Gen 1 Red/Blue **save-format + gameplay** domain knowledge (offsets, checksum, badges, trade status, retroactive natures/shininess, randomizer rules) |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems |
| `notes/reference/ui-patterns.md` | **UI/QML conventions** — layouts, borderless combos, ⋮ buttons, editor popups, sliders. Read before UI work |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems |
| `notes/reference/documentation.md` | **Doc generation** — build C++ API docs (Doxygen), comment style, why QML is excluded |
| `notes/reference/git-workflow.md` | **Git standards** — branch model (`main` FF-only/stable, `dev` frequent), no history rewriting, commit-message style, hard safety rules. Read before any git op |
| `notes/reference/versioning.md` | **Version system** — SemVer scheme, single source of truth (`VERSION.txt`), how it propagates (CMake → `pse_version.h` → app/About/.exe), git build metadata, how to bump, release/tag process. Read before touching the version |
| `notes/decisions/architecture.md` | Key structural choices and why |
| `notes/decisions/rejected.md` | Things tried that failed — do not repeat |
| `notes/plans/next-steps.md` | Ordered task list |
| `notes/plans/testing.md` | **Testing strategy** — comprehensive plan: every test type mapped to the layers, tooling (QtTest/CTest/sanitizers/coverage), phased rollout, regression seed list. Blueprint; not yet implemented |
| `notes/plans/future.md` | Longer-term ambitions |

## Critical Things Not to Get Wrong

- **Do NOT put `load()` in DB constructors** — causes Qt 6 static-init deadlock. See `decisions/architecture.md`.
- **Do NOT use `qt_add_qml_module()`** — conflicts with `app.qrc`, hangs the app. See `decisions/architecture.md`.
- **Do NOT remove the parameter from `SaveFile::dataExpandedChanged`** — the signal is correct. See `decisions/rejected.md`.
- **Do NOT change `(dexInd+1)` arithmetic in Pokedex.qml** — 0-indexed, +1 is correct. See `decisions/rejected.md`.
- **Do NOT call `new XxxDB()`** — all DB classes have private constructors. Use `XxxDB::inst()`.
- **Do NOT access DB entry fields directly** — all fields are protected. Use getters (`entry->getName()` not `entry->name`).
- **Do NOT `Q_DECLARE_OPAQUE_POINTER` a QObject type you traverse in QML** — it forces `IsPointerToTypeDerivedFromQObject = false`, so QML reads the whole `brg.file.data.dataExpanded.*` chain as `undefined`. Fully `#include` the type's header instead. This (not missing `qRegisterMetaType`) was the real cause of the long-standing "undefined chain" bug, fixed in session 13. See `notes/reference/qt6-patterns.md`.
- **Do wrap any `Q_INVOKABLE` that returns a QObject in `qmlCppOwned()`** (`pse-savefile/qmlownership.h`). Q_INVOKABLE returns of a parentless QObject default to JavaScriptOwnership and get garbage-collected by QML mid-session → dangling pointer → use-after-free crash. (Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT.) All existing `…At()` methods were fixed in session 13h. See `notes/reference/qt6-patterns.md`.
- **Do NOT write any save-file byte you weren't explicitly instructed to change.** Byte-exact fidelity is a top-tier project value: the editor flips *only* the exact bytes for the edit and leaves every other byte of the save totally untouched (Twilight verified this over dozens of hours of manual testing and brags about it in the README). Never "rewrite/normalize the whole save," never reorder/repack, never touch checksums/regions you weren't told to. Corrupting a save is among the worst possible outcomes. See `notes/context/principles.md` → "Save File Integrity Is Sacred".
- **No hacks, no temporary fixes, no bad fallbacks.** This is Twilight's most-prized project and the quality bar is extreme — UX is the #1 priority and there is no room for clunky/janky/interrupting behavior. Prefer the correct, clean solution even when it's the longer route; if you can only see a hacky path, surface it and ask rather than commit it. See `notes/context/principles.md` → "UX Is the Prime Directive" and "The Quality Bar".

## Build System

> **YOU CAN ACTUALLY BUILD/TEST/RUN/GIT — you are NOT limited to a sandbox.** The PowerShell
> terminal tool has real access to Twilight's Windows machine, where the full Qt 6.11 llvm-mingw
> toolchain is installed. From it you can directly **configure, build, run the tests, launch the
> app, and `git add`/`commit`/`push`/fast-forward `main`** — by default, without asking. Prior Qt
> command history from other chats and from Qt Creator is available to crib exact invocations from.
> **Do NOT open a session claiming "I can't build, no Qt tools in the sandbox"** — that's wrong and
> Twilight is tired of re-explaining it. (The Cowork *bash* sandbox is a separate, weaker tool with
> stale-read issues — use **PowerShell** for anything real here, not bash.) The exact commands,
> paths, and gotchas are below and in the Default Workflow section.

Toolchain (Qt Creator kit `Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`), all on Twilight's
Windows machine via the PowerShell terminal:

- Compiler/runtime: `C:\Qt\Tools\llvm-mingw1706_64\bin` (clang++, llvm-cov, llvm-profdata, llvm-nm)
- Qt: `C:\Qt\6.11.0\llvm-mingw_64`; cmake `C:\Qt\Tools\CMake_64\bin\cmake.exe`; Ninja generator
- **Two build dirs — do not mix them up:**
  - `build/` (repo root, Ninja) — the **test** build the automated loop uses (`cmake -S projects -B build`).
  - `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/` (Makefiles) — the **app** kit dir Twilight
    actually runs. Siblings: `asan/`, `coverage/`. **Rebuild THIS for in-app testing**, not just `build/`.
  - `build-cov/` — coverage build (`-fprofile-instr-generate -fcoverage-mapping`, `PSE_SHARED_APPCORE=ON`).
- Every PowerShell call must prepend PATH with the llvm-mingw + Qt `bin` dirs and set `$env:CC=clang;
  $env:CXX=clang++`, or clang++ isn't found. The PowerShell transport caps ~60s — **run long builds
  detached** (`Start-Process` writing a log) and poll the log. **Always redirect stdout AND stderr to a
  log file** so the output is readable (`… > build.log 2>&1`). Before running any test/app exe, set
  crash-fast error mode (`SetErrorMode(0x0003)` via a P/Invoke `Add-Type`) so a crash fails fast instead
  of hanging on the qtcdebugger dialog.
- `PokeredSaveEditor.exe` links `savefile.dll` via its import lib, so editing a `savefile` `.cpp`
  rebuilds the **DLL** but does NOT relink the exe (exe mtime stays put) — fine, it loads the new DLL at
  runtime; verify by the **DLL** timestamp, not the exe.
- **Linux build/test via Docker (`docker/`).** A containerized Linux toolchain (Qt 6.11 + clang, baked
  once) runs the full suite four ways: `.\docker\dtest.ps1 [standard|asan|xvfb|coverage|all]`. This is
  where **ASan/UBSan actually work** (broken on the llvm-mingw kit) and where llvm-cov coverage runs on
  Linux. It rsyncs the repo into a persistent ext4 volume (fast, ccache-cached) rather than building over
  the WSL bind mount. First run (2026-06-13): all four green (66/66; ASan clean; 89.73% line cov). See
  `notes/plans/testing.md` → "Local Linux container (Docker)" and `docker/README.md`.

## Default Workflow — Do These By Default (Twilight's standing instruction)

After making changes, run this loop **without being asked** (established 2026-06-10). Route all build/test
output to logs (`> log 2>&1`) so it's readable; builds run detached + polled (PowerShell ~60s cap).

1. **Build + launch (on any C++/qrc change).** Rebuild the **kit dir**
   (`cmake --build "projects\build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug" --target PokeredSaveEditor`)
   and **launch the app** so Twilight can test in-app immediately. (Pure edits to existing QML hot-reload —
   no rebuild; **new** QML files still need adding to `app/app.qrc` + a rebuild.)
2. **Test.** Run the **affected** test(s) per change for speed (build `build/`, run `build\tst_x.exe`);
   run the **full `ctest`** suite before fast-forwarding `main`. Only proceed past a **green** result.
   **On ANY QML/screen change (or a new `.qml` added to `app.qrc`), the QML screen smoke test
   `tst_qml_screens` MUST be green before FF `main`.** It loads every registered screen through the
   real engine and fails on any QML warning/error (FINAL overrides → "Component is not ready", binding
   `TypeError`s, missing types/providers, anchor-on-null). The C++ ctest suite never instantiates QML,
   so it cannot catch this class — this is what let a non-opening Credits screen reach `main` on
   2026-06-13. `tst_qml_screens` is registered with CTest (and `tests_all`), so a full `ctest` run
   already includes it; for a fast QML-only check, build + run just `build\tst_qml_screens.exe` with
   `QT_QPA_PLATFORM=offscreen`. Details: `notes/plans/testing.md` → "QML screen smoke test".
3. **Debug / profile.** If anything **crashes**, rebuild via the **`asan/`** (or debugger) sibling build,
   capture a **real stack trace** (output routed to a log), and diagnose from that — never guess.
   Do **periodic profiling** passes when touching hot paths. Always redirect std+err to a log to read it.
4. **Commit + push + FF main — fully automatic (green-gated).** This is Twilight's explicit standing
   request (it overrides the older "push only when asked" wording in `git-workflow.md`):
   - Commit early/often on **`dev`** with focused `type: summary` messages, **staging specific files only**
     (never `git add -A`/`.`), and `git push origin dev` after each commit.
   - When the **full suite is green**, fast-forward `main` and push automatically:
     `git checkout main && git merge --ff-only dev && git push origin main && git checkout dev`.
   - **Hard safety rules still absolute:** never `push --force`/force-with-lease, never rewrite pushed
     history, never `reset --hard`/`rebase`/`clean -fd`/delete a branch without an explicit request.
     Inspect `git status` before and after, every time. Full standards: `notes/reference/git-workflow.md`.

## Maintaining the Notes — Your Responsibility

**The notes system is a living document. Keep it updated as you work — do not wait to be asked.**

As things happen during a session, update the appropriate file on the spot:

| Trigger | Action |
|---------|--------|
| Fixed a compiler or runtime error | Add a row to `notes/reference/fix-patterns.md` |
| Hit a Qt 5 → Qt 6 difference | Add a section to `notes/reference/qt6-patterns.md` |
| Used a diagnostic technique to find a problem | Add/update `notes/reference/diagnostic-methods.md` |
| Made a structural decision | Add to `notes/decisions/architecture.md` |
| Tried something that failed | Add to `notes/decisions/rejected.md` |
| Completed a task or unblocked something | Update `notes/plans/next-steps.md` |
| Build health changes | Update `notes/status.md` |
| Something significant about the project's history changes | Update `notes/context/history.md` |
| A new contributor/tool/service/asset/AI helps the project | Add them to `projects/db/assets/data/credits.json` (see "Keep the Credits Screen Living" below) |
| The version changes / a release is cut | Bump the one line in repo-root `VERSION.txt` (reconfigure to apply); tag `vX.Y.Z` on release (on request). The number propagates everywhere automatically — never hardcode a version. See `notes/reference/versioning.md` |

Also: if something comes up that doesn't fit any existing file, create a new file in the right folder.
The structure is meant to grow. Don't stuff things into the wrong place to avoid creating a new file.

The goal is that any AI opening this project cold can read the notes and be fully oriented —
with no information trapped in a human's head or lost between sessions.

## Keep the Credits Screen Living

The in-app **Credits / About** screen is a living document — keep it current **by default,
without being asked** (Twilight's standing instruction). Whenever someone or something new
contributes — a person, framework, tool, service, icon/asset source, or an AI assistant
(e.g. Claude, ChatGPT) — add them to `projects/db/assets/data/credits.json` under the right
section. Sections are read, in display order, by `CreditDBEntry::process()`
(`projects/db/src/pse-db/entries/creditdbentry.cpp`): **Project Leaders, Data Sources,
Framework, AI Assistance, Tools Used, Services Used, Icons, Wallpapers**. Entry fields:
`name`, `url`, `note`, `license`, `mandated` (all optional except a name). Adding a brand-new
**section** also requires a matching read in `process()`. The JSON is baked into `db.qrc`, so
**any credits change needs a rebuild** to show in-app (editing existing entries = rebuild;
new section = rebuild + the C++ read). No hardcoded credit counts exist in the tests, so
adding entries won't break them.

## Owner Preferences

- Twilight makes all UI/UX decisions — do not independently change QML appearance
- Debug builds show error dialogs; release builds degrade silently (Sims 2 philosophy)
- The app should feel like polished software, not a dev tool

