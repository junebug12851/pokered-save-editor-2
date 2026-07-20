---
date: 2026-07-19
procedure: check-for-updates
node: pokered-save-editor-2
outcome: completed (check only — project leadership then said "adopt all"; applied in 2026-07-19-adopting-updates-chrome-2.2.1.md)
hub_version: 0.20.2
hub_commit: 697bc5c
chrome_version_hub: 2.2.1
chrome_version_adopted: 2.0.0
---

# Process Report — check-for-updates, 2026-07-19

> A full, honest account of running a fairyfox system procedure. The point is to improve the system —
> so say what was rough even if the run succeeded. Voice: direct, matter-of-fact, no hype.

## Outcome in one line

Refreshed the hub mirror and diffed it against the project's adopted copies. The hub advanced
**0.16.1 → 0.20.2** since this morning's adoption run; the one material, adoptable delta is the
**shared-chrome bundle 2.0.0 → 2.2.1**, which adds a new user-facing **Coins reading-engagement
counter** (`coins.js`) to the docs-site chrome. Reported and stopped — surfaced for project leadership's call
because it is a visible UI addition and re-adopting the chrome requires full in-browser verification.

## What was done

1. Refreshed the read-only hub clone at `assets/references/fairyfox.io/`. The hub's `dev` had been
   force-pushed (routine), so the `--ff-only` pull aborted and the `reset --hard origin/dev` fallback
   ran on the git-ignored mirror — landing at `697bc5c`, site version **0.20.2** (was 0.16.1 at this
   morning's adoption).
2. Read the express-authorization ledger (`hub/authorizations.yml`). Two active entries: the standing
   `adopt-standards-by-default` (2026-07-02, expires null) covering `hub/standards/` + `hub/templates/`,
   and `express-authorization-rollout` (2026-06-26). So any standard/template change is pre-authorized
   — the confirmation pause is skipped, but the full verification floor and copy-not-clobber still bind.
3. Diffed the hub against what the project runs. Findings below.

## What changed in the hub (0.16.1 → 0.20.2)

- **Shared-chrome bundle 2.0.0 → 2.2.1.** The bundle now ships a fourth behaviour file, **`coins.js`**
  — a "reading-engagement" coin counter injected into the header just left of the reader "Aa" button
  (load order: `nav.js`, `reader.js`, then `coins.js`, all `defer`). There is a matching `coins.md`
  standard. The project adopted 2.0.0 this morning and does **not** carry coins.
- **27 standards present** in `hub/standards/`. The project's committed adopted subset is
  `cross-project-sync.md`, `git-workflow.md`, `versioning.md`, the `CLAUDE.md` mesh block, the
  notes-skeleton, and the docs-site chrome. The remaining standards (badges, coins, dependencies,
  deployment, engineering-quality, maintenance-sweep, repo-hygiene, research-capture,
  supply-chain-hardening, testing, working-rhythm, farm-operating-model, planning, agent-tooling,
  legal-docs, self-hosted-assets, …) remain **un-adopted** — nothing new required them this pass.
- The three text standards the project tracks (`cross-project-sync`, `git-workflow`, `versioning`)
  showed no new load-bearing rule against the project's adapted copies.

## What adopting the chrome bump would touch

Re-vendor the master `main.css` / `reader.js` / `nav.js` **plus the new `coins.js`** into
`docs/fairyfox/`; add the `coins.js` `defer` load after `reader.js` in the Doxygen `footer.html`
template + the gallery builder; bump `docs/fairyfox/CHROME_VERSION` → `2.2.1`; **preserve the
deliberate self-hosted-fonts divergence** recorded this morning (must not be clobbered); then verify
in Chrome across prose/API pages and light/dark themes (the coin button appears and counts; nothing
else shifted).

## Why nothing was applied this pass

Two reasons, both safety-floor, not the authorization:

- **The coins counter is a new user-facing element** (a reading-engagement counter on the docs site).
  That is a UX addition worth project leadership's explicit nod, not a silent vendor — the same instinct that
  made the font deviation a "surface and ask" this morning.
- **Full verification is in-browser and substantial.** A chrome bump is a visual change; verifying it
  properly means re-running the Doxygen + gallery build and reviewing rendered pages across themes.
  Per the standing rule, when full verification of a pre-authorized change can't be cleanly completed
  in-pass, fall back to check-report-wait rather than auto-apply.

## Verification

- Mirror landed at `697bc5c` / `0.20.2`; ledger read read-only from the mirror; no tracked file in the
  project was modified (this report aside). Git safety: the concurrent map-states/World-panel WIP in
  the working tree was left untouched.

## Would improve the procedure

- Same standing recommendation as this morning still open: **self-host the fonts in the bundle** so no
  node needs the font deviation. (Filed to the hub in `2026-07-19-adopting-updates.md`.)
- A chrome bundle **CHANGELOG.md** (2.0.0 → 2.1.x → 2.2.1) would make "what changed" a read instead of
  an inference from the README/VERSION and the new `coins.js` file.
