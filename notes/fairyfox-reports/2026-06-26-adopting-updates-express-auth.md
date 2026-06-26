---
date: 2026-06-26
procedure: adopting-updates
node: pokered-save-editor-2
outcome: completed
hub_version: 0.9.2
hub_commit: 42b6ea4
---

# Process Report — adopting-updates, 2026-06-26 (express-authorization)

> Started as the standing automated "check the fairyfox system for updates" run
> (scheduled task), then Twilight gave an explicit go-ahead in the same session, so
> it became an adopt. Per the process-reports rule, a check that becomes an adopt in
> one session is **one combined `adopting-updates` report** — this file. (Distinct
> from the morning's `2026-06-26-adopting-updates.md`, which was a separate, earlier
> run against hub 0.8.1.)

## What was done

1. Refreshed the read-only, git-ignored hub mirror at `assets/references/fairyfox.io/`.
   The `--ff-only` refresh **aborted** — hub `dev` had been force-pushed
   (`42263fe → 42b6ea4`, routine). Reconciled the mirror with `fetch` +
   `reset --hard origin/dev` (mirror only — never project history). New mirror HEAD
   `42b6ea4`, hub `VERSION` **0.9.2**.
2. Diffed hub standards/templates `42263fe..42b6ea4` against this repo's adopted
   copies. Anchor from the prior report (`2026-06-26-adopting-updates.md`): hub
   **0.8.1**. So this pass covered **0.8.1 → 0.9.2**.
3. Reported to Twilight, who said proceed. Adopted the changes below the git-flow way
   (dev commit; notes/docs-only, so no `VERSION` bump and no changelog entry).

## What changed in the fairyfox system (0.8.1 → 0.9.2)

- **New express-authorization mechanism (the significant one):** new hub
  `authorizations.yml` ledger with a standing `express-authorization-rollout` entry
  (`expires: null`) covering `cross-project-sync.md`, `adopting-updates.md`,
  `authorizations.yml`, and `templates/CLAUDE.md`. A node may adopt a covered change
  without the check-and-report pause — skipping *only* that pause; all other safety
  steps still run. Companion edits in `cross-project-sync.md` (ledger read + an
  anti-recursion bullet), `templates/CLAUDE.md` (the "Exception — pre-authorized
  changes" paragraph + reworded guardrails), and `adopting-updates.md` (+132 lines:
  the pre-authorized path and force-push/version-anchor handling).
- **`cross-project-sync.md`:** documents the `reset --hard` mirror fallback for a
  force-pushed `dev` (exactly the reconcile used this run).
- **`git-workflow.md`:** new "Who creates the tag — CI vs. by hand" subsection
  (already covered in substance by this repo's copy).
- **`process-reports.md` + `fairyfox-report.md` template:** `hub_version` is now the
  durable "last adopted" anchor (real number, not a placeholder, since a force-push
  can erase the SHA); a check-that-becomes-an-adopt is one combined report; a
  check-only run on a node lacking `notes/fairyfox-reports/` reports inline.

## What was adopted here

- **`CLAUDE.md` mesh block** — added the "Exception — pre-authorized changes"
  paragraph and reworded the guardrails to cover the express-authorization ledger.
- **`notes/reference/cross-project-sync.md`** — added the `reset --hard` force-push
  mirror fallback to the refresh commands; added the express-authorization ledger
  read paragraph and its anti-recursion bullet.
- **Behavioral (no local file — read from the hub clone at run time):**
  `adopting-updates.md`, `process-reports.md`, `hub/authorizations.yml`. This node
  now honors express-authorizations going forward.
- **No change needed:** `git-workflow.md` already states the CI-owns-tagging rule;
  the report template already records a real `hub_version`.

This adoption was itself pre-authorized by the standing `express-authorization-rollout`
entry, but as the node had not yet adopted the express-auth feature, the run still
deferred to Twilight's explicit go-ahead — which was given. Future covered changes
can adopt without the pause.

## What went well

- The force-push abort was anticipated and cleanly reconciled on the mirror.
- Both diff endpoints survived in the shallow mirror, so the change set was exact.
- The edits were confined to notes/docs (`CLAUDE.md`, `notes/reference/`), so no
  build/test cycle and no version movement were warranted.

## Friction / suggestions

- The adopted `cross-project-sync.md` previously shipped only the `--ff-only` refresh,
  which reliably aborts against the force-pushed hub `dev`; this adoption closes that
  recurring snag by documenting the `reset --hard` mirror fallback.
- Worth a standing decision from Twilight: should unattended scheduled check runs ever
  auto-apply express-authorized change sets, or always defer to a human? This run
  deferred even though the change was pre-authorized.

## Outcome

**completed.** Adopted into `dev`; notes/docs-only (no `VERSION` bump, no changelog
entry, no `main` release). Committed as a reviewable act.
