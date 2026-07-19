---
date: 2026-07-19
procedure: adopting-updates
node: pokered-save-editor-2
outcome: completed
hub_version: 0.16.1
hub_commit: 10b484f
chrome_version: 2.0.0
---

# Process Report — adopting-updates, 2026-07-19

> A full, honest account of running a fairyfox system procedure. The point is to improve the system —
> so say what was rough even if the run succeeded. Voice: direct, matter-of-fact, no hype.

## Outcome in one line

Adopted the hub's **vendored shared-chrome bundle** (chrome VERSION 2.0.0) into this project's Doxygen
docs site + the screenshots gallery — replacing the 2026-07-06 hand-reimplementation — with one
documented deviation (self-hosted fonts). Completed and verified in-browser across prose/API pages and
light/dark themes.

## What was done

1. Refreshed the read-only hub clone at `assets/references/fairyfox.io/`. The hub's `dev` had been
   force-pushed (routine), so the `--ff-only` pull aborted and the `reset --hard origin/dev` fallback
   ran on the git-ignored mirror — landing at `10b484f`, site version **0.16.1** (was 0.14.4 at the
   last adoption), chrome bundle **VERSION 2.0.0**.
2. Read the express-authorization ledger (`hub/authorizations.yml`). The standing
   `adopt-standards-by-default` entry (2026-07-02) covers `hub/standards/` + `hub/templates/`, so the
   docs-site adoption is pre-authorized: applied without the check-report-wait pause. Every other
   safety step still ran (copy-not-clobber, divergence re-prompt, verification, this report).
3. Diffed the hub `docs-site/` standard against what the project runs. The model **changed**: the
   chrome is no longer a spec you reimplement — since 0.15.0 it is a **vendored bundle** you copy
   verbatim (`12-shared-chrome.md` + `chrome/`), and the hub ships a **Doxygen adapter** that names
   this project as its reference case. Also folded in: the owner rename `junebug12851 → 1fairyfox`
   (this repo's own remote is already `github.com/1fairyfox/...`), and the reader's Farms dropdown +
   story-only line-spacing/width controls.
4. **Surfaced the one real conflict and asked** (a deliberate local divergence must not be clobbered
   even under pre-authorization): the bundle's `head.html` loads fonts from Google Fonts, but this
   project deliberately self-hosts them (2026-07-06, per the hub legal-docs standard). Twilight chose
   **keep self-hosted** + file a recommendation to the hub (below).
5. Vendored `main.css` + `reader.js` + `nav.js` from the master into `docs/fairyfox/`; pinned
   `docs/fairyfox/CHROME_VERSION = 2.0.0`. Rebuilt `header.html`/`footer.html` as Doxygen templates
   wrapping the bundle markup (Projects active, `1fairyfox` links, self-hosted fonts). Retired
   `ff-docs.js` and shrank `fairyfox-doxygen.css` from ~1000 lines to a thin **bridge**
   (doxygen-awesome-var → main.css-token mapping + generated-reference layout). Wired the Doxyfile.
   Migrated `scripts/build_gallery.py` to the same bundle.
6. Verified in Chrome (rendered, JS on): Overview/Notes = full-width prose; API pages = treeview
   pinned below the sticky chrome; reader panel (theme/accent/size + story-only controls disabled);
   Farms dropdown; footer; light + dark themes. Fixed three integration issues found by the review
   (below).

## What was rough / integration issues found and fixed

- **doxygen-awesome's `summary::before { content:"▼" }`** (its collapsible-section marker) leaked onto
  the chrome's Farms `<summary>`, producing a stray triangle. Bridge suppresses it on `.nav`/`.subnav`.
- **App-shell vs document scroll.** doxygen-awesome wants a fixed-height, internally-scrolling
  `#doc-content` + fixed sidebar; the chrome wants one sticky-top document scroll. The thin bridge
  initially under-ported the prior adoption's reconciliation, so (a) the treeview overlapped the title
  area and (b) once pinned it collapsed to 8px (doxygen's JS resize didn't stick under `position:fixed`).
  Fixed by porting the single-page-scroll rules, a deterministic sidebar width + matching content
  margin, and a live `--ff-header-h` measurement in the footer glue.
- **Browser cache** made the gallery look unstyled after a hot CSS swap — a false alarm, not a real
  defect (a cache-busted reload rendered the grid correctly). Worth noting because it can mislead a
  reviewer mid-iteration.

None of these are hub problems — they're the inherent "wear the chrome, let the reference body be
reference" boundary the Doxygen adapter already calls out. The adapter was accurate and sufficient.

## Recommendation back to the hub (requested by Twilight)

**Move the shared-chrome bundle to self-hosted fonts.** The bundle's `chrome/head.html` currently
pulls Fraunces / Inter / JetBrains Mono from `fonts.googleapis.com`. Every node that also honors the
hub's own **legal-docs** "no third-party IP/PII exposure" posture then has to deviate on adoption (this
node does, and documents it) — which is exactly the per-project drift the bundle exists to remove. If
the bundle instead shipped the woff2 files + a `fonts.css` `@font-face` sheet (as this project already
does), self-hosting would be the mesh default, no node would need a font deviation, and the "copy
verbatim" contract would hold for the whole `head`. Suggested shape: add `chrome/fonts/*.woff2` +
`chrome/fonts.css` to the bundle, point `head.html` at `{{FF_FONTS_HREF}}`, bump `chrome/VERSION`
MAJOR (a copy nodes already vendored changes by hand). This node's `docs/fairyfox/fonts/` can serve as
the reference implementation.

## Verification

- `## Verify` (shared-chrome): shipped chrome is the vendored bundle markup + master `main.css`/
  `reader.js`/`nav.js` (not a re-authored variant); assets are vendored, not hot-linked (renders with
  the main site unreachable); only the `{{FF_*}}` slots + `.active` + the **documented font deviation**
  differ from the bundle; the reader "Aa" button injects and the pre-paint theme script is in `<head>`;
  adopted `chrome/VERSION` recorded.
- Doxygen build clean (no chrome-related warnings; the warnings present are pre-existing content refs).
  `build_gallery.py` passes an `ast.parse` check and builds 29 shots. Git safety: staged only the
  chrome-adoption files; the concurrent map-states WIP in the working tree was left untouched.

## Would improve the procedure

- The Doxygen adapter could add a one-paragraph "layout reconciliation" note (single-page-scroll +
  pinned-sidebar + the deterministic width) — it's the part every Doxygen node will hit, and it's
  currently learned by rediscovery rather than documented.
- Self-hosting the fonts in the bundle (above) would remove the single most common documented deviation.
