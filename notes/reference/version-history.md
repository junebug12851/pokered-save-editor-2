# Version History (the changelog) -- maintenance note

The project keeps a plain-English changelog: **one entry per commit, newest first**, expanding
each commit message into a short narrative of what actually changed (no diff noise). It spans the
whole history -- the 2019 origins, the 2020 library/DB refactor, and the 2026 revival.

## Layout

- [`version.md`](../../version.md) (repo root) is the **index**: the intro, a months table, and
  the maintenance notes.
- [`version/`](../../version/) holds **one file per month**, newest first, e.g.
  `version/2026-06.md`, `version/2020-03.md`, ... down to `version/2019-06.md`. The changelog was
  split by month so no single page got unwieldy (596 entries as of June 2026).

## How it's kept updated

**By hand, on request** -- there is no automated script (an earlier `scripts/version_pending.sh`
was removed; the maintenance is just done manually now). When a refresh is wanted, someone (or an
AI assistant) writes up the new commits.

Each entry carries a hidden marker `<!-- commit: <40-hex> -->`. Those markers are the trick that
keeps a refresh cheap -- diffing them against the git log shows exactly which commits still need
an entry. A quick way to list the undocumented ones (both sides sorted so `comm` lines up):

```
comm -23 <(git rev-list HEAD | sort) \
         <(grep -rhoE 'commit: [0-9a-f]{40}' version.md version/ | awk '{print $2}' | sort -u)
```

For each new commit, `git show -s --format='%an%n%s%n%b' <hash>` + `git show --stat <hash>` gives
the message and file list to write from; pull the full patch with `git show <hash>` when a terse
message needs a closer look. Then add an entry at the **top of the current month's file** under
`version/` (create `version/YYYY-MM.md` and a new row in `version.md`'s table when the month rolls
over).

Entry format:

```
### YYYY-MM-DD -- Short human title
<!-- commit: <full hash> -->
`<short hash>` - Author

One or two paragraphs in plain English. More for big/meaningful commits, a sentence or two for
trivial ones. No diff noise.
```

## Style notes

- Length scales with significance: routine/tiny commits get 1-2 sentences; major ones
  (milestones, big refactors, the revival) get a fuller paragraph.
- Merge commits, README/version bumps, and "fml"-style frustration commits still each get an
  entry -- summarize plainly and keep the human flavor where it's part of the story.
- ASCII only, to match the rest of the codebase.
- The `<!-- commit: ... -->` lines in `version.md`'s own header are illustrative examples (not
  real 40-hex hashes), so a marker scan correctly ignores them.

## Doxygen

`version.md` **and** the `version/` directory are in the Doxyfile `INPUT`, so the changelog builds
into the generated doc site (the index plus a page per month, under Related Pages). The
`<!-- commit: ... -->` markers are HTML comments and don't show up in the rendered output. See
`reference/documentation.md`.
