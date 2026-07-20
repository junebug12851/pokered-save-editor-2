---
date: 2026-06-28
procedure: check-for-updates
node: pokered-save-editor-2
outcome: changes-found-not-applied
hub_version: 0.9.6
hub_commit: 300b8ab
prev_checked_version: 0.9.4
---

# Process Report — check-for-updates, 2026-06-28 (scheduled)

> Standing automated "check the fairyfox system for updates" run (scheduled task,
> owner not present). Default flow: **check, report, then wait** — nothing applied.
> One adopted standard (`cross-project-sync.md`) has a real upstream change this pass.

## What was done

1. Refreshed the read-only, git-ignored hub mirror at `assets/references/fairyfox.io/`.
   Mirror moved `316ed8a` (0.9.5) → `300b8ab` (0.9.6). The reconcile touched the
   **mirror only**, never project history.
2. Diffed the refreshed hub standards/templates/ledger against this repo's adopted
   copies (`notes/reference/cross-project-sync.md`, `git-workflow.md`, `versioning.md`,
   the `CLAUDE.md` mesh block). Captured the **full delta since the last reported
   check** (0.9.4 → 0.9.6), since the mirror had advanced past the last report's anchor.
3. Concluded **one adopted standard changed upstream** and stopped. Made no edits, no
   commits, no pushes — per the scheduled-run guardrails and check-report-wait default.

## What changed in the fairyfox system (0.9.4 → 0.9.6)

Two hub commits since the last reported check:

- **0.9.5** (`316ed8a`) — "reconcile to Random AI Prompt 2.7.25; blog the day."
  Another node's adoption + a blog entry. **Nothing for this node** — touches no
  standard this project adopts.
- **0.9.6** (`16515fa` / merge `300b8ab`) — **`fix(sync): full-history mirrors, kill
  the phantom force-push + routine reset --hard`.** A coherent revision of the
  cross-repo sync mechanism across the hub's sync runbooks
  (`cross-project-sync.md`, `adopting-updates.md`, `compliance.md`,
  `new-project-setup.md`, `onboarding-existing-project.md`, `process-reports.md`,
  `templates/fairyfox-report.md`). **Of the standards this node adopts, only
  `cross-project-sync.md` is affected.** `git-workflow.md`, `versioning.md`, and
  `authorizations.yml` are **unchanged**.

### The substance of the 0.9.6 sync fix

The old model (still described in this node's adopted copy): a **shallow** `--depth 1`
mirror; `dev` is "force-pushed routinely," so a `--ff-only` refresh is *expected to
abort* and the fallback is `reset --hard origin/dev` on the git-ignored mirror.

The new model:

- **Clone single-branch but full-history** (`clone --branch dev --single-branch`),
  *not* `--depth 1`.
- **Refresh = `fetch origin dev` + `merge --ff-only origin/dev`** — which should
  **always succeed**.
- **`dev` is append-only across the whole mesh — nothing force-pushes it** (now stated
  as a hard safety rule that lives in `git-workflow.md`).
- A `--ff-only` abort is **no longer routine** — it's an anomaly to diagnose. The
  usual cause is a *leftover shallow mirror* that can't compute a merge base
  (`refusing to merge unrelated histories`); fix by deepening to full history
  (`fetch --unshallow`) or re-cloning, **not** by `reset --hard`. Only a genuine `dev`
  history rewrite would make a full mirror fail to fast-forward — which must never
  happen; stop and investigate rather than bulldoze.
- The "last-adopted" anchor remains the **hub VERSION** recorded in the latest process
  report (sturdier and self-documenting), though a SHA would now survive too since
  `dev` isn't rewritten.

The `authorizations.yml` ledger is **unchanged** — the single standing entry covers
`express-authorization-rollout`, which does **not** cover this sync-mechanism change.
So there is **no pre-authorization** for it: this stays check-report-**wait**, not
auto-apply.

## What adopting it would touch in this repo

1. **`notes/reference/cross-project-sync.md`** — the primary edit. Its "Checking for
   updates" / refresh commands (the `--depth 1` clone, the "`dev` force-pushed
   routinely → `reset --hard` fallback" narrative, and the matching lines in the two
   flow blocks) would be rewritten to the new full-history + `merge --ff-only` model,
   with the ff-only-abort-is-an-anomaly diagnosis replacing the reset-routine. Net:
   a focused rewrite of ~3 short blocks, no structural change.

2. **The git-ignored hub mirror is currently SHALLOW** (confirmed: `.git/shallow`
   present). Under the new model it should be deepened (`fetch --unshallow`) or
   re-cloned single-branch full-history. This is an **operational** change to the
   disposable mirror only — produces no commit. (Note: *this* pass still refreshed via
   the old shallow path; it worked because `dev` was not in fact force-pushed.)

3. **`notes/reference/git-workflow.md`** — *optional/minor.* The new standard leans on
   a hard "`dev` is append-only — nothing force-pushes it" rule said to live in
   `git-workflow.md`. This repo's `git-workflow.md` has the general no-force-push /
   no-history-rewrite rule (lines ~150, ~197) but **not** an explicit mesh-wide "dev is
   append-only" statement. Adopting cleanly *may* warrant adding that one explicit line
   so the cross-reference resolves; the general rule already covers it in spirit.

4. **`CLAUDE.md` mesh block** — *no change needed.* Its wording is general (the
   one git-safety line at ~L158) and does not pin the old shallow/reset mechanism, so
   it stays correct under the new model.

No `VERSION` movement, no behavioral change to the app, no save-file surface involved.

## Verification

- Diff anchored on hub `VERSION` (0.9.4 last reported → 0.9.6 now) per the standard;
  cross-checked by commit range `6777a73..300b8ab`.
- Confirmed `authorizations.yml` byte-unchanged across the range (no new/changed
  pre-authorization) → correctly handled as check-report-wait.
- Confirmed `git-workflow.md` / `versioning.md` upstream standards unchanged across the
  range — `cross-project-sync.md` is the sole adopted standard affected.
- `git status` clean before and after; the git-ignored mirror produced no tracked
  change. No commits, no pushes, nothing applied.

## Friction / suggestions

- **The new standard's own concern bit this very run:** the mirror was still a
  `--depth 1` shallow clone, so this pass used the deprecated `fetch + reset --hard`
  path. It succeeded only because `dev` wasn't actually rewritten. Adopting 0.9.6 should
  include re-cloning the mirror full-history so future refreshes are clean
  `merge --ff-only` fast-forwards and a real abort becomes a meaningful signal.
- Minor doc-coherence nit for whoever adopts: decide whether to add the explicit
  "dev is append-only" line to this node's `git-workflow.md` so the new
  `cross-project-sync.md` cross-reference points at something concrete.

## Outcome

**changes-found-not-applied.** One adopted standard (`cross-project-sync.md`) has a
real upstream revision at hub 0.9.6 (the full-history / no-force-push sync fix); no
pre-authorization covers it, so nothing was applied. Adopting would touch
`notes/reference/cross-project-sync.md` (primary), re-clone the shallow mirror full
(operational), and optionally add one explicit line to `git-workflow.md`. Awaiting
Project leadership's go-ahead. (This report was written but, per the scheduled-run no-commit
guardrail, left **uncommitted** for review.)
