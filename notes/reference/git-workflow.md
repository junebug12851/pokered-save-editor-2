# Git Workflow & Standards

The standards for this repo. The overriding goal: a **clean, faithful, low-risk** history —
"a dull flat repo over a screwed-up one." Established 2026-06-07; moved to the fairyfox
**git-flow** model 2026-06-26 (adopted from `hub/standards/git-workflow.md`).

## The model: full git-flow (policy, not scripts)

This repo follows **git-flow** (Vincent Driessen's branching model) **in full**, as far as
makes sense for a solo project — the branch roles, where work happens, and the release/hotfix
flows are the standard. It does **not** require the `git flow` CLI or any wrapper scripts:
plain `git` carries the whole model, upheld by judgement.

This **replaces** the old lean `dev → main` **fast-forward** habit (2026-06-07 → 2026-06-26),
which was only a thin shadow of git-flow. git-flow is the sturdier starting point.

## Branches

Two long-lived branches and three kinds of short-lived support branch.

- **`main`** — production. Every commit on `main` is a **tagged release**, always reached by
  **`--no-ff` merge** — **never commit directly**. It advances at a release: a **PATCH** goes
  **directly from `dev`**; a **MINOR/MAJOR** goes through a **`release/*`** branch; an urgent
  fix through a **`hotfix/*`** branch. The stable branch is **`main`** (renamed from `master`
  2026-06-08 — `master` is not used).
- **`dev`** — the **integration branch** (git-flow's `develop`; we keep the shorter name).
  All finished work lands here first. **Commit early and often**, even mid-work/broken (Git
  style), with good focused messages. This is the branch the fairyfox hub and sibling projects
  track when syncing.
- **`feature/<name>`** — the normal unit of work. Branch from `dev`, build it, merge **back into
  `dev`** with `--no-ff`, then delete. Never branch a feature off `main`.
- **`release/<x.y.0>`** — the mechanism for a **MINOR or MAJOR** release. Branch from `dev` to
  bake the milestone (final polish, `VERSION` bump, changelog), merge **into `main`** + tag
  **and** back **into `dev`** (`--no-ff` both), then delete. **PATCH releases skip this.**
- **`hotfix/<x.y.z>`** — an urgent production fix. Branch from `main`, fix, merge **into `main`**
  + tag **and** back **into `dev`** (`--no-ff` both), then delete.

Support-branch names use a **`type/` prefix** (`feature/`, `release/`, `hotfix/`) + a short
kebab-case description.

### Solo / small-project latitude

git-flow assumes a team; this project is solo, so one piece of judgement applies *within* the
model: a genuinely **trivial** change — a typo, a one-line doc fix, a notes update — may be
committed **directly on `dev`** rather than via a `feature/*` branch. Anything that is really
"a feature," or is large/risky, still gets its own branch. The **release path is not latitude**
— it's fixed by the SemVer level below.

## Developing a feature

```sh
git checkout dev
git checkout -b feature/<name>
# … commit work on the feature branch; push it to back it up …
git checkout dev
git merge --no-ff feature/<name>     # keeps the feature as one revertible unit
git branch -d feature/<name>
git push origin dev
```

The `--no-ff` merge commit groups the feature's commits under one parent, so the feature stays
legible in history and revertible in one move (`git revert -m 1 <merge>`).

## Cutting a release — the path is set by the SemVer level

This project ships releases through `release.yml` (see `notes/reference/deployment.md`), which
fires on every push to `main` and publishes a GitHub Release when `VERSION` was bumped.

> **Project divergence (recorded on purpose): the release tag is created by CI, not by hand.**
> The hub runbook ends a release with a manual `git tag -a vX.Y.Z && git push --tags`. Here the
> `release.yml` pipeline **owns tagging** — its `version` job derives `v<VERSION>` and
> `softprops/action-gh-release` creates the tag on the merge commit. So **do NOT manually tag**:
> a hand-pushed tag would make the `version` job see the tag already exists and **skip the
> release**. The git-flow *intent* is fully met (every `main` commit is a `--no-ff` merge
> carrying a matching `v<VERSION>` tag); only *who* applies the tag differs, and the project owns
> that rendering. This is the one deliberate departure from the hub runbook's step 5.

- **PATCH** (the default — fixes, ordinary changes): release **directly** `dev → main`, no
  release branch.

  ```sh
  git checkout main
  git merge --no-ff dev               # merge commit; CI tags v<VERSION> + publishes the release
  git push origin main
  git checkout dev
  ```

- **MINOR / MAJOR** (a milestone): go through a **`release/X.Y.0`** branch so the milestone is a
  deliberate, reviewable event. (MAJOR → `1.0.0` is the project owner's — Twilight's — call only.)

  ```sh
  git checkout dev
  git checkout -b release/X.Y.0
  # … finalize: bump VERSION, finish the changelog entry, last polish …
  git checkout main
  git merge --no-ff release/X.Y.0     # CI tags v<VERSION> + publishes
  git push origin main
  git checkout dev
  git merge --no-ff release/X.Y.0     # carry the finalizations back to dev
  git branch -d release/X.Y.0
  git push origin dev
  ```

## Hotfixes

A production problem that can't wait for the next `dev` cycle is fixed on a branch cut from
`main`, then folded back into both lines:

```sh
git checkout main
git checkout -b hotfix/X.Y.Z
# … fix, bump VERSION (patch), changelog …
git checkout main
git merge --no-ff hotfix/X.Y.Z       # CI tags + publishes
git push origin main
git checkout dev
git merge --no-ff hotfix/X.Y.Z
git branch -d hotfix/X.Y.Z
git push origin dev
```

### Windows gotcha: checking out pre-`dab0a1d` history collides with `VERSION`

Before commit `dab0a1d` (2026-06-13) the version file was `VERSION.txt` and the changelog folder
was `version/`. That commit renamed them to `VERSION` and `version-notes/` (the changelog was
later moved into `notes/` and renamed `notes/version/` on 2026-06-14 — a subdirectory, so it no
longer collides with the root `VERSION`). Because the Windows filesystem is **case-insensitive**,
the working-tree file `VERSION` and an old commit's tracked directory `version/` are the *same
path*, so `git checkout <pre-rename-ref>` aborts with *"untracked working tree files would be
overwritten: version"*. It only bites when checking out / bisecting **old** history. Workarounds:
fast-forward a ref **without** a working-tree switch — `git fetch . dev:main` — or temporarily
move `VERSION` aside before the historical checkout. (See `reference/versioning.md`.)

## Merging — `--no-ff`, never rewrite

git-flow merges with `--no-ff`: features back into `dev`; `release/`/`hotfix/` branches into both
`main` and `dev`; and a PATCH release `dev → main`. Each creates a merge commit, so the grouping
stays legible and revertible as a unit. **Every merge into `main` is a tagged release.** This is
all **additive** — it never rewrites history. Do **not** squash, rebase, reorder, or amend
anything already pushed; every original commit is preserved through every merge.

## Pushing — push every commit

**Push on every commit**, early and often — don't leave work only on the local machine. All
sessions keep the remote current, just like the living notes.

> **Standing default (2026-06-10):** commit/push on `dev` AND the green-gated release to `main`
> are **fully automatic — done by default without being asked**. This supersedes the older
> "push only when asked" phrasing; the *only-when-asked* restriction applies just to the
> genuinely destructive ops (force-push, history rewrite, `reset --hard`, `rebase`, `clean -fd`,
> long-lived-branch deletion), which remain off-limits without an explicit request. The full
> default loop lives in `CLAUDE.md` → "Default Workflow — Do These By Default."

- **`dev`**: `git push origin dev` after each commit (and push `feature/*` branches as you go, to
  back them up).
- **`main`**: advanced only by a `--no-ff` release merge after a **green** checkpoint ("green" =
  builds + full `ctest` pass, and the remote `tests` CI is green). `git push origin main` then
  triggers `release.yml` (which tags + publishes). Never commit on `main` directly.

## No history rewriting

Do **not** rebase/squash/reorder/amend already-pushed commits to "tidy up." It's both the #1
corruption risk and what destroys faithfulness. The discipline lives in writing good commits up
front, not in after-the-fact surgery. (git-flow's `--no-ff` merge commits are additive and
allowed — they are not a rewrite.)

## Commit messages

- One **logical, focused** change per commit (not broad "misc" commits).
- Imperative, concise, structured: `type: summary` — `feat:`, `fix:`, `refactor:`, `docs:`,
  `build:`, `test:`, `chore:`, `content:`, `style:`. Short body when the *why* isn't obvious.
- **Changelog rides inside the commit** (see the versioning + notes-system standards): write the
  entry, stage it in the same commit. No separate "document the last commit" commits.
- **Keep `VERSION` current** as part of the release — bumped on `dev` for a PATCH, or on the
  `release/*`/`hotfix/*` branch for a milestone/hotfix; the release tag on `main` matches it.

## Hard safety rules (to avoid past repo corruption)

- **Never** `push --force`, force-with-lease, or rewrite pushed history. (git-flow's `--no-ff`
  merge commits are additive and allowed.)
- **Never** `reset --hard`, `rebase`, `clean -fd`, or delete a **long-lived** branch
  (`main`/`dev`) **without an explicit request**. Spent `feature/`/`release/`/`hotfix/` branches
  are deleted as the normal end of their merge.
- **Stage specific files**, never `git add -A`/`.` — build artifacts stay out (gitignored:
  `build/`, `projects/build/`, `*.dll`, `*.exe`, `*.o`, `*.moc`, and `assets/references/`).
- Inspect `git status` before, verify clean/expected after — every time.

## Verify (is it being followed?)

The check that catches a violation — run on request, report `done`/`partial`/`missing` (the
per-standard slice the fairyfox [compliance audit](compliance.md) aggregates):

| Passes only when… | How to check |
|-------------------|--------------|
| Stable branch is **`main`**, not `master` | `git branch -a` |
| Every commit on `main` is a `--no-ff` **release merge** carrying a matching `vX.Y.Z` tag — no direct commits | `git log --first-parent --oneline main`; `git tag` |
| Pushed history is intact — no force-push / rebase / reset of published commits | history stable across fetches; no `--force` in reflog |
| Spent `feature/`/`release/`/`hotfix/` branches deleted; `main`/`dev` intact | `git branch -a` |
| Each release to `main` rode a green build/test checkpoint (local `ctest` + remote `tests` CI) | release followed a green check |

## Remote setup state (complete 2026-06-08)

Done: `master` renamed to `main` locally; `dev` created; both pushed to `origin`; GitHub default
branch set to `main`; stale `origin/master` deleted (verified **0** unique commits on it first,
with a guard). Remote has exactly **`main`** (stable) + **`dev`**; `origin/HEAD → origin/main`.
Repo: `github.com/junebug12851/pokered-save-editor-2`. No commits were lost in the rename. (The
`master → main` rename git-flow mandates was therefore already done long before this repo adopted
git-flow.)
