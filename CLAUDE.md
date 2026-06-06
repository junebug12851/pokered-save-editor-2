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
| `notes/context/history.md` | How the project was revived — what was broken, what was fixed |
| `notes/reference/fix-patterns.md` | Compiler/runtime error → fix lookup table |
| `notes/reference/qt6-patterns.md` | Qt 5 → Qt 6 migration gotchas |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems |
| `notes/reference/ui-patterns.md` | **UI/QML conventions** — layouts, borderless combos, ⋮ buttons, editor popups, sliders. Read before UI work |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems |
| `notes/decisions/architecture.md` | Key structural choices and why |
| `notes/decisions/rejected.md` | Things tried that failed — do not repeat |
| `notes/plans/next-steps.md` | Ordered task list |
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

Also: if something comes up that doesn't fit any existing file, create a new file in the right folder.
The structure is meant to grow. Don't stuff things into the wrong place to avoid creating a new file.

The goal is that any AI opening this project cold can read the notes and be fully oriented —
with no information trapped in a human's head or lost between sessions.

## Owner Preferences

- Twilight makes all UI/UX decisions — do not independently change QML appearance
- Debug builds show error dialogs; release builds degrade silently (Sims 2 philosophy)
- The app should feel like polished software, not a dev tool

