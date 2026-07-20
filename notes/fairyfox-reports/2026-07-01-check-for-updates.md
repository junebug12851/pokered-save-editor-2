---
date: 2026-07-01
procedure: check-for-updates
node: pokered-save-editor-2
outcome: new-adopted-standard-changes-pending
hub_version: 0.11.0
hub_commit: 2ffe455
prev_checked_version: 0.9.14
prev_checked_commit: 0fb30be
---

# Process Report — check-for-updates, 2026-07-01 (scheduled)

> Standing automated "check the fairyfox system for updates" run (scheduled task,
> owner not present). Default flow: **check, report, then wait** — nothing applied.
> **Unlike the last several passes, this one has real adopted-standard movement:** the
> hub advanced two milestones (0.10.0 "one-site-standards", 0.11.0 "one-site-frontend")
> and 0.10.0 refined `cross-project-sync.md` (adopted) again and added **two new shared
> standards** (`planning.md`, `deployment.md`).

## What was done

1. Refreshed the read-only, git-ignored hub mirror at `assets/references/fairyfox.io/`.
   Mirror moved `0fb30be` (0.9.14) → `2ffe455` (0.11.0). The refresh touched the
   **mirror only**, never project history.
2. Diffed the refreshed hub `hub/standards/`, `hub/templates/`, and
   `hub/authorizations.yml` against this repo's adopted copies
   (`notes/reference/cross-project-sync.md`, `git-workflow.md`, `versioning.md`,
   `deployment.md`, the `CLAUDE.md` mesh + Default-Workflow blocks).
3. Concluded the deltas and stopped. Made no edits, no commits, no pushes — per the
   scheduled-run guardrails and the check-report-wait default. (The one ledger-covered
   change was **not** auto-applied either: this is the check flow and the task
   explicitly said apply nothing.)

## What changed in the fairyfox system (0.9.14 → 0.11.0)

Four commits, across two milestones:

- `0f77022` **feat(standards): one-seamless-site model, deployment + planning standards,
  shallow-clone cleanup (0.10.0)** — the substantive one.
- `560d059` Merge feature/one-site-standards into dev (0.10.0)
- `1de98e2` feat(site): shared submenu nav (.subnav) + advance digested report markers (0.11.0)
- `2ffe455` Merge feature/one-site-frontend into dev (0.11.0)

### Touches an ADOPTED standard

1. **`cross-project-sync.md` — refined again (the "shallow-clone cleanup").** The hub's
   canonical copy now drops even the "full-history, not shallow" emphasis and simplifies
   to: single-branch clone, *"an ordinary clone fast-forwards every time, since `dev` is
   append-only across the mesh"*; if `--ff-only` ever aborts it's an **anomaly** — the
   mirror is disposable/git-ignored, so **delete and re-clone**, don't `reset --hard`
   through it. **This node's committed copy still describes the OLD `--depth 1` shallow /
   "force-pushed routinely → `reset --hard`" model** — so the long-standing carry-over
   not only persists, its target has moved further from the node's copy. **Pre-authorized**
   by the standing `express-authorization-rollout` ledger entry (covers
   `cross-project-sync.md`, `expires: null`).

### Net-new shared standards (not yet adopted by this node)

2. **`planning.md` — NEW "Plan Before Execute" standard.** A mesh-wide *default way of
   working*: for non-trivial work write a short structured plan (decisions, work
   breakdown by file/area, open items, release shape) in `notes/plans/` **before**
   executing; trivial one-step changes exempt. It's wired into the **`templates/CLAUDE.md`
   Default Workflow** (a new "Plan before you execute" paragraph ahead of the build/test
   loop). This node's `CLAUDE.md` Default Workflow does **not** yet state
   plan-before-execute, and it has no `notes/reference/planning.md`.
3. **`deployment.md` — NEW mesh-wide deployment-policy standard.** "Where each kind of
   project deploys": static content → GitHub Pages on the shared `fairyfox.io/<key>/`
   domain; built/runnable apps → Netlify; games are the static-collection exception on
   Pages. **Note the name collision:** this node already has
   `notes/reference/deployment.md`, but that is **project-specific** (its own
   `release.yml` pipeline), a *different scope* from this new shared hosting-policy
   standard. This node's docs+screenshots already deploy to GitHub Pages on the shared
   domain, so it is already *compliant* with the new standard's substance; adoption here
   would be optional (a pointer/short adopted copy), not a behavior change.

### Non-adopted / cosmetic

4. **`hub/templates/CLAUDE.md`** — besides the plan-before-execute addition (above),
   general mesh-block text; the express-auth ledger only covers this template's
   **mesh-awareness block**, *not* the Default-Workflow section, so the
   plan-before-execute wiring is **not** pre-authorized.
5. **`hub/templates/project.gitignore`** — cosmetic: comment reworded "shallow clones" →
   "clones". No functional change to `assets/references/*`.
6. Also changed but **not adopted by this node** (hub-internal / process / docs-site):
   `adopting-updates.md`, `ai-context.md`, `compliance.md`, `new-project-setup.md`,
   `onboarding-existing-project.md`, `process-reports.md`, and `docs-site/*` (incl. new
   `reference/chrome.html`, `reference/main.css` for the shared site chrome). 0.11.0 is
   the site frontend (shared `.subnav` submenu nav, digested-report markers).

### Authorization ledger

**`hub/authorizations.yml` is byte-for-byte unchanged.** Still the single standing
entry `express-authorization-rollout` (2026-06-26, Fairy Fox, `grants:
adopt-without-reprompt`, `expires: null`) covering `cross-project-sync.md`,
`adopting-updates.md`, `authorizations.yml`, and `templates/CLAUDE.md` (mesh block only).

## What adopting would touch in this repo

1. **`notes/reference/cross-project-sync.md`** (primary; **ledger-pre-authorized**) —
   rewrite the clone/refresh commands + the "force-pushed routinely → `reset --hard`"
   narrative to the current model: single-branch clone, `fetch` + `merge --ff-only`,
   `dev` is append-only mesh-wide, `--ff-only` abort = anomaly → delete & re-clone the
   disposable mirror.
2. **The git-ignored hub mirror is STILL SHALLOW** (`--depth 1`). Under the current
   model it should be re-cloned single-branch full-history so `--ff-only` works and the
   phantom "forced update" signal stops. Mirror-only, produces no commit. **This is the
   4th consecutive check flagging the shallow mirror** — this pass again used the old
   `fetch` + `reset --hard` path; the reset succeeded only because `dev` was genuinely
   append-only (not rewritten).
3. **`CLAUDE.md` (project root) Default Workflow** (NOT pre-authorized) — add a
   plan-before-execute paragraph per the new `planning.md` standard, and (optionally)
   adopt `notes/reference/planning.md`.
4. **`deployment.md` shared standard** (NOT pre-authorized) — decide whether to add a
   short adopted copy / pointer; substance already satisfied (Pages on shared domain).
   Mind the name clash with the existing project-specific `notes/reference/deployment.md`.
5. **`.gitignore`** — the "clones" wording is cosmetic; optional.

No `VERSION` movement, no app behavior change, no save-file surface involved. Adopting
1–2 (the pre-authorized sync fix + mirror re-clone) is one small, self-contained commit;
3–4 (planning, deployment) are separate adoption decisions needing project leadership's go-ahead.

## Verification

- Diff anchored on the last report's baseline (`0fb30be`/0.9.14) → current
  (`2ffe455`/0.11.0); confirmed via `git log --oneline 0fb30be..2ffe455` (4 commits) and
  `git diff --stat 0fb30be..2ffe455 -- hub/` (22 files).
- Read the full `cross-project-sync.md` diff — confirmed it is a refinement of the same
  model this node's copy still lags, and confirmed `git-workflow.md` and `versioning.md`
  are **not** in the changed-file set (unchanged this range).
- Read `planning.md` and `deployment.md` in full/headline — confirmed both are net-new
  standards absent from this node.
- Confirmed `authorizations.yml` unchanged and still carries only the standing
  `express-authorization-rollout` entry; confirmed its `covers` list scopes
  `templates/CLAUDE.md` to the mesh block, so the new Default-Workflow plan-before-execute
  wiring is NOT pre-authorized.
- `git status` clean; the git-ignored mirror produced no tracked change. No commits, no
  pushes, nothing applied.

## Friction / suggestions

- **Shallow-mirror concern recurs a 4th time.** Still `--depth 1`; this run again used
  the deprecated `fetch` + `reset --hard` path. Clearing the `cross-project-sync.md`
  carry-over (re-clone full-history single-branch) ends this recurring false signal — and
  it's ledger-pre-authorized, so it needs only a single go-ahead.
- **Real movement this pass, after several quiet ones.** Worth a short review session:
  the pre-authorized sync fix (+ mirror re-clone) can go in immediately on go-ahead; the
  two new standards (`planning`, `deployment`) are genuine adoption decisions — `planning`
  in particular would change the project's default working loop (plan-first), so it merits
  a deliberate yes rather than a silent copy.
- The `deployment.md` name collision (shared hosting-policy standard vs. this node's
  project-specific release-pipeline doc) is a small trap for a future adopter — flag it if
  adopting so the shared standard doesn't overwrite the local file.

## Outcome

**new-adopted-standard-changes-pending.** The hub advanced 0.9.14 → 0.11.0; 0.10.0
refined the adopted `cross-project-sync.md` (further from the node's still-old copy;
pre-authorized) and added two net-new shared standards (`planning.md`, `deployment.md`)
plus a plan-before-execute addition to the `CLAUDE.md` template Default Workflow (not
pre-authorized). `authorizations.yml` unchanged. Nothing applied per the scheduled-run
guardrails and the explicit "apply nothing" task instruction; awaiting project leadership's
go-ahead. (This report was written but, per the scheduled-run no-commit guardrail, left
**uncommitted** for review.)
