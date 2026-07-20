---
date: 2026-06-29
procedure: check-for-updates
node: pokered-save-editor-2
outcome: changes-found-not-applied
hub_version: 0.9.9
hub_commit: bbf9b70
prev_checked_version: 0.9.6
---

# Process Report — check-for-updates, 2026-06-29 (scheduled)

> Standing automated "check the fairyfox system for updates" run (scheduled task,
> owner not present). Default flow: **check, report, then wait** — nothing applied.
> One standing carry-over (`cross-project-sync.md`) plus one small new upstream
> change to `git-workflow.md`; both already substantively handled / low-impact.

## What was done

1. Refreshed the read-only, git-ignored hub mirror at `assets/references/fairyfox.io/`.
   Mirror moved `300b8ab` (0.9.6) → `bbf9b70` (0.9.9). The refresh touched the
   **mirror only**, never project history.
2. Diffed the refreshed hub standards/templates/ledger against this repo's adopted
   copies (`notes/reference/cross-project-sync.md`, `git-workflow.md`, `versioning.md`,
   the `CLAUDE.md` mesh block). The diff is cumulative against the live hub state, so it
   captures every delta regardless of intermediate commits.
3. Concluded the adopted-standard deltas and stopped. Made no edits, no commits, no
   pushes — per the scheduled-run guardrails and the check-report-wait default.

## What changed in the fairyfox system (0.9.6 → 0.9.9)

- **0.9.7 / 0.9.8** — a hub **report-review pass** that folded sibling process-report
  themes into the standards. Of the standards this node adopts, the one substantive
  touch is in **`git-workflow.md`**: a new **"Who creates the tag — CI vs. by hand"**
  callout under *Cutting a release*, plus a forward-pointer at the top of that section.
  (Other folded themes landed in `adopting-updates.md` and `process-reports.md`, which
  this node does not keep committed copies of.)
- **0.9.9** (`bbf9b70`) — "reconcile to Random AI Prompt 2.14.0 (Manage tab + focus
  dial), blog the day." Hub **site content** for another node + a blog entry. **Nothing
  for this node** — touches no standard this project adopts.

### Adopted-standard deltas this pass

1. **`cross-project-sync.md` — standing carry-over from the 2026-06-28 report (the
   0.9.6 sync fix), still un-adopted.** The hub's model is now: clone **single-branch
   full-history** (`--branch dev --single-branch`, *not* `--depth 1`); refresh =
   `fetch origin dev` + `merge --ff-only origin/dev`, which should **always** succeed;
   **`dev` is append-only across the mesh — nothing force-pushes it**; a `--ff-only`
   abort is an **anomaly to diagnose** (usually a leftover shallow mirror that can't
   compute a merge base — fix with `fetch --unshallow` / re-clone), **not** a routine to
   `reset --hard` through. This node's committed copy still describes the **old** model
   ("`dev` force-pushed routinely → `reset --hard` fallback", `--depth 1`). Unchanged
   since the prior report — still the primary thing adoption would rewrite.

2. **`git-workflow.md` — NEW this pass (0.9.8), but already substantively aligned.** The
   hub generalized *Cutting a release* so the canonical default is **hand-tagging**, with
   **CI-owns-tagging** documented as a divergence to check `release.yml` for (new
   "Who creates the tag — CI vs. by hand" subsection). This node's `git-workflow.md`
   **already encodes** exactly this project's CI-owns-tagging path (CI derives
   `v<VERSION>`; do not hand-tag). So the substance is already correct here; adopting the
   hub's framing would be **wording/structure only**, no behavioral change.

3. **`versioning.md` — unchanged in substance.** The hub copy stays the short canonical
   form; this node's longer project-specific copy already carries the substantive rules
   (CI owns tag, PATCH default, never auto-MAJOR).

### Express-authorization status

The standing `express-authorization-rollout` entry in `authorizations.yml`
(`expires: null`) **covers `hub/standards/cross-project-sync.md`** (among others). So the
cross-project-sync sync-fix adoption is, by the ledger, **pre-authorized** — under the
normal interactive flow a node could adopt it without the re-prompt pause (verification
floor still mandatory). The new `git-workflow.md` callout is **not** covered by any
active entry. Per the scheduled-run guardrails and the explicit task instruction, this
run still **applies nothing** and waits for project leadership's go-ahead either way.

## What adopting would touch in this repo

1. **`notes/reference/cross-project-sync.md`** (primary) — rewrite the refresh/clone
   commands and the "force-pushed routinely → `reset --hard`" narrative to the
   single-branch full-history + `merge --ff-only` model, with the
   ff-only-abort-is-an-anomaly diagnosis. ~3 short blocks; no structural change.
2. **The git-ignored hub mirror is still SHALLOW** — under the new model it should be
   deepened (`fetch --unshallow`) or re-cloned single-branch full-history. Operational
   change to the disposable mirror only; produces no commit. (This pass again used the
   old shallow `fetch --depth 1` + `reset --hard` path, which reported a "forced update"
   — exactly the shallow-mirror false signal the new standard warns about; it succeeded
   only because `dev` wasn't actually rewritten.)
3. **`notes/reference/git-workflow.md`** (optional/cosmetic) — could absorb the hub's
   new "Who creates the tag" framing, but the project's CI-owns-tag path is already
   documented; substance needs no change. Optionally add an explicit mesh-wide "`dev` is
   append-only — nothing force-pushes it" line so the new `cross-project-sync.md`
   cross-reference resolves concretely (the general no-force-push rule already covers it
   in spirit).
4. **`CLAUDE.md` mesh block** — no change needed; its wording is general and does not pin
   the old shallow/reset mechanism.

No `VERSION` movement, no app behavior change, no save-file surface involved.

## Verification

- Diff anchored on hub `VERSION` (0.9.6 last reported → 0.9.9 now); cross-checked against
  the hub's own `.last-seen.yml` round-up notes (standards changes at 0.9.7/0.9.8; 0.9.9
  is RAP site content).
- Confirmed `authorizations.yml` carries the single standing `express-authorization-rollout`
  entry (covers `cross-project-sync.md`; does **not** cover the new `git-workflow.md`
  callout).
- Confirmed `versioning.md` substance unchanged; `git-workflow.md` substance already
  aligned (CI-owns-tag) — only framing differs upstream.
- `git status` clean; the git-ignored mirror produced no tracked change. No commits, no
  pushes, nothing applied.

## Friction / suggestions

- **The shallow-mirror concern recurs:** the mirror is still `--depth 1`, so this run
  again used the deprecated `fetch + reset --hard` path (and saw the phantom "forced
  update"). Adopting the 0.9.6 sync fix should include re-cloning the mirror full-history
  so future refreshes are clean `merge --ff-only` fast-forwards and a real abort becomes
  a meaningful signal. This is the second consecutive report to flag it.
- The `cross-project-sync.md` carry-over is pre-authorized by the ledger but has now sat
  un-adopted across two checks — worth a single go-ahead to clear it (plus the optional
  `git-workflow.md` framing refresh) in one small commit.

## Outcome

**changes-found-not-applied.** Standing carry-over: the 0.9.6 `cross-project-sync.md`
sync fix remains un-adopted (pre-authorized by the ledger). New this pass: a small,
substance-neutral `git-workflow.md` "Who creates the tag" framing change (0.9.8) — this
node already does CI-owns-tagging, so no behavioral gap. 0.9.9 is unrelated hub site
content. Nothing applied per the scheduled-run guardrails; awaiting project leadership's go-ahead.
(This report was written but, per the scheduled-run no-commit guardrail, left
**uncommitted** for review.)
