# Project Notes — pokered-save-editor-2 {#pse_notes_system}

Living documentation for the codebase and project. Written during development sessions so that anyone
picking it up — a human or an AI opening the repo cold — can orient fast and avoid re-learning things
the hard way. The goal: **no knowledge trapped in one person's head, and nothing lost between
sessions.**

This file describes the **system** — where everything lives and how it's kept current. Read
[`status.md`](status.md) first for the actual current state.

---

## How to find things

| Folder / File | What's in it |
|---------------|-------------|
| **[`status.md`](status.md)** | **Start here.** Current state only — build/runtime health, open issues, what's next. No history. |
| [`sessions/`](sessions/README.md) | **The history.** One file per day, grouped into month folders (`YYYY-MM/YYYY-MM-DD.md`): what changed each session and why. `sessions/README.md` explains the system; `revival-s13-series.md` is the undated pre-corruption revival narrative. |
| [`version.md`](version.md) | **The changelog** — plain-English, one entry per commit, newest first (index; months under `version/`). |
| `context/` | Background that changes rarely: [`project.md`](context/project.md) (what it is + goals), [`architecture.md`](context/architecture.md) (codebase structure + build), [`principles.md`](context/principles.md) (philosophy, what to avoid), [`origins.md`](context/origins.md) (the 2019–2020 story), [`history.md`](context/history.md) (the 2026 revival). |
| `systems/` | **System map** — [`overview.md`](systems/overview.md) (the machine: layers, boot, data flow, byte-fidelity) + per-layer deep-dives (`common`/`savefile`/`db`/`app`/`qml`). |
| `reference/` | Quick lookup, no story: [`fix-patterns.md`](reference/fix-patterns.md) (error→fix table), [`qt-patterns.md`](reference/qt-patterns.md) (the Qt/QML catalog + Qt 5→6 patterns + case studies), [`ui-patterns.md`](reference/ui-patterns.md) (UI/QML conventions — read before UI work), [`gen1-knowledge.md`](reference/gen1-knowledge.md) (Gen 1 save-format domain knowledge) + [`box-recovery-research.md`](reference/box-recovery-research.md), [`diagnostic-methods.md`](reference/diagnostic-methods.md), [`i18n.md`](reference/i18n.md), [`documentation.md`](reference/documentation.md) (Doxygen + comment style + progress), [`git-workflow.md`](reference/git-workflow.md), [`versioning.md`](reference/versioning.md) (the version-*number* scheme). |
| `decisions/` | Rationale: [`architecture.md`](decisions/architecture.md) (choices + why), [`rejected.md`](decisions/rejected.md) (things tried that failed — don't repeat). |
| `plans/` | What's next: [`next-steps.md`](plans/next-steps.md) (ordered tasks), [`testing.md`](plans/testing.md) (the test suite + gaps), [`future.md`](plans/future.md) (longer-term vision). |

> **`version.md` vs `versioning.md`** — easy to confuse. `version.md` (+ `version/`) is the
> **changelog** (the narrative of what changed). `reference/versioning.md` is the **version-number
> scheme** (SemVer, the `VERSION` file). One is the story; the other is the label.

---

## How the system is kept current (the maintenance loop)

The notes are a **living document** — updated as work happens, by default, not on request. Each piece
has one home and one trigger:

| When this happens | Write it here |
|-------------------|---------------|
| You did work worth recording this session | Append to today's `sessions/YYYY-MM-DD.md` (newest on top; create the file if it's the day's first entry) |
| You made a substantive commit | Its plain-English changelog entry rides **inside that commit**, at the top of `version/YYYY-MM.md` (see [`version.md`](version.md) → the inline rule) |
| Build/runtime health or open issues changed | Update [`status.md`](status.md) (keep it current-state only) |
| Fixed a compiler/runtime error | Add a row to [`reference/fix-patterns.md`](reference/fix-patterns.md) |
| Hit any Qt/QML landmine | Add to [`reference/qt-patterns.md`](reference/qt-patterns.md) |
| Made / rejected a structural decision | [`decisions/architecture.md`](decisions/architecture.md) / [`decisions/rejected.md`](decisions/rejected.md) |
| Learned something about the save format or game | [`reference/gen1-knowledge.md`](reference/gen1-knowledge.md) |
| A UI convention emerged | [`reference/ui-patterns.md`](reference/ui-patterns.md) |
| A new contributor/tool/AI helped | `projects/db/assets/data/credits.json` (the in-app Credits screen is living too) |

**The division of labor:** the **session log** records *that something happened, on a day*; the
**reference files** record the *reusable lesson*; **status.md** records *where things stand now*; the
**changelog** records *per commit*. When a fix is worth reusing, it goes in two places — the session
log (the event) and the right reference file (the lesson). Don't duplicate prose; cross-link.

The structure is meant to **grow**. If something doesn't fit an existing file, make a new one in the
right folder rather than stuffing it somewhere wrong. (The fuller, AI-facing version of this loop is in
`../CLAUDE.md` → "Maintaining the Notes".)

---

## How to write here

- **Direct.** These are notes, not documentation pages. Short is better. No cheerful intros/outros.
- **Code blocks for code.** Show it, don't describe it.
- **Tables for lookups.** error→fix, old→new.
- **Date** when timing matters (`2026-06-14`); session files are named by date.
- **Bold the most important line** in a section so it's easy to spot.
- **Voice:** plain and matter-of-fact, written from the project's perspective. Most of the time no name is needed; where attribution genuinely matters, use plain names (no different from any other contributor) and neutral phrasing — "agreed"/"decided", not "so-and-so signed off" or "X-directed".
- **Cross-link** related files with relative links; don't restate another file's content.

---

## Folder structure

```
notes/
  README.md              ← this file (the system)
  status.md              ← current state only (health + open issues, no history)
  version.md             ← changelog index (plain-English, per commit)
  version/               ← changelog, one file per month (YYYY-MM.md)
  sessions/              ← the history, one file per day in month folders
    README.md            ← how the per-day log system works
    YYYY-MM/             ← month folder (e.g. 2026-06/)
      YYYY-MM-DD.md      ← what changed that day and why (newest on top)
    revival-s13-series.md← undated pre-corruption 2026 revival narrative (legacy)
  context/               ← background that changes rarely
    project.md  architecture.md  principles.md  origins.md  history.md
  systems/               ← the system map (macro + per-layer)
    overview.md  common.md  savefile.md  db.md  app.md  qml.md  README.md
  reference/             ← quick lookup, no story
    fix-patterns.md  qt-patterns.md  ui-patterns.md  gen1-knowledge.md
    box-recovery-research.md  diagnostic-methods.md  i18n.md
    documentation.md  git-workflow.md  versioning.md
  decisions/             ← rationale for choices
    architecture.md  rejected.md
  plans/                 ← what comes next
    next-steps.md  testing.md  future.md
```
