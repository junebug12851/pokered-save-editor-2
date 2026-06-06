# Project Notes — pokered-save-editor-2

Living documentation for the codebase. Written during development sessions. Useful for
anyone picking up the project — orient fast, avoid re-learning things the hard way.

---

## How to Find Things

| Folder / File | What's in it |
|---------------|-------------|
| **`status.md`** | **Start here.** Current state only — what builds, what works, open issues |
| `sessions/session-log.md` | Chronological history — what changed each session and why (newest first) |
| `context/project.md` | What the project is, its history, goals and philosophy |
| `context/architecture.md` | How the codebase is structured, build system, key patterns |
| `context/principles.md` | Project will — philosophy, constraints, what to avoid, design convictions |
| `context/history.md` | How the project was revived — phases, what was found, the truncation incident |
| `reference/fix-patterns.md` | Error → fix lookup table. Fast. No prose. |
| `reference/qt6-patterns.md` | Qt 5 → Qt 6 migration gotchas, with code examples |
| `reference/diagnostic-methods.md` | How to find and fix systemic problems (truncation, hangs, QML chain failures) |
| `decisions/architecture.md` | Key choices made and *why* — so they don't get undone accidentally |
| `decisions/rejected.md` | Things tried that failed — so they don't get tried again |
| `plans/next-steps.md` | Ordered list of what to do next |
| `plans/future.md` | Longer-term vision and ambitions |

---

## How to Write Here

Keep entries short and specific. Notes, not essays. If something needs a lot of explanation,
that probably means it belongs in `decisions/` not `reference/`.

### When to add something

| Situation | Where to write |
|-----------|---------------|
| Hit a compiler error and found the fix | `reference/fix-patterns.md` — add a row |
| Discovered a Qt 5 → Qt 6 difference | `reference/qt6-patterns.md` — add a section |
| Found a systemic debugging approach | `reference/diagnostic-methods.md` — add a section |
| Made a structural choice, want to explain why | `decisions/architecture.md` |
| Tried something, it failed, don't want to repeat | `decisions/rejected.md` |
| Something changed about build/runtime health or open issues | Update `status.md` (keep it lean — current state only) |
| Finished a work session (what changed + why) | Prepend an entry to `sessions/session-log.md` |
| New task to track | `plans/next-steps.md` or `plans/future.md` |

### Style

- **Direct.** These are notes, not documentation pages. Short is better.
- **Code blocks for code.** Show it, don't describe it.
- **Tables for lookups.** If it's error→fix or old→new, use a table.
- **Date** when timing matters (e.g., "Fixed 2026-06-04").
- **Bold the most important line** in a section so it's easy to spot.
- No cheerful intros or conclusions — just the content.

---

## Folder Structure

```
notes/
  README.md              ← This file
  status.md              ← Current project health + open issues (lean, no history)
  sessions/              ← Chronological history
    session-log.md       ← What changed each session and why (newest first)
  context/               ← Background that changes rarely
    project.md           ← What the project is, history, goals
    architecture.md      ← Codebase structure, build system, key patterns
    principles.md        ← Philosophy, constraints, what to avoid
    history.md           ← Project recovery story and major milestones
  reference/             ← Quick lookup, no story
    fix-patterns.md      ← Error → fix table
    qt6-patterns.md      ← Qt 5 → Qt 6 migration gotchas
    diagnostic-methods.md ← How to find and fix systemic problems
  decisions/             ← Rationale for choices made
    architecture.md      ← Key choices and why
    rejected.md          ← Things tried that failed
  plans/                 ← What comes next
    next-steps.md        ← Ordered immediate tasks
    future.md            ← Longer-term vision
```
