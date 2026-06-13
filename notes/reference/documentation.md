# Documentation Generation

API documentation for the **C++** is generated with **Doxygen**. Set up 2026-06-06.

## How to build the docs

From the repository root:

```
doxygen Doxyfile
```

Open `docs/html/index.html`. That's the whole workflow — one command, one config file.

Requirements on the machine running it:

- **Doxygen** 1.9.5 or newer (the theme's `HTML_COLORSTYLE = LIGHT` color handling needs
  ≥1.9.5; older versions just warn "unknown tag" and still produce docs). A current Qt
  toolchain machine will have a recent Doxygen.
- **Graphviz** (`dot`) on `PATH` — for the class / collaboration / include graphs
  (`HAVE_DOT = YES`). If Graphviz is absent, set `HAVE_DOT = NO` and docs still build,
  just without diagrams.

## What's covered / what's not

- **Covered:** all C++ under the four source roots
  (`projects/common/src`, `projects/db/src`, `projects/savefile/src`, `projects/app/src`) —
  279 files at setup time. `README.md` is the documentation landing page
  (`USE_MDFILE_AS_MAINPAGE`), so there's no separate hand-written intro file.
- **Also covered:** the `notes/` tree is in `INPUT`, so the living project notes
  (including the `notes/systems/` architecture deep-dive) build as cross-linked
  doc-site pages alongside the API reference. The **changelog** is in `INPUT` too
  — `version-notes.md` (the index) plus the `version-notes/` directory (one page per month) —
  rendering as a "Version History" index and a page per month under Related Pages;
  the hidden `<!-- commit: … -->` markers are HTML comments and stay invisible in
  the output.
- **Not covered:** QML (`projects/app/ui/**`). This is **intentional** — see
  `decisions/architecture.md` → "Documentation: Doxygen for C++, no generator for QML".
  QML is documented with plain human-readable comments, not a doc generator.

## Comment style

Doxygen reads the declaration on the next line, so comments stay human and Markdown-friendly.
`QT_AUTOBRIEF`/`JAVADOC_AUTOBRIEF` are on, so the **first sentence is the brief** — no `\brief`
command needed. Examples:

```cpp
/// Loads the save file at `path`.
///
/// Returns `true` on success, or `false` if the file is unreadable or fails
/// validation. Only the bytes that change are written.
bool load(const QString &path);
```

```cpp
/**
 * Expands a packed save into its in-memory structure.
 *
 * The raw bytes are left untouched; this builds a parallel view.
 * @param raw  the on-disk save buffer
 */
void expand(const QByteArray &raw);
```

Use backticks for inline code, normal Markdown lists/headings in longer blocks, `@param` /
`@return` only when you want the structured table. No special command soup required.

## Files (the entire doc footprint)

- `Doxyfile` — the one config file (root). Curated, not a full default dump; unlisted tags take
  defaults. Upgrade across Doxygen versions with `doxygen -u Doxyfile`.
- `docs/doxygen-awesome/doxygen-awesome.css` — vendored theme (MIT). See that folder's README.
- `docs/html/` — generated output. **git-ignored**, never committed.

The Doxyfile predefines Qt's macros (`Q_OBJECT`, `Q_PROPERTY`, `Q_INVOKABLE`, `signals`, `slots`,
etc.) under `PREDEFINED` so Doxygen's preprocessor doesn't mis-parse them — the standard Qt+Doxygen
gotcha. If a new Qt macro ever confuses the parser, add it there.

## Why Doxygen and not qdoc

Considered qdoc (Qt's own tool, the only first-party way to document QML). Rejected: its comment
syntax requires restating every signature with topic commands (`\fn`, `\class`, `\qmltype`),
which makes in-code comments verbose and technical — a poor reading experience for whoever is in
the source. The project's UX-first bar applies to the code itself, not just the app. Doxygen keeps
comments human; the cost is no generated QML doc site, judged not worth qdoc's tax for a solo
project. Full reasoning in `decisions/architecture.md`.
