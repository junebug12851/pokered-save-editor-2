---
date: 2026-07-06
procedure: adopting-updates
node: pokered-save-editor-2
outcome: completed
hub_version: 0.14.4
hub_commit: 9efb6ad
---

# Process Report — adopting-updates, 2026-07-06

> A full, honest account of running a fairyfox system procedure. The point is to improve the system —
> so say what was rough even if the run succeeded. Voice: direct, matter-of-fact, no hype.

## Outcome in one line

Adopted the hub's updated docs-site **chrome + reader** standard into this project's Doxygen theme —
mirroring the sibling `random-ai-prompt`'s implementation, reimplemented for Doxygen — plus the
self-hosted fonts and the API-only sidebar. Completed and verified in-browser across all three themes.

## What was done

1. Refreshed the read-only hub clone at `assets/references/fairyfox.io/`. Clean fast-forward
   (`git pull --ff-only origin dev`, 0 ahead / 45 behind) — no force-push this time — landing at
   `9efb6ad`, site version `0.14.4` (was `0.11.2`).
2. Read the express-authorization ledger (`hub/authorizations.yml`). The standing
   `adopt-standards-by-default` entry (2026-07-02) covers `hub/standards/`, so the docs-site adoption
   is pre-authorized: applied directly, skipping only the check-report-wait pause. Every other safety
   step still ran.
3. Studied the source of truth — `hub/standards/docs-site/` (`reference/chrome.html`, `02-design-tokens`,
   `05-navigation`, `11-measurements`) — and the sibling `random-ai-prompt`'s live implementation
   (`assets/docs-theme/`: `modules/chrome.js`, `modules/reader.js`, `theme/{tokens,chrome,reader}.css`,
   `fonts/`), which the owner pointed at as the reference to match closely.
4. Reimplemented the chrome + reader in this project's stack. The sibling injects everything client-side
   because docdash has no header template; Doxygen DOES, so the clean reimplementation is static chrome
   in `header.html`/`footer.html` + the reader (`ff-docs.js`) + adopted tokens/CSS — the standard's
   intended "faithful hand-reimplementation", not a file copy. Mapped the sibling's `--*` tokens onto
   this project's `--ff-*` names; kept the registry pink accent across all three themes.
5. Handled three owner refinements mid-verify (each a design call, applied directly): use the **hub fox
   logo** not the project's; **Projects** active in the primary nav, not Docs; sidebar on **API pages
   only** like the sibling.
6. Fixed two real bugs surfaced by the in-browser verify (see friction). Updated the docs pipeline note,
   changelog (`version/2026-07.md`, new month), session log, and this report; registered the new pages
   in `_nav.dox`. No `VERSION` bump (docs-site only).

## What went well

- The ledger made the authorization unambiguous — a standards adoption is squarely covered by the
  standing grant, so no redundant prompt.
- The sibling's implementation plus the hub `reference/chrome.html` gave an exact target; the chrome
  and reader modules are largely framework-agnostic, so porting the markup + behaviour was clean.
- The project already carried the full `--ff-*` token family and the "generator IS the site" (Case B)
  mapping, so the reader's live theming/accent worked with only additive token blocks (data-theme
  light + sepia) — no restructuring.

## What went wrong / friction

- **doxygen-awesome's own theme system fought the reader.** doxygen-awesome sets its palette under
  `html:not(.light-mode)` (a `@media prefers-color-scheme: dark` rule, specificity 0,1,1) and
  `html.dark-mode`, both of which beat a plain `html {}` var re-mapping. On a dark-OS machine, the
  reader's "Light" left `--page-foreground-color` at doxygen-awesome's dark value and washed the text
  out. Fix: scope the mapping to `html.dark-mode, html.light-mode, html:not(.dark-mode):not(.light-mode)`
  (>= their specificity, loaded later → wins), have the reader drive the `light-mode`/`dark-mode`
  class, and set `HTML_COLORSTYLE = LIGHT` so Doxygen stops injecting its own competing dark script.
  This is a Doxygen-specific integration point the docs-site standard (written around docdash/JSDoc,
  which has no such vendor theme) doesn't mention — worth a note for other Doxygen nodes.
- **Doxygen's treeview resizer corrupts across the API-only sidebar boundary.** Doxygen persists the
  sidebar width globally in `localStorage["doxygen_width"]`; hiding `#side-nav` on a prose page made the
  resizer record a collapsed width (`2`), which then shrank the sidebar on the API pages too. Fix:
  freeze the `doxygen_width` key on prose pages (guard `localStorage.setItem`, applied before Doxygen's
  resize script runs). Again Doxygen-specific — the standard's sidebar-on-API-pages guidance assumes a
  stateless sidebar.
- **`file://` is blocked by the browser extension**, so verifying the built site meant standing up a
  tiny local http server for `docs/html/` — same friction the sibling's report already noted.

## Suggestions / feedback

- The docs-site standard is written around the docdash/JSDoc reference and doesn't cover **generator
  themes that ship their own light/dark system** (doxygen-awesome, and likely others). A short
  "integrating with a themed generator" note — own the generator's theme class, beat its palette
  specificity, disable its built-in toggle — would save every Doxygen/MkDocs-Material node the same
  reverse-engineering. (Ties to both bugs above.)

## Environment

- **Repo/stack:** `pokered-save-editor-2` — Qt 6 C++/QML app; docs are **Doxygen 1.17** + vendored
  doxygen-awesome, themed by hand-authored `docs/fairyfox/` (CSS + templates + one vanilla-JS file).
  The chrome is static in the Doxygen templates; the reader is injected client-side. This made it a
  faithful reimplementation of the sibling's JSDoc theme, not a copy.
- **OS/shell:** Windows, PowerShell (per the standing "use PowerShell, not the bash sandbox" rule).
- **Branch model on arrival:** git-flow; docs-site-only, so no `VERSION` bump — rides to `main` with
  the next real release.
- **Verify:** local `doxygen Doxyfile` clean (only pre-existing doc-comment warnings). Live in-browser
  check over http: chrome/subnav correct, hub fox logo, Projects active; reader panel matches the hub;
  dark/light/sepia each repaint the full page with correct contrast; accent + size apply live; sidebar
  present on `annotated.html`, absent on `index.html`/`pages.html`; `doxygen_width` healthy after a
  prose→API navigation; no console errors.
