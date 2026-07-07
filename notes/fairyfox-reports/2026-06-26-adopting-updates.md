---
date: 2026-06-26
procedure: adopting-updates
node: pokered-save-editor-2
outcome: completed
hub_version: 0.8.1
hub_commit: 42263fe
---

# Process Report — adopting-updates, 2026-06-26

> A full, honest account of running a fairyfox system procedure. The point is to
> improve the system — so say what was rough even if the run succeeded. Standard:
> `hub/standards/process-reports.md`.

## Outcome in one line

Adopted the full git-flow standard, the process-reports standard, and the compliance/Verify
layer into this repo; the one unavoidable divergence (CI owns release tagging, not a manual
`git tag`) was recorded on purpose.

## What was done

1. Refreshed the read-only hub clone. The `--ff-only` pull aborted because the hub's `dev` had
   been force-pushed (history rewritten), so I used the runbook's documented fallback: delete +
   re-clone fresh. Landed at hub `dev` 42263fe, VERSION 0.8.1.
2. Worked out what changed since the last adoption (docs-site theming, ~0.5–0.6.x) by reading the
   hub git log + version numbers — the new material was git-flow (0.6.x–0.7.2), process-reports
   (0.7.0), and the compliance enforcement layer (0.8.0).
3. Adopted git-flow into `notes/reference/git-workflow.md` (full rewrite: `--no-ff` tagged-release
   `main`, `feature/`/`release/`/`hotfix/` branches, PATCH direct / MINOR-MAJOR via `release/*`,
   plus a `## Verify` section), keeping the project-specific bits (Windows `VERSION` checkout
   gotcha, the already-done `master→main` history).
4. Updated `CLAUDE.md` (the git-workflow table row + Default Workflow step 4 release loop, swapping
   `merge --ff-only` for the git-flow `--no-ff` release).
5. Reconciled `versioning.md` (release path by SemVer level + `## Verify`) and `cross-project-sync.md`
   (added the process-reports + compliance sections and the report step).
6. Stood up `notes/fairyfox-reports/` (README + this report).
7. Updated `release.yml`'s header comment to describe the `--no-ff` model and the CI-owns-tagging
   divergence (no functional change to the pipeline).

## What went well

- The re-clone fallback in `adopting-updates.md` step 1 is exactly right and was needed verbatim —
  the force-push case is real and the runbook anticipates it.
- The "copy the change, not the file / keep deliberate divergences" guidance made the right call
  obvious: this repo's adopted copies are heavily path-adapted, so a hand-merge (not an overwrite)
  was clearly correct.
- The per-standard `## Verify` tables are easy to drop into an adopted copy and immediately useful
  for self-auditing.

## What went wrong / friction

- **The release runbook conflicts with a project that auto-tags in CI.** `adopting-updates.md`
  step 5 and `git-workflow.md` → "Cutting a release" end with a manual
  `git tag -a vX.Y.Z && git push --tags`. This repo's `release.yml` already derives `v<VERSION>`
  and creates the tag itself (softprops) on the `main` push; its `version` job *skips the release
  if the tag already exists*. So following the runbook literally — hand-pushing the tag — would
  make every release silently no-op. I had to diverge: the pipeline owns tagging, and the
  release merge to `main` carries no hand tag. This was the single biggest snag and is invisible
  in the standard.
- **Detecting "what's new since last adoption" is manual.** The hub's `.last-seen.yml` records a
  per-project *commit*, but the node keeps no record of the hub VERSION it last adopted, and the
  hub's `dev` history was rewritten (the previously-cloned commit was gone), so I couldn't just
  `git log old..new`. I reconstructed the delta from hub commit subjects + version numbers. It
  worked but was guess-adjacent.
- The raw `diff hub/standards/X.md notes/reference/X.md` is dominated by intentional local
  adaptation, so the diff is noisy and not a reliable "what changed upstream" signal.

## Suggestions / feedback

- **git-workflow.md / adopting-updates.md step 5 should acknowledge the CI-owns-tagging case.** Add
  a sentence: "If the project's release pipeline creates the version tag, do NOT also tag by hand —
  the merge to `main` is the release act and CI applies the tag; a hand-pushed tag will make a
  tag-gated release run skip itself." Right now nothing warns about this and it would bite any node
  with a tag-gated `release.yml`.
- **Give the node a lightweight "last adopted hub version" marker** (e.g. a line in the project's
  `cross-project-sync.md`, or a tiny `notes/fairyfox-reports/.adopted` field) so the next adoption
  has a bounded "since X" instead of reconstructing from the hub log — especially since hub `dev`
  gets force-pushed and old commits vanish.
- The adopting-updates "See what changed" step could note that diffing the canonical hub file
  against a heavily-adapted node copy is noisy, and suggest skimming the hub's `notes/version/`
  changelog as the primary signal instead (it already hints at this, but lead with it).

## Environment

Qt 6 C++/QML desktop app (Windows, llvm-mingw kit), notes-heavy living-docs system. Driven from
PowerShell (the Cowork bash sandbox is banned here for stale-mount reasons). The repo was already
on `main` with a `dev`/`main` split and a working tag-gated `release.yml` + `pages.yml`, so the
`master→main` half of git-flow was a no-op — the friction was entirely in the release/tagging
mechanics, not the branch rename. Run was an autonomous scheduled check that the owner then told
to proceed into applying.
