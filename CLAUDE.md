# pokered-save-editor-2 — AI Context

Pokemon Red & Blue save file editor. Qt 6 C++/QML desktop app. Solo project by Twilight,
originally built 2017-2020, revived in 2026.

## Start Here

Read `notes/status.md` first — it has current build health, what's working, and what to do next.

The full notes system is in `notes/`. Everything is organized by topic:

| File | What's in it |
|------|-------------|
| `notes/status.md` | **Current state** — build/runtime health, open issues. Lean; no history |
| `notes/sessions/session-log.md` | Chronological "what changed each session and why" (newest first) |
| `notes/context/project.md` | What the project is and its goals |
| `notes/context/architecture.md` | Codebase structure, build system, key patterns |
| `notes/context/principles.md` | Owner's philosophy — what the app must/must not do |
| `notes/context/history.md` | How the project was revived — what was broken, what was fixed |
| `notes/reference/fix-patterns.md` | Compiler/runtime error → fix lookup table |
| `notes/reference/qt6-patterns.md` | Qt 5 → Qt 6 migration gotchas |
| `notes/reference/ui-patterns.md` | **UI/QML conventions** — layouts, borderless combos, ⋮ buttons, editor popups, sliders. Read before UI work |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems |
| `notes/decisions/architecture.md` | Key structural choices and why |
| `notes/decisions/rejected.md` | Things tried that failed — do not repeat |
| `notes/plans/next-steps.md` | Ordered task list |
| `notes/plans/future.md` | Longer-term ambitions |

## Critical Things Not to Get Wrong

- **Do NOT 