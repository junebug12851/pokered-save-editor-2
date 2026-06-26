# Cross-Project Sync — the fairyfox mesh

How this project shares standards with, and stays aware of, the **fairyfox.io hub**
**without entanglement.** This is this project's committed copy of the hub's
canonical `cross-project-sync` standard, lightly adapted to local paths. The hub is
the source of truth; re-pull and reconcile by hand (see "Checking for updates" below).

> This project's identity in the mesh: **key** `pokered-save-editor-2`, **branch**
> `dev` (work) fast-forwarding to `main` (stable). The hub lives at
> `github.com/junebug12851/junebug12851.github.io` and is published at fairyfox.io.

## The one rule

**Communication is git-only, one-directional per flow, and happens only on explicit
request.** No submodules, no package dependency, no build-time coupling, no webhooks,
no cross-repo automation. Each side *reads* a shallow clone of the other when a human
or AI deliberately asks. Both flows track the **`dev`** branch (latest work); fall
back to a repo's default branch if it has no `dev`.

This keeps things modular and **prevents recursion**: nothing on one side
automatically triggers a pull on the other, so the repos can't set each other off in
a loop.

## Roles

- **Hub** — the fairyfox.io repo that holds shared standards/templates (`hub/`) and a
  project `registry.yml`. It also *reads* projects to track changes / blog about them.
- **Project** — this repo (and its siblings), which adopts the hub's standards.

## The two flows

### 1. Hub reads projects (inbound)

The hub keeps read-only shallow clones of each project under its own
`assets/references/<project>/` (git-ignored). Nothing for this project to do.

### 2. This project reads the hub (outbound)

This project keeps a read-only shallow clone of the hub under
`assets/references/fairyfox.io/` (git-ignored — see the repo `.gitignore`) and
**copies** what it needs out of `hub/standards/` and `hub/templates/` into its own
tree, committing *that*:

```sh
# first time
git -C assets/references clone --depth 1 --branch dev \
    https://github.com/junebug12851/junebug12851.github.io fairyfox.io
# refresh
git -C assets/references/fairyfox.io pull --depth 1 --ff-only origin dev
# refresh aborts? the hub's dev was force-pushed (routine) — reset the mirror, don't fight it:
git -C assets/references/fairyfox.io fetch origin dev
git -C assets/references/fairyfox.io reset --hard origin/dev   # the git-ignored mirror ONLY — never a tracked branch
```

The hub's `dev` is force-pushed routinely, so the `--ff-only` refresh will usually
abort; the `reset --hard` (or a re-clone) on the **git-ignored mirror** is the
expected fallback — never a `reset` of this project's own history.

Adopting a standard is a **copy committed locally**, not a live link — re-pull later
and merge changes by hand.

Alongside the standards, this project also reads the hub's **express-authorization
ledger** (`assets/references/fairyfox.io/hub/authorizations.yml`) out of the same
read-only clone. It records the go-aheads Twilight makes at the hub; a node adopting a
change the ledger `covers` treats it as pre-authorized and skips its redundant
confirmation pause — **but only that pause; every other adoption safety step still
runs** (copy-not-clobber, divergence re-prompt, process report, reviewable commit).
This is still a read, on request — it adds no automation and no push into the node, so
anti-recursion holds.

## Checking for updates ("check the fairyfox system for updates")

On request only (the request must carry the word *fairyfox* + an update/sync intent —
see the standing instruction in the repo-root `CLAUDE.md`). The default is **check,
report, then wait**:

1. Refresh the read-only clone under `assets/references/fairyfox.io/` (commands above).
2. Diff it against what this project has adopted (this file, `git-workflow.md`,
   `versioning.md`, the `CLAUDE.md` mesh block, the notes skeleton).
3. **Report what changed and what adopting it would touch — then stop.** Apply nothing
   until Twilight says go ahead; applying is a separate, confirmed act. Full procedure:
   the hub's `adopting-updates` runbook (`hub/standards/adopting-updates.md`).
4. **Either way, write a process report** in `notes/fairyfox-reports/` — running this flow
   (applied or check-only) is a fairyfox system interaction, so it ends with an honest report
   the hub reads to improve the procedure. See "Process reports" below.

## Process reports (adopted 2026-06-26)

Every run of a fairyfox system procedure here — setup, onboarding, adopting updates, or a
check-for-updates pass — ends with a **process report** committed under
`notes/fairyfox-reports/` (`YYYY-MM-DD-<procedure>.md`, from the hub's
`templates/fairyfox-report.md`). It's a full, honest account of how the run went and what would
improve the procedure. Reports are **this repo's own record** (committed, unlike the git-ignored
reference clone); the hub *reads* them through its read-only inbound clone, on request — the node
never pushes to the hub, preserving anti-recursion. Canonical standard:
`hub/standards/process-reports.md`.

## Compliance audit (the enforcement layer)

The hub also defines a **standards compliance audit** (`hub/standards/compliance.md`): an
on-request pass that runs each standard's `## Verify` check and reports `done`/`partial`/`missing`.
This project's adopted standards carry their own `## Verify` sections (`git-workflow.md`,
`versioning.md`) so the node can be — and can self-run — that audit. Like every cross-repo read
here it's on-request, read-only, and changes nothing on disk without a go-ahead.

## Anti-recursion checklist

- ✅ Pulls are manual / on request — never scheduled to chain across repos.
- ✅ Each flow is read-only on the far side — sync never pushes into the other repo.
- ✅ Reference clones are git-ignored — a pull produces no commit, so it triggers
  nothing downstream.
- ✅ Adoption is a copy, not a runtime dependency.
- ✅ The express-authorization ledger is **read-only on the far side** like every
  other artifact — a pre-authorization lets a node skip a prompt, never lets the hub
  act on the node. The node still adopts only when Twilight invokes the flow.

## Why `assets/references/`, not submodules

Submodules pin a commit and couple repos at clone/build time — the opposite of the
goal. A throwaway shallow clone in a git-ignored folder gives the content to read with
zero coupling and zero history weight.
