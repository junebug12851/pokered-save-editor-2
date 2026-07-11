# Working With Twilight — Preferences, Standing Rules & Cross-Cutting Feedback

The canonical home for the standing preferences, working rules, and feedback that used to live only in
an AI's per-conversation "memory." **The project notes are the source of truth — read them at session
start and keep them current; do not stash project knowledge in any external/personal memory** (Twilight,
2026-07-10). This file consolidates that knowledge so nothing is trapped in a side channel. Where a rule
has a deeper home, it's cross-referenced.

---

## Who Twilight is

- **Goes by Twilight.** She formerly used "June" / "June Hanabi"; do **not** use those (removed from the
  project 2026-06-05). GitHub handle `junebug12851` stays (still her account).
- Solo developer, deep Qt C++/QML expertise, intimately familiar with her own codebase from years of
  solo work. This project is one of her proudest — written meticulously, line by line, and the 2026
  revival is a genuine second chance to finish something that matters to her personally. Treat it that way.
- **Prefers concise, direct responses.** Don't over-explain things she already knows; focus on the
  problem at hand.

## Working style & collaboration

- **She makes ALL UI/UX decisions.** Implement what's asked; never propose or make unsolicited changes to
  QML appearance. (See `principles.md`, `feedback_pokered` never-change list below.)
- **Preserve her own notes/knowledge verbatim; don't silently "correct" them.** When integrating her
  scattered files/comments into the notes, keep what she wrote and flag any discrepancy as an open
  "verify before relying on this" question — annotate, don't rewrite. (She once had me stop "fixing" the
  Gen 1 growth-rate formulas in `growth-notes.txt`; she may have pasted them for a reason and they may
  matter to the algorithms.)
- **Track stacked-up asks as tasks by default.** When she fires several requests in a row, capture them
  all in the task list so nothing slips (established 2026-07-10).
- **Manual screenshot review is mandatory on any UI change** — capture and actually scrutinise it
  yourself before declaring done. See CLAUDE.md → Default Workflow step 1, `screenshots.md`, `ui-patterns.md`.

## Content & spelling rules

- **Always write "Poké" / "Pokémon" with the accented é** everywhere user-visible — app UI, GitHub
  release titles/descriptions, README, notes, commit messages (Poké Ball, Pokédex, Poké Mart, etc.).
  Plain "Poke"/"Pokemon" is a typo. **Display text only** — never change internal identifiers (uppercase
  item `name` keys like "POKE BALL", route ids like "pokemart", QML filenames `Pokemart.qml`, C++
  symbols, lookup strings). Changing `readable` names in data JSON is indexed — verify nothing looks it
  up by the old string, and the ask-first-on-JSON rule still applies.
- **Targets the US English release of Pokémon Red & Blue (Gen 1)** specifically — not Japanese, not the
  EU localizations. Use US item/move/location names and US mechanics; note Red/Blue version differences
  where relevant. The `pret/pokered` disassembly is the US-version oracle.
- **`brg.settings.primaryColor` is PINK (`#d81b60`), not red.** For red on screens use
  `brg.settings.errorColor` (a fixed, theme-independent red token added 2026-06-15, left untouched by
  `setColorScheme()`); don't use `primaryColor` as "red" and don't hardcode literal `"red"` (fights the
  planned theming pass). Pokémart already uses `errorColor`; migrate others over time.

## Data & files

- **Don't auto-edit data JSON — ask first.** Any change to DATA in a `.json` (e.g.
  `projects/db/assets/data/*.json` — items, gameCorner, prices, maps, credits) needs Twilight's OK first;
  the JSON is curated game data (often hand-tuned against `pret/pokered`). Read freely; prefer fixing the
  C++/QML logic that *uses* the data. (E.g. the Game Corner rate `price: 20` was correct; the bug was the
  exchange math using it inverted — fixed the code, left the JSON alone.)
- **Save-file fidelity is sacred** — only ever write the exact bits/bytes for the requested edit; never
  normalize/repack/touch checksums or regions you weren't told to. (See `principles.md`.)

## Credits (keep proactively)

- Twilight tends to forget credits and explicitly asked me to **proactively flag & add them** for any new
  asset, tool, service, data source, or AI assistance — don't wait to be asked. Credits live in
  `projects/db/assets/data/credits.json` (baked into `db.qrc` → change needs a rebuild) + the generated
  `credits.md`. **ChatGPT (OpenAI)** created the AI-generated artwork (status icons, gym-badge images,
  gym-leader portraits, and the rival artwork; the `trainer.png` illustration is the same lineage,
  historically uncredited). **Claude (Anthropic)** is credited for the 2026 revival/debug/test/notes.
  See CLAUDE.md → "Keep the Credits Screen Living".

## Version numbering (SemVer by CHANGE SCALE)

Twilight owns the SemVer feel. Bump by the **scale/significance of the change to the product**, not code
/diff size (diff size is only a hint):

- **PATCH** = an individual control / small change / fix (e.g. one General-tab control). PATCH is not
  capped at 9 — let it climb (0.8.47 is fine).
- **MINOR** = a whole **feature / screen / capability completed** — either one commit that completes it,
  or recognising that a stretch of patches cumulatively finished something significant (e.g. the whole
  DV/EV tab = a MINOR even though each control along the way was a PATCH).
- **MAJOR (→ 1.0.0)** = project staff (Twilight) only; never auto-bump.
- Docs/notes/test/CI-only commits don't move the number. Single source of truth is repo-root `VERSION`.
  See `reference/versioning.md`, `feedback`-history in `version.md`.

## Git, releases & GitHub

- **Commit early/often on `dev` and push each commit** (`git push origin dev`) — keep the remote current
  like the living notes (backup + source of truth across chat tabs). Push `feature/*` branches too.
- **Releases are MANUAL (2026-07-10).** Only cut a release (`--no-ff` merge `dev → main`) when Twilight
  says **"ship" / "ship it"**. Never auto-release even when green. `main` advances only by `--no-ff`
  tagged-release merges at a `VERSION` bump (PATCH direct; MINOR/MAJOR via `release/X.Y.0`). **CI owns
  the tag — never `git tag` by hand** (`release.yml` derives `v<VERSION>`; a manual tag makes the run
  skip). "Green" includes the remote `tests` CI, not just local `ctest`. See CLAUDE.md → Default
  Workflow step 4 and `reference/git-workflow.md`.
- **Deleting releases/tags (or any remote-history destruction) is never casual** — needs an explicit,
  specific per-instance go-ahead, and then do the minimum. Avoid the situation entirely (don't over-bump
  versions and create stray releases needing cleanup).
- **Hard git safety (always):** never force-push/`--force-with-lease`, rewrite pushed history,
  `reset --hard`, `rebase`, `clean -fd`, or delete a long-lived branch without an explicit request.
  Inspect `git status` before and after.
- **GitHub is part of normal management:** the `gh` CLI is installed + authed (`junebug12851`). When
  prepping to ship, check CI/issues/PRs and surface anything open; never auto-act on issues/PRs.
- **Verification is never skipped**, even on automated/express-authorized applies — that skips only the
  redundant confirmation pause, never the build/tests/compliance/constraint checks. If full verification
  can't complete, fall back to check-report-wait.

## Tooling & build/test (I CAN build/test/git here)

- **I am NOT sandbox-limited. Do not open a session claiming "no Qt tools available."** The
  `mcp__Windows-MCP__PowerShell` tool has real terminal access to Twilight's Windows machine with the full
  Qt 6.11 llvm-mingw toolchain — I can configure, build, run tests, launch the app, and git
  commit/push/merge directly, by default. (She is tired of re-explaining this.)
- **NEVER use the Cowork bash sandbox (`mcp__workspace__bash`)** — banned outright (stale mounts / false
  truncation; a `sed -i` sweep once truncated 55 files on the real disk). Use **PowerShell** +
  Read/Write/Edit for everything real (git, builds, tests, lock-file cleanup). Bash reads can lag file-tool
  writes and show phantom truncation / false NUL bytes — verify with the **Read** tool, not `cat`/`grep`/`wc`.
- **Toolchain paths:** compiler/runtime `C:\Qt\Tools\llvm-mingw1706_64\bin`; Qt
  `C:\Qt\6.11.0\llvm-mingw_64`; cmake `C:\Qt\Tools\CMake_64\bin`; Ninja. Every PowerShell call must
  prepend the llvm-mingw + Qt `bin` dirs to PATH and set `$env:CC=clang;$env:CXX=clang++` — **including
  when launching the app** (Qt `bin` must be on PATH or the DLLs don't load). Set crash-fast error mode
  (`SetErrorMode(0x0003)` P/Invoke) before running test/app exes. The PowerShell transport caps ~60s →
  run long builds **detached** and poll a log (redirect stdout+stderr).
- **Two build dirs:** the test loop uses repo-root `build/` (Ninja); the app runs from the kit dir
  `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/` (Makefiles; siblings `asan/`, `coverage/`).
  Rebuild the **kit dir** for in-app testing. **Kill the running app before a build that relinks a DLL**
  (it holds `db.dll`/`savefile.dll` open → link "Permission denied"). Editing a `savefile` `.cpp`
  rebuilds the DLL but not the exe — verify by DLL timestamp. Full loop + coverage: `project_pokered`
  test-loop notes and `reference/` / `plans/testing.md`.
- **git HEAD history caveat (recovery era):** an older HEAD (`2c2d6e5`) was the corrupted/truncated
  commit — when reconstructing/diagnosing, search full history (`git log -S/-p --all`, pre-corruption
  refs `424fd91`/`3a15b03`/`c44e068`) rather than trusting HEAD. (Largely historical now.)

## Reference clones (read-only, local-only)

- **`pret/pokered` disassembly** — the standing oracle for Gen 1 save-format / behaviour questions.
  In-repo at `assets/references/pokered` (inside the mount → reachable by Read/Grep/Glob); also a copy
  outside the mount at `C:\Users\juneh\Documents\projects\pokered` (needs PowerShell). Scan by label
  ranges, not just literal addresses (clears/copies hit regions by `label+size`). See
  `reference/gen1-knowledge.md`, `reference/box-recovery-research.md`.
- **pokered-save-editor v1** (Angular/Electron, TypeScript) at `assets/references/pokered-save-editor/` —
  a data-rich lookup reference for carrying features to v2 (screens, save-expansion models, game data).
  Consult it; do NOT batch-port it on my own. `assets/references/` is the official local-only reference
  home (gitignored contents, never a build input).

## Hard "never change" list (from debugging + owner feedback)

See also CLAUDE.md → "Critical Things Not to Get Wrong" and `decisions/rejected.md`:

- **No `load()` in DB constructors** (Qt 6 static-init deadlock; `DB::loadAll()` is the sole caller).
- **No `qt_add_qml_module()`** (conflicts with `app.qrc`, hangs `setSource()` — QRC + `qmlRegisterType()`
  in `bootQmlLinkage.cpp` is sufficient).
- **Don't remove the parameter from `SaveFile::dataExpandedChanged(SaveFileExpanded*)`** — the signal is
  correct.
- **Don't change `(dexInd+1)` arithmetic in `Pokedex.qml`** — `dexInd` is 0-indexed; `+1` is correct.
- **Don't make UI/UX decisions independently.**
- **Debug dialogs OK; release dialogs are not** — `#ifdef QT_DEBUG` guard any `QMessageBox` (Sims-2-style
  graceful degradation in release).

## Docs site (fairyfox theme)

The Doxygen docs site uses the fairyfox.io hub theme in `docs/fairyfox/`, deployed by `pages.yml` to
`fairyfox.io/pokered-save-editor-2/`. The hub chrome + "Aa" reader menu were adopted 2026-07-06. On any
web/docs work, **screenshot the actual render in Chrome and compare to the named reference** (e.g.
`random-ai-prompt`) before declaring done — an AI can spot layout problems visually that DOM probes miss.
See `project_pokered_docs_theme_wip` history and `reference/documentation.md`.
