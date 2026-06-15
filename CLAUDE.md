# pokered-save-editor-2 — AI Context

Pokemon Red & Blue save file editor. Qt 6 C++/QML desktop app. Open source, built by Twilight.
Originally built 2017-2020, revived in 2026.

## Start Here

Read `notes/status.md` first — it has current build health, what's working, and what to do next.

The full notes system is in `notes/`. Everything is organized by topic:

| File | What's in it |
|------|-------------|
| `notes/status.md` | **Current state** — build/runtime health, open issues, immediate actions. Start here |
| `notes/sessions/` | **Session logs**, one file per day grouped in month folders (`YYYY-MM/YYYY-MM-DD.md`) — the day-by-day story of what changed and why. `sessions/README.md` defines the system; `revival-s13-series.md` holds the undated pre-corruption revival narrative |
| `notes/version.md` | **Changelog** — the plain-English, one-entry-per-commit history (index; months under `notes/version/`). NOT the version-number scheme (that's `reference/versioning.md`) |
| `notes/context/project.md` | What the project is and its goals |
| `notes/context/architecture.md` | Codebase structure, build system, key patterns |
| `notes/context/principles.md` | Project philosophy — what the app must/must not do |
| `notes/context/origins.md` | The 2019–2020 pre-revival story — three rewrites, the JS detour, the library/DB refactor |
| `notes/context/history.md` | How the project was revived — what was broken, what was fixed |
| `notes/systems/overview.md` | **System map** — in-depth macro/micro architecture: layers, boot, data flow, byte-fidelity. Per-layer deep-dives in `notes/systems/{common,savefile,db,app,qml}.md`. Start here to understand the machine |
| `notes/reference/fix-patterns.md` | Compiler/runtime error → fix lookup table |
| `notes/reference/qt-patterns.md` | **The Qt/QML reference** — the project-lifetime catalog of every Qt/QML landmine (2019→2026) + the detailed Qt 5→6 patterns (with code) + case studies. (Merged: was `qt6-patterns.md` + `qt-gotchas.md` + `player-name-hang.md`) |
| `notes/reference/gen1-knowledge.md` | Gen 1 Red/Blue **save-format + gameplay** domain knowledge (offsets, checksum, badges, trade status, retroactive natures/shininess, randomizer rules); box-recovery deep-dive in `notes/reference/box-recovery-research.md` |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems (truncation, hangs, QML chain failures, transcript recovery) |
| `notes/reference/ui-patterns.md` | **UI/QML conventions** — layouts, borderless combos, ⋮ buttons, editor popups, sliders, drag & drop, View All drawers. Read before UI work |
| `notes/reference/screenshots.md` | **UI screenshot + animation capture** — the headless `screenshooter` tool + Pillow GIF assembler + capture scripts that render the live UI to `tmp/screenshots/` (offscreen, no save writes). How it's driven + the font/backend gotchas |
| `notes/reference/i18n.md` | **Translations** — Qt Linguist pipeline (`qsTr`/`tr` → `.ts`/`.qm` at `:/i18n`, `QTranslator` in boot), how to add a language; language switching deferred until a 2nd locale + Options screen exist |
| `notes/reference/documentation.md` | **Docs** — generating the Doxygen site, the comment house-style, and the doc-pass progress ledger (all merged here) |
| `notes/reference/git-workflow.md` | **Git standards** — branch model (`main` FF-only/stable, `dev` frequent), no history rewriting, commit-message style, hard safety rules. Read before any git op |
| `notes/reference/versioning.md` | **Version-number scheme** — SemVer, single source of truth (`VERSION`), how it propagates (CMake → `pse_version.h` → app/About/.exe), how to bump, release/tag process. (The *changelog* is `notes/version.md`) |
| `notes/reference/deployment.md` | **Releases / deployment** — the GitHub Actions `release.yml` pipeline: builds Windows portable+zip+installer, Linux AppImage+tar.gz, Doxygen docs zip, screenshots zip, and publishes a GitHub Release on each `main` commit that bumped `VERSION` (tag-gated). Toolchain mirrors `tests.yml`; first-run shakeout points noted |
| `notes/decisions/architecture.md` | Key structural choices and why |
| `notes/decisions/rejected.md` | Things tried that failed — do not repeat |
| `notes/plans/next-steps.md` | Ordered task list |
| `notes/plans/testing.md` | **Testing** — the suite (QtTest/CTest, GUI harness, Docker variants, coverage) and remaining gaps. Live, not a blueprint — full `ctest` is green |
| `notes/plans/future.md` | Longer-term ambitions |

## Critical Things Not to Get Wrong

- **Do NOT put `load()` in DB constructors** — causes Qt 6 static-init deadlock. See `decisions/architecture.md`.
- **Do NOT use `qt_add_qml_module()`** — conflicts with `app.qrc`, hangs the app. See `decisions/architecture.md`.
- **Do NOT remove the parameter from `SaveFile::dataExpandedChanged`** — the signal is correct. See `decisions/rejected.md`.
- **Do NOT change `(dexInd+1)` arithmetic in Pokedex.qml** — 0-indexed, +1 is correct. See `decisions/rejected.md`.
- **Do NOT call `new XxxDB()`** — all DB classes have private constructors. Use `XxxDB::inst()`.
- **Do NOT access DB entry fields directly** — all fields are protected. Use getters (`entry->getName()` not `entry->name`).
- **Do NOT `Q_DECLARE_OPAQUE_POINTER` a QObject type you traverse in QML** — it forces `IsPointerToTypeDerivedFromQObject = false`, so QML reads the whole `brg.file.data.dataExpanded.*` chain as `undefined`. Fully `#include` the type's header instead. This (not missing `qRegisterMetaType`) was the real cause of the long-standing "undefined chain" bug, fixed in session 13. See `notes/reference/qt-patterns.md`.
- **Do wrap any `Q_INVOKABLE` that returns a QObject in `qmlCppOwned()`** (`pse-savefile/qmlownership.h`). Q_INVOKABLE returns of a parentless QObject default to JavaScriptOwnership and get garbage-collected by QML mid-session → dangling pointer → use-after-free crash. (Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT.) All existing `…At()` methods were fixed in session 13h. See `notes/reference/qt-patterns.md`.
- **Do NOT write any save-file bit or byte you weren't explicitly instructed to change.** Bit- and byte-exact fidelity is a top-tier project value: the editor changes *only* the exact bits and bytes for the edit and leaves every other bit and byte of the save totally untouched — even unused/unallocated bits are precious; a single unintended bit flip is unacceptable (this has been verified over many hours of manual testing). Never "rewrite/normalize the whole save," never reorder/repack, never touch checksums/regions you weren't told to. Corrupting a save is among the worst possible outcomes. See `notes/context/principles.md` → "Save File Integrity Is Sacred".
- **No hacks, no temporary fixes, no bad fallbacks.** The quality bar here is high — UX is the #1 priority and there is no room for clunky/janky/interrupting behavior. Prefer the correct, clean solution even when it's the longer route; if you can only see a hacky path, surface it and ask rather than commit it. See `notes/context/principles.md` → "What the App Should Feel Like".

## Build System

> **YOU CAN ACTUALLY BUILD/TEST/RUN/GIT — you are NOT limited to a sandbox.** The PowerShell
> terminal tool has real access to the local Windows machine, where the full Qt 6.11 llvm-mingw
> toolchain is installed. From it you can directly **configure, build, run the tests, launch the
> app, and `git add`/`commit`/`push`/fast-forward `main`** — by default, without asking. Prior Qt
> command history from other chats and from Qt Creator is available to crib exact invocations from.
> **Do NOT open a session claiming "I can't build, no Qt tools in the sandbox"** — that's wrong and
> do not repeat this misconception. (The Cowork *bash* sandbox is a separate, weaker tool with
> stale-read issues — use **PowerShell** for anything real here, not bash.) The exact commands,
> paths, and gotchas are below and in the Default Workflow section.

Toolchain (Qt Creator kit `Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`), all on the local
Windows machine via the PowerShell terminal:

- Compiler/runtime: `C:\Qt\Tools\llvm-mingw1706_64\bin` (clang++, llvm-cov, llvm-profdata, llvm-nm)
- Qt: `C:\Qt\6.11.0\llvm-mingw_64`; cmake `C:\Qt\Tools\CMake_64\bin\cmake.exe`; Ninja generator
- **Two build dirs — do not mix them up:**
  - `build/` (repo root, Ninja) — the **test** build the automated loop uses (`cmake -S projects -B build`).
  - `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/` (Makefiles) — the **app** kit dir
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

## Default Workflow — Do These By Default (a standing instruction)

After making changes, run this loop **without being asked** (established 2026-06-10). Route all build/test
output to logs (`> log 2>&1`) so it's readable; builds run detached + polled (PowerShell ~60s cap).

1. **Build + launch (on any C++/qrc change).** Rebuild the **kit dir**
   (`cmake --build "projects\build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug" --target PokeredSaveEditor`)
   and **launch the app** so it can be tested in-app immediately. (Pure edits to existing QML hot-reload —
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
4. **Commit + push + FF main — fully automatic (green-gated).** This is an explicit standing
   request (it overrides the older "push only when asked" wording in `git-workflow.md`):
   - **Changelog rides inside the commit (write it BEFORE committing).** For any substantive change,
     write its plain-English entry at the top of the current month's file in `notes/version/` and stage
     it in the **same commit** as the change — one commit carries both. Inline entries take **no
     `<!-- commit: hash -->` marker and no short-hash byline** (a commit can't hold its own hash; `git
     blame` the entry line to find its commit). **Never** make a separate "document the last commit"
     commit, and **never** give a changelog/notes-only maintenance commit its own entry — that recursion
     (commit → entry → commit → …) is exactly what this rule prevents. See `notes/version.md` →
     "How this is kept updated (the inline rule)".
   - **Keep `VERSION` current — bump it inside the same commit when a change warrants it.** **PATCH**
     for a bug fix / small change, **MINOR** for a feature / notable change (you decide). **NEVER bump
     MAJOR** (`→ 1.0.0`) — that's the project leaders' (Twilight's) call only. Docs / notes / test /
     CI-only commits don't move the number. See `notes/reference/versioning.md`.
   - Commit early/often on **`dev`** with focused `type: summary` messages, **staging specific files only**
     (never `git add -A`/`.`), and `git push origin dev` after each commit.
   - When the **full suite is green**, fast-forward `main` and push automatically:
     `git checkout main && git merge --ff-only dev && git push origin main && git checkout dev`.
   - **"Green" now includes the GitHub Actions CI, not just local `ctest` (standing request).** Before
     fast-forwarding `main`, confirm the remote **`tests`** workflow passed on the `dev` HEAD being
     merged — `gh run list --branch dev -L 1` / `gh run view <id>` (the GitHub CLI is installed +
     authed). If that CI run is still in progress, wait for it; if it failed, treat it exactly like a
     local red and do **not** FF. Local `ctest` green is necessary but no longer sufficient.
   - **After FF `main`, watch the `release` run** — pushing `main` triggers `release.yml`, which (when
     `VERSION` was bumped → tag `v<VERSION>` is new) builds + publishes the GitHub Release. Monitor it
     with `gh run watch` / `gh run view --log-failed`; a failed build leaves NO tag/release, so fix
     forward and the next `main` push retries the same version. See `notes/reference/deployment.md`.
   - **After fast-forwarding `main`, rebuild the Doxygen docs by default** — `doxygen Doxyfile` from the
     repo root — so the generated `docs/html/` (git-ignored) always tracks `main`. See
     `notes/reference/documentation.md`.
   - **Also after fast-forwarding `main`, refresh the UI screenshots by default** —
     `pwsh -File scripts/capture_screenshots.ps1` (Linux/CI: `scripts/capture_screenshots.sh`) — so the
     `tmp/screenshots/` PNGs + GIFs (git-ignored; never committed) always track `main`. It builds + runs
     the headless `screenshooter` tool (renders only, never writes a save byte) and assembles the GIFs
     with Pillow (skipped cleanly if Python/Pillow is absent). See `notes/reference/screenshots.md`.
   - **Hard safety rules still absolute:** never `push --force`/force-with-lease, never rewrite pushed
     history, never `reset --hard`/`rebase`/`clean -fd`/delete a branch without an explicit request.
     Inspect `git status` before and after, every time. Full standards: `notes/reference/git-workflow.md`.

## GitHub Is Part of Default Management (a standing instruction)

The GitHub CLI (`gh`) is installed + authenticated (account `junebug12851`), so GitHub state is part of
the normal workflow — not something to wait to be asked about. The cadence is **event-based, not a
calendar** (Twilight's call): the trigger is **preparing `main` for shipment**, not a timed ping.

- **Whenever prepping `main` for shipment** (i.e. about to FF `main`), do a quick GitHub check as part of
  the same step: `gh run list` (CI/release health — must be green; see Default Workflow step 4), plus
  `gh issue list` and `gh pr list`. If there are **open/new/changed issues or PRs**, surface them to
  Twilight as a short summary and **ask whether to work on them now or later** — don't silently start.
- **Non-trivial issues / PR reviews are usually their own chat.** Offer to spin one up rather than
  derailing the shipment; a quick "these are open — now or later?" is the default, not diving in.
- **No timed/scheduled pings** unless Twilight later asks for one — the check rides on the
  shipment-prep event. (If wanted, a scheduled digest can be added with the scheduled-tasks tools.)
- **Never auto-act on issues/PRs** (no closing issues, merging PRs, or pushing to PR branches) without an
  explicit go-ahead — surfacing + asking is the default, acting is opt-in. Hard git safety rules apply.

### Release & publishing policy (standing rules)

- **GitHub Releases are for SOFTWARE releases ONLY — never an images-only or otherwise non-software
  release.** The versioned `release.yml` release (Windows installer/portable, Linux AppImage/tar.gz, docs
  zip, screenshots zip) is the only kind. Don't create side releases just to host files.
- **Every Release gets a well-written title + description by default** — clear, concise, informative,
  structured/organized (a downloads table, the prerelease/unsigned note, etc.). `release.yml` composes
  this automatically (`Compose release notes` step → `body_path`) with the auto "What's Changed" appended;
  keep that quality bar if you touch it.
- **The GitHub Pages site (docs + screenshots) is deployed by `pages.yml`, not git or a release.** On
  every `main` push it builds the Doxygen docs + captures the screenshots and deploys one Pages site:
  the **Doxygen home is the root** (`…github.io/pokered-save-editor-2/`) with **Screenshots + GitHub**
  custom nav tabs (injected via a generated `DoxygenLayout.xml`, README untouched), and the images live
  at `…/screenshots/<name>` (no `frames/`) — zero repo-size growth, no third-party host. README + docs
  embed the absolute `https://junebug12851.github.io/pokered-save-editor-2/screenshots/<name>` URLs. See
  `notes/reference/deployment.md`.

## Maintaining the Notes — Your Responsibility

**The notes system is a living document. Keep it updated as you work — do not wait to be asked.**

As things happen during a session, update the appropriate file on the spot:

| Trigger | Action |
|---------|--------|
| Did work worth recording this session | Append an entry to today's `notes/sessions/YYYY-MM/YYYY-MM-DD.md` (create the file — and the `YYYY-MM/` month folder if it's a new month — if it's the first entry today; newest on top). **If you CREATE a new day file, add its `\subpage` to `notes/_nav.dox`** (newest-first under the month hub; a new month → add a month-hub page too) or it floats to the Doxygen root. See `notes/sessions/README.md` |
| Fixed a compiler or runtime error | Add a row to `notes/reference/fix-patterns.md` |
| Hit a Qt 5 → Qt 6 difference or any Qt/QML landmine | Add a section/row to `notes/reference/qt-patterns.md` |
| Used a diagnostic technique to find a problem | Add/update `notes/reference/diagnostic-methods.md` |
| Made a structural decision | Add to `notes/decisions/architecture.md` |
| Tried something that failed | Add to `notes/decisions/rejected.md` |
| Completed a task or unblocked something | Update `notes/plans/next-steps.md` |
| Build health changes | Update `notes/status.md` |
| Made any substantive commit | Write its changelog entry inline in `notes/version/YYYY-MM.md` and stage it in the **same** commit (no marker; never a separate doc commit). See Default Workflow step 4 + `notes/version.md` |
| Something significant about the project's history changes | Update `notes/context/history.md` |
| A new contributor/tool/service/asset/AI helps the project | Add them to `projects/db/assets/data/credits.json` (see "Keep the Credits Screen Living" below) |
| A change warrants a new version (keep `VERSION` current by default) | Bump the one line in repo-root `VERSION` in the **same commit** — **PATCH** for a fix/small change, **MINOR** for a feature; **never MAJOR** (leaders-only). Reconfigure to apply; tag `vX.Y.Z` on release (on request). Never hardcode a version. See `notes/reference/versioning.md` |
| Created ANY new Markdown note in the Doxyfile `INPUT` (a session day, a new month folder, a `reference/`/`decisions/`/etc. page, a new changelog month) | Add its `\subpage` to `notes/_nav.dox` under the right hub, **same commit** — a page with no entry floats flat to the top of "Related Pages" on the Doxygen/Pages site instead of nesting. New month/folder → add its hub page too. (This bites easily — today's session + new reference docs all floated until fixed, 2026-06-15.) See the hard rule atop `notes/_nav.dox` |

Also: if something comes up that doesn't fit any existing file, create a new file in the right folder.
The structure is meant to grow. Don't stuff things into the wrong place to avoid creating a new file.

The goal is that any AI opening this project cold can read the notes and be fully oriented —
with no information trapped in a human's head or lost between sessions.

## Keep the Credits Screen Living

The in-app **Credits / About** screen is a living document — keep it current **by default,
without being asked** (a standing instruction). Whenever someone or something new
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

**Also regenerate `projects/db/assets/data/credits.md` whenever you edit `credits.json`** — it's the
human-readable Markdown rendering of the same data (linked from the root `README.md` and built into
the Doxygen docs under "Project & Repository"). Keep the two in sync: edit the JSON, then rewrite the
`.md` from it (same sections/entries, each `name` linked to its `url`, with note/license/mandated
shown). The JSON is the source of truth; the `.md` is a generated view.

## Project Preferences

- UI/UX decisions are a design decision — do not independently change QML appearance
- Debug builds show error dialogs; release builds degrade gracefully and clearly (never silently swallow errors, and never at the cost of save data)
- The app should feel like polished software, not a dev tool

