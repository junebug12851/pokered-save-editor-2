---
date: 2026-07-19
procedure: adopting-updates
node: pokered-save-editor-2
outcome: completed
hub_version: 0.20.2
hub_commit: 697bc5c
chrome_version: 2.2.1
---

# Process Report — adopting-updates (chrome 2.2.1), 2026-07-19

> A full, honest account of running a fairyfox system procedure. The point is to improve the system —
> so say what was rough even if the run succeeded. Voice: direct, matter-of-fact, no hype.

## Outcome in one line

Adopted the shared-chrome bundle **2.0.0 → 2.2.1** into the Doxygen docs site + screenshots gallery —
re-vendoring `main.css` / `reader.js` / `nav.js` and adding the new **`coins.js`** reading-engagement
coin counter — preserving the self-hosted-fonts divergence. Verified in-browser (coin button injects
and counts across page loads; light + dark; API treeview pinned).

## Sequence

This is the applied half of `2026-07-19-check-for-updates.md`. That pass checked, found the chrome
bump adds a **new user-facing coin counter**, and stopped to surface it (a visible UX addition
warranted Twilight's nod even though the standing `adopt-standards-by-default` ledger entry
pre-authorizes it). Twilight replied **"adopt all please"** — so this run applied it under that
standing authorization, with the full safety floor still running.

## What was done

1. Vendored the master assets from the read-only hub mirror (`697bc5c`, site 0.20.2) into
   `docs/fairyfox/`: `main.css` (48.5 KB → 55.1 KB), `reader.js`, `nav.js`, and the **new**
   `coins.js` (19.2 KB). Confirmed the refreshed `main.css` carries **zero** Google-Fonts /
   `@font-face` refs, so the self-hosted-fonts divergence is untouched by the CSS swap.
2. Pinned `docs/fairyfox/CHROME_VERSION` → **2.2.1**.
3. Wired the `coins.js` `defer` load **after** `reader.js` in the Doxygen `footer.html` template and
   in `scripts/build_gallery.py`'s `FF_SCRIPTS`; updated both comments.
4. Added `docs/fairyfox/coins.js` to the Doxyfile `HTML_EXTRA_FILES` (the mechanism that copies the
   bundle behaviour files flat into `docs/html/`) — without this the script tag would 404.
5. Rebuilt Doxygen (exit 0, no chrome-related warnings; the 71 warnings are pre-existing content
   refs) and confirmed `coins.js` deployed + referenced across all generated pages.

## Verification (in Chrome, rendered, JS on)

- **Coin button injects** as `.ff-coin-btn` into `.site-header .wrap`, just left of the reader "Aa"
  button; both present, nav present. The counter **incremented 1 → 2** across page navigations —
  the reading-engagement tracking is live.
- **Self-hosted fonts intact**: body computed font is `Inter`, `fonts.css` link present, and there are
  **no** `googleapis`/`gstatic` links on the page. The documented deviation holds.
- **Layout intact** both themes: Overview = full-width prose (no treeview overlap); API `annotated.html`
  = treeview pinned below the sticky chrome, content right, no collapse. Light + dark both correct.
- `coins.js` is fully client-side (localStorage only; no network fetch), self-guards on
  `.site-header .wrap`, and degrades gracefully where it can't find a reading-content container (it
  looks for `main .content|.prose|article|main`, which Doxygen API pages don't expose — the button
  still works, it just won't award reading coins on those pages). Acceptable, matches the
  "reference body is reference" boundary.

## What was rough

- The bundle's behaviour files must be listed in the Doxyfile's `HTML_EXTRA_FILES` to be deployed;
  `coins.js` was **not** copied on the first rebuild until added there. A one-line "new bundle JS →
  add to HTML_EXTRA_FILES" note in the hub's Doxygen adapter would catch this on every future part
  the bundle adds. (Same class as this morning's "layout reconciliation" adapter gap.)
- No chrome `CHANGELOG.md` in the bundle, so "what changed 2.0.0 → 2.2.1" was inferred from the
  README + VERSION + the presence of `coins.js`. A bundle changelog would make it a read.

## Not applied

VERSION unmoved (docs/notes/build-script change only — no app code, per versioning.md). No `main`
release (adoption committed to `dev`; ship is Twilight's word).
