# Documentation (generation, comment style, progress)

Everything about the project's code documentation: how the doc site is generated, the house comment
style every pass follows, and the status of the project-wide doc-comment effort.

---

## 1. Generating the docs

API documentation for the **C++** is generated with **Doxygen** (set up 2026-06-06).

From the repository root:

```
doxygen Doxyfile
```

Open `docs/html/index.html`. That's the whole workflow — one command, one config file.

Requirements on the machine running it:

- **Doxygen** 1.9.5 or newer (the theme's `HTML_COLORSTYLE = LIGHT` handling needs ≥1.9.5; older
  versions just warn "unknown tag" and still produce docs).
- **Graphviz** (`dot`) on `PATH` — for the class / collaboration / include graphs (`HAVE_DOT = YES`).
  If absent, set `HAVE_DOT = NO` and docs still build, just without diagrams.

### What's covered / what's not

- **Covered:** all C++ under the four source roots (`projects/common/src`, `projects/db/src`,
  `projects/savefile/src`, `projects/app/src`) — 279 files at setup time. `README.md` is the
  documentation landing page (`USE_MDFILE_AS_MAINPAGE`), so there's no separate intro file.
- **Also covered:** the `notes/` tree is in `INPUT`, so the living project notes (including the
  `notes/systems/` architecture deep-dive) build as cross-linked doc-site pages. The **changelog** is
  in `INPUT` too — `version.md` (the index) plus the `version/` directory (one page per month),
  rendering under Related Pages; the hidden `<!-- commit: … -->` markers are HTML comments and stay
  invisible in the output.
- **Not covered:** QML (`projects/app/ui/**`). **Intentional** — see `../decisions/architecture.md` →
  "Documentation: Doxygen for C++, no generator for QML". QML is documented with plain human-readable
  comments, not a doc generator.

### Files (the entire doc footprint)

- `Doxyfile` — the one config file (root). Curated, not a full default dump; unlisted tags take
  defaults. Upgrade across Doxygen versions with `doxygen -u Doxyfile`.
- `docs/doxygen-awesome/doxygen-awesome.css` — vendored theme (MIT). See that folder's README.
- `docs/fairyfox/` — the **fairyfox.io hub theme** layered on doxygen-awesome (the docs-site is part
  of the fairyfox mesh; see [`cross-project-sync.md`](cross-project-sync.md)). As of **2026-07-19** the
  chrome is the hub's **vendored shared-chrome bundle** (chrome **VERSION 2.0.0**), not a
  hand-reimplementation — the hub's own standard now says chrome adoption *is a file copy*
  (`12-shared-chrome.md`), wired for Doxygen via its `adapters/doxygen.md`. Process report:
  `../fairyfox-reports/2026-07-19-adopting-updates.md`; decision + deviation:
  `../decisions/architecture.md`. Pieces:
    - `main.css`, `reader.js`, `nav.js` — the **master assets, vendored verbatim** from the hub
      (`assets/css/main.css` + `assets/js/{reader,nav}.js`). `main.css` is the palette + every chrome
      component (header/nav/subnav/reader/footer); `reader.js` injects the "Aa" reading menu (theme ·
      accent · size · story-only line-spacing/width, key `fairyfox:reader:b`); `nav.js` drives the
      hamburger + the Farms dropdown. Never edited — pulled and re-diffed on refresh.
    - `CHROME_VERSION` — the adopted bundle version (`2.0.0`), pinned so a later refresh is a clean diff.
    - `fairyfox-doxygen.css` — the thin **bridge** (all that's left of the old ~1000-line chrome CSS).
      Two jobs: (1) map doxygen-awesome's variables onto `main.css`'s live theme tokens (scoped to
      `html.dark-mode, html.light-mode, html:not(.dark-mode):not(.light-mode)` so it wins in every
      state) so the generated API body tracks the reader theme; (2) the **generated-reference boundary**
      — hide `#titlearea` + `#main-menu` (redundant with the chrome; this also hides Doxygen's built-in
      search box), single-page scroll (`#doc-content` flows, no inner scrollbar), the treeview pinned
      `position:fixed` below the sticky chrome **on API pages only** at a deterministic width, prose
      pages full-width. Also carries the screenshots-gallery styles + the skip link.
    - `header.html`/`footer.html` — Doxygen templates wrapping the **bundle** `head`/`header`/`subnav`/
      `footer` markup: the fixed mesh-wide primary nav (Home · Projects · Farms [Stories · Games] ·
      Docs · Updates · About — **Projects** active), the project subnav (Overview · Notes · API
      Reference · Screenshots · Repository → `1fairyfox`), the bundle footer, the pre-paint theme
      script, and the `theme-color` metas. Brand = the **hub fox logo** linking home (no separate
      "Back" button). Small **project glue** kept separate from the vendored bundle: a footer `<script>`
      mirrors the reader `data-theme` onto doxygen-awesome's `light-mode`/`dark-mode` class, measures
      the chrome height into `--ff-header-h`, and marks the active subnav pill; a pre-paint header
      `<script>` adds `html.ff-no-sidebar` on prose pages.
    - `fonts/` — self-hosted Fraunces / Inter / JetBrains Mono woff2 + `fonts.css`. **DOCUMENTED
      DEVIATION** from the bundle (whose `head.html` uses Google Fonts): same families/weights, no
      third-party IP exposure, per the hub legal-docs standard (a recommendation to self-host in the
      bundle itself was filed back to the hub). `pse-logo.png` is the favicon/project logo. `ff-docs.js`
      (the old hand-built reader) was **retired** in the 2.0.0 adoption.
  All wired in the Doxyfile (`HTML_HEADER`/`HTML_FOOTER`; `HTML_EXTRA_STYLESHEET` = doxygen-awesome +
  `main.css` + the bridge; `reader.js`/`nav.js`/fonts in `HTML_EXTRA_FILES`; `HTML_COLORSTYLE = LIGHT`
  so Doxygen stops running its own competing dark script). `scripts/build_gallery.py` wears the same
  bundle. The hub standard + adapter live in the read-only clone at
  `assets/references/fairyfox.io/hub/standards/docs-site/`.
- `docs/html/` — generated output. **git-ignored**, never committed.

The Doxyfile predefines Qt's macros (`Q_OBJECT`, `Q_PROPERTY`, `Q_INVOKABLE`, `signals`, `slots`,
etc.) under `PREDEFINED` so Doxygen's preprocessor doesn't mis-parse them — the standard Qt+Doxygen
gotcha. If a new Qt macro ever confuses the parser, add it there.

### Why Doxygen and not qdoc

Considered qdoc (Qt's own tool, the only first-party way to document QML). Rejected: its comment syntax
requires restating every signature with topic commands (`\fn`, `\class`, `\qmltype`), which makes
in-code comments verbose and technical — a poor reading experience for whoever is in the source.
the project UX-first bar applies to the code itself, not just the app. Doxygen keeps comments human; the
cost is no generated QML doc site, judged not worth qdoc's tax for a solo project. Full reasoning in
`../decisions/architecture.md`.

### Markdown pages, links & navigation

Doxygen is a C++ API tool, so a few settings make it render the project's GitHub-flavored Markdown
correctly:

- **Every Markdown page is placed in an explicit tree** by `notes/_nav.dox`, which builds two
  top-level "Related Pages" hubs: **Project Notes** (all of `notes/`) and **Project & Repository**
  (`credits.md`, `VERSION`, `LICENSE`). **Hard rule:** any Markdown file added to the Doxyfile `INPUT`
  must get a `\subpage` line in `_nav.dox` — never let Doxygen auto-append a page flat at the top of
  Related Pages. The index READMEs and `structure.md` carry explicit `{#labels}`; everything else uses
  Doxygen's path-derived IDs.
- **`AUTOLINK_SUPPORT = NO`** — otherwise Doxygen turns every bare filename in the notes (even inside
  `backticks`) into a link to a near-empty "File Reference" stub. With it off, only real
  `[text](file.md)` links and `\ref` resolve, pointing at the proper content pages. (Cost: prose
  mentions of C++ names don't auto-link; use `\ref`/`\see`. Structured nav is unaffected.)
- **`MARKDOWN_ID_STYLE = GITHUB`** — so heading anchors match the `#slug` style the README's
  Table of Contents uses. The TOC itself uses **raw-HTML anchors** (`<a href="#slug">…</a>`) because
  Doxygen 1.17 doubles the text of Markdown `[text](#anchor)` links.
- **`LICENSE`, `VERSION`** (extensionless) are in `INPUT` + `FILE_PATTERNS` with
  `EXTENSION_MAPPING = no_extension=md`, so the README's `[LICENSE](LICENSE)` / `[VERSION](VERSION)`
  links resolve. `credits.md` is the human-readable rendering of `credits.json` (regenerate on every
  JSON edit — see `CLAUDE.md` → "Keep the Credits Screen Living").
- **Blockquote gotcha: keep blockquotes single-line, and use `*italics*` not `_italics_`.** A
  **multi-line** `> …` blockquote (and/or `_..._` emphasis next to a link/code span) makes Doxygen
  mis-parse it — it doesn't render the emphasis AND leaks a literal `</blockquote>` that a following
  `---` turns into a stray `<h1>` (polluting the page and the auto-TOC/nav). Seen on the README intro
  when it became the Pages docs home (2026-06-15). GitHub renders both forms fine, so prefer the
  single-line `*…*` form that satisfies both renderers.
- **Doxygen VERSION matters — CI installs 1.17 from the official binary, not apt's 1.9.1.** The
  blockquote bug above is present in ubuntu's old Doxygen 1.9.1 but fixed by ~1.17 (the local toolchain).
  A local `doxygen` test can therefore pass while CI (if it used apt's 1.9.1) stays broken. So
  `pages.yml` and `release.yml` download Doxygen 1.17.0 to `/usr/local/bin` — from the **Doxygen GitHub
  release** (`github.com/doxygen/doxygen/releases/.../doxygen-<v>.linux.bin.tar.gz`), since
  `doxygen.nl/files/` returns **403** to datacenter/curl requests (a browser-UA `doxygen.nl` fetch is the
  fallback). Keeps CI output identical to local; bump the pinned version when the local toolchain moves.

---

## 2. Comment style (house rules)

The conventions every documentation pass follows, so the comments read as one consistent voice.

### Where the comment goes

- **API docs live in the header** (`.h`), on the declaration. Doxygen associates the comment with the
  next declaration, so the comment stays human and never restates the signature.
- **Implementation files** (`.cpp`) get a short `@file` line at the top and then ordinary `//` notes
  inside bodies explaining *why* a step is done. They do NOT repeat the header's `/** */` blocks.
- Every file opens (after the licence header) with a one-line `@file` + `@brief`.

### Syntax

- Multi-line API doc: `/** ... */`. One-liner: `///`. Trailing member doc: `///<`.
- `QT_AUTOBRIEF`/`JAVADOC_AUTOBRIEF` are on, so **the first sentence is the brief** — no explicit
  `@brief` needed (though it's fine to use for emphasis).
- Common tags: `@param`, `@return`, `@note`, `@see`, `@ref`, `@c code`, `@e emphasis`.

Examples:

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

### Voice

- Describe **purpose and role**, not the obvious. "Process-wide source of randomness" beats "the Random
  class".
- Call out the non-obvious: ownership, singletons, Qt 6 gotchas, byte-fidelity implications, why a
  guard exists. Link related types with `@see`.
- Depth setting for this project (a design decision): **document everything**, including trivial
  getters/setters and private members — but keep each line genuinely informative rather than padding.

### Preserving existing human-written comments (hard rule)

- **Never delete an existing human-written comment.** If it overlaps a new Doxygen block, *merge*
  its content into the block (keep the meaning/wording); otherwise leave it exactly where it is,
  untouched.
- Adding new explanatory `//` notes on the code is **encouraged** — as the architecture is mapped out,
  record the non-obvious "why" inline next to the code, not just in the headers.

### Hard rules

- **ASCII only** in comments. No em/en dashes, arrows, or `>=` glyphs — use `--`, `->`, `>=`. The
  codebase is ASCII; non-ASCII also tripped a write/encoding issue once.
- **Comments only.** A documentation pass must never change a single line of code. Verify this every
  time (below).
- Preserve the existing licence header verbatim at the top of every file.

### Verifying a pass is comments-only

Strip comments from the old and new versions and diff the remaining code:

```
strip() { gcc -fpreprocessed -dD -E "$1" | grep -v '^#' \
          | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//' | grep -v '^$'; }
git show HEAD:"$f" > /tmp/orig; strip /tmp/orig > /tmp/a; strip "$f" > /tmp/b
diff /tmp/a /tmp/b   # empty == comments-only
```

**Caveat:** the Cowork bash sandbox sometimes serves a *stale, falsely-truncated* view of a file right
after it's written, making the diff report bogus "code removed" lines. When that happens, trust the
**Read tool** (true on-disk state), not bash. (This is also why bulk/scripted edits go through the
Read/Edit/Write tools or Windows-side PowerShell, never shell redirection over the mount.)

---

## 3. Progress ledger

The project-wide doc-comment pass (started 2026-06-06) is **complete** — every C++ layer documented to
the house style and verified comments-only; every QML file carries a header block + inline notes
(insertion-only, no code changed). ~374 source files, ~43k lines.

| Layer | Path | .h | .cpp | qml | Status |
|-------|------|----|----|-----|--------|
| common   | `projects/common/src/pse-common`     | 4  | 2  | --  | **DONE + verified** (the style reference) |
| savefile | `projects/savefile/src/pse-savefile` | 50 | 48 | --  | **DONE + verified** comments-only |
| db       | `projects/db/src/pse-db`             | 57 | 56 | --  | **DONE + verified** comments-only |
| app      | `projects/app/src`                   | 29 | 33 | --  | **DONE + verified** comments-only |
| qml      | `projects/app/ui`                    | -- | -- | 95  | **DONE** — header block + inline notes, insertion-only |

Notes worth keeping:

- **common is the style reference** — new/future code should match its voice and depth.
- The shared "expanded node" convention (load/save/reset/randomize) is documented once on
  `SaveFileExpanded` and referenced by the rest; the DB-singleton + DB-entry patterns are documented
  once on `creditsdb.h` / `entries/creditdbentry.h` and referenced by the rest.
- `MainWindow` (`app/ui/window`) is C++ but **outside** the Doxyfile `INPUT` — flagged in
  `../systems/app.md`.
- QML recurring invariants flagged in-place where someone would edit them: the `(dexInd+1)` /
  `(dexNum+1)` / `(itemDex+1)` 0→1 dex conversions (do NOT simplify), the commit-on-finish (atomic)
  name/ID writes, and the shiny `onToggled` guard. Existing human-written comments were preserved (filename
  prepended, never replaced).

The architecture write-ups live in [`../systems/`](../systems/README.md).

### Per-module checklist (for any new module going forward)

- [ ] Read the module's headers + cpp; understand its role and wiring.
- [ ] Capture the understanding in the system-map notes (`../systems/`).
- [ ] Add `@file` + class + member docs (headers) and `//` impl notes (cpp).
- [ ] ASCII-only; comments only.
- [ ] Verify comments-only (strip-diff; trust the Read tool if bash is stale).
- [ ] Note it here with the date.
