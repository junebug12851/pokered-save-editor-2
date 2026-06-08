# Git Workflow & Standards

Twilight's standards for this repo. The overriding goal: a **clean, faithful, low-risk** history —
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

## Hard safety rules (to avoid the repo corruption Twilight has hit before)

- **Never** `push --force`, force-with-lease, or rewrite pushed history.
- **Never** `reset --hard`, `rebase`, `clean -fd`, or delete a branch **without an explicit request**.
- Only routine ops: `status`, `log`, `add <specific paths>`, `commit`, `branch`, `checkout`,
  `merge --ff-only`, and `push` (push only when asked).
- **Stage specific files**, never `git add -A`/`.` — build artifacts stay out (also gitignored:
  `build/`, `projects/build/`, `*.dll`, `*.exe`, `*.o`, `*.moc`).
- Inspect `git status` before, verify clean/expected after — every time.

## Remote setup state (2026-06-07)

Local `master` was renamed to `main`; `dev` created. **Remote not yet reconciled** — `origin` still has
`master` as default. To finish (do with care, Twilight to approve each push):
1. `git push -u origin main` and `git push -u origin dev` (additive — safe).
2. On the Git host's web UI, set the **default branch to `main`**.
3. Then `git push origin --delete master` (only works once it's no longer the default).
