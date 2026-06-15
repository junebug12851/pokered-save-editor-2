# Git Workflow & Standards

The standards for this repo. The overriding goal: a **clean, faithful, low-risk** history —
"a dull flat repo over a screwed-up one." Established 2026-06-07.

## Branches

- **`main`** — stable, releasable, **pushed**. In early development "stable" just means *builds + tests
  green*, not a formal release. **Never commit directly to `main`.** It only ever moves by
  **fast-forward** to a green `dev` commit.
- **`dev`** — the development branch. Commit **early and often**, even mid-work/broken (Git style).
  Good focused messages even here.
- **No feature branches** for now (solo dev, early stage — they add merge overhead for little gain).
  Add one only to isolate a genuinely large/risky change, then FF/merge it back and delete it.

## Merging — fast-forward only (this resolves the "clean vs faithful" question)

The clean-vs-faithful tension only exists when branches **diverge**. Because we **never commit on
`main` directly**, `dev` is always strictly ahead, so `dev → main` is always a **fast-forward**:

```
git checkout main
git merge --ff-only dev      # linear, no merge commit, every dev commit preserved
```

This gives a clean *linear* history **and** keeps every original commit/message faithfully — so we do
**not** squash, rebase, or reorder. (Tradeoff: `main`'s history includes `dev`'s intermediate commits.
Fine at this stage; if pristine release history is wanted later, tag releases or switch to
squash-merges then — not now.)

### Windows gotcha: checking out pre-`dab0a1d` history collides with `VERSION`

Before commit `dab0a1d` (2026-06-13) the version file was `VERSION.txt` and the changelog folder was
`version/`. That commit renamed them to `VERSION` and `version-notes/` (the changelog was later moved into `notes/` and renamed `notes/version/` on 2026-06-14 — a subdirectory, so it no longer collides with the root `VERSION`). Because the Windows filesystem
is **case-insensitive**, the working-tree file `VERSION` and an old commit's tracked directory `version/`
are the *same path*, so `git checkout <pre-rename-ref>` aborts with *"untracked working tree files would
be overwritten: version"*. Going forward this never bites a normal `dev → main` FF (both have `VERSION` +
`notes/version/`); it only happens when checking out / bisecting **old** history. Workarounds: fast-forward
a ref **without** a working-tree switch — `git fetch . dev:main` (FF-only; safely updates local `main`
off-branch) — or temporarily move `VERSION` aside before the historical checkout. (This case collision is
exactly why the file was originally suffixed `.txt`; renaming `version/` → `version-notes/` is what freed
the bare `VERSION` name. See `reference/versioning.md`.)

## Pushing — push every commit (added 2026-06-08)

**Push on every commit**, same as commits themselves: early and often. All chat tabs/sessions are
expected to keep the remote current, just like they keep the living notes current — don't leave green
work sitting only on the local machine.

> **Standing default (2026-06-10):** commit/push on `dev` AND the green-gated `main` fast-forward are
> now **fully automatic — done by default without being asked** (by design explicit instruction). This
> supersedes the "push only when asked" phrasing in the Hard safety rules below; the *only-when-asked*
> restriction now applies just to the genuinely destructive ops (force-push, history rewrite,
> `reset --hard`, `rebase`, `clean -fd`, branch deletion), which remain off-limits without an explicit
> request. The full default loop (build+launch, test, debug/profile, commit/push/FF) lives in
> `CLAUDE.md` → "Default Workflow — Do These By Default."

- **`dev`**: `git push origin dev` after each commit. (This environment appears to push `dev` on commit
  automatically — `origin/dev` was already current after local commits — but push explicitly anyway;
  it's a harmless no-op when already in sync.)
- **`main`** (sometimes called "master" out of habit — the branch was renamed master→main on
  2026-06-08; the stable branch is **`main`**): after a green checkpoint, fast-forward it to the latest
  green `dev` and push:
  ```
  git checkout main && git merge --ff-only dev && git push origin main && git checkout dev
  ```
  `main` = "the latest `dev` that compiles and tests green," so only FF it to a commit you've actually
  built + run `ctest` on (all green). Never commit on `main` directly; FF-only (see Merging above).
- Safety unchanged: only ever `git push` / `git push origin main` (FF). **Never** `push --force` /
  force-with-lease / rewrite pushed history.

## No history rewriting

Do **not** rebase/squash/reorder/amend already-pushed commits to "tidy up." It's both the #1 corruption
risk and what destroys faithfulness. The discipline lives in writing good commits up front, not in
after-the-fact surgery.

## Commit messages

- One **logical, focused** change per commit (not broad/generalized "misc" commits).
- Imperative, concise, structured: `type: summary` — types e.g. `tests:`, `fix:`, `refactor:`,
  `docs:`, `build:`. Add a short body when the *why* isn't obvious.
- Commit a batch only after it **builds and the suite is green** (for `dev`, broken WIP commits are
  allowed but prefer green; for `main`, always green).

## Hard safety rules (to avoid past repo corruption)

- **Never** `push --force`, force-with-lease, or rewrite pushed history.
- **Never** `reset --hard`, `rebase`, `clean -fd`, or delete a branch **without an explicit request**.
- Only routine ops: `status`, `log`, `add <specific paths>`, `commit`, `branch`, `checkout`,
  `merge --ff-only`, and `push`. (Commit/push/FF-main are now done **by default** — see the Pushing
  section's "Standing default" note above; the destructive ops in the first two bullets still require an
  explicit request.)
- **Stage specific files**, never `git add -A`/`.` — build artifacts stay out (also gitignored:
  `build/`, `projects/build/`, `*.dll`, `*.exe`, `*.o`, `*.moc`).
- Inspect `git status` before, verify clean/expected after — every time.

## Remote setup state (complete 2026-06-08)

Done: `master` renamed to `main` locally; `dev` created; both pushed to `origin`; GitHub default branch
set to `main`; stale `origin/master` deleted (verified **0** unique commits on it first, with a guard).
Remote now has exactly **`main`** (stable) + **`dev`**; `origin/HEAD → origin/main`. Repo:
`github.com/junebug12851/pokered-save-editor-2`. No commits were lost in the rename.
