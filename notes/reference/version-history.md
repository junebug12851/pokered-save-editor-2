# Version History (the changelog) -- maintenance note

The project keeps a plain-English changelog: **one entry per commit, newest first**, expanding
each commit message into a short narrative of what actually changed (no diff noise). It spans the
whole history -- the 2019 origins, the 2020 library/DB refactor, and the 2026 revival.

## Layout

- [`version.md`](../version.md) (in `notes/`) is the **index**: the intro, a months table, and
  the maintenance notes.
- [`version/`](../version/) holds **one file per month**, newest first, e.g.
  `version/2026-06.md`, `version/2020-03.md`, ... down to `version/2019-06.md`. The changelog was
  split by month so no single page got unwieldy (596 entries as of June 2026).

## How it's kept updated (the inline rule)

**Write each entry as part of the commit it describes -- before committing, not after** (adopted
2026-06-14). When you make a change, add its entry to the top of the current month's file under
`version/` and stage it in the **same commit** as the change. One commit = the work plus its own
changelog entry. There is no automated script and no separate "update the changelog" commit.

This is a deliberate root-cause fix. The old model wrote entries after the fact and tied each to a
`<!-- commit: <40-hex> -->` marker; keeping it current then meant a follow-up commit to document
the prior commits -- but that follow-up is itself an undocumented commit, which needs another, and
so on. Folding the entry into the same commit removes the recursion entirely.

A commit can't contain its own hash, so **inline entries carry no marker and no short-hash byline**.
To find the commit for an entry, `git blame` the entry's line -- since the entry ships inside the
change, blame lands exactly on it. Inline format (newest on top; create `version/YYYY-MM.md` and a
new row in `version.md`'s table when the month rolls over):

```
### YYYY-MM-DD -- Short human title

One or two paragraphs in plain English. More for big/meaningful commits, a sentence or two for
trivial ones. No diff noise.
```

**Two exceptions / the backlog:**

- A commit that only edits `notes/version*` (or otherwise just maintains the notes) is **not
  self-documented** -- documenting the documentation is the noise this avoids.
- Entries **before 2026-06-15** were written after the fact and still carry
  `<!-- commit: <40-hex> -->` + a `` `short` - Author`` byline (their commits already existed). To
  catch any old commit never written up (inline entries are markerless and won't appear, which is
  expected):

  ```
  comm -23 <(git rev-list HEAD | sort) \
           <(grep -rhoE 'commit: [0-9a-f]{40}' version.md version/ | awk '{print $2}' | sort -u)
  ```

  For an old commit, `git show -s --format='%an%n%s%n%b' <hash>` + `git show --stat <hash>` gives
  the message and file list to write from.

## Style notes

- Length scales with significance: routine/tiny commits get 1-2 sentences; major ones
  (milestones, big refactors, the revival) get a fuller paragraph.
- Merge commits, README/version bumps, and "fml"-style frustration commits still each get an
  entry -- summarize plainly and keep the human flavor where it's part of the story.
- ASCII only, to match the rest of the codebase.
- The `<!-- commit: ... -->` lines in `version.md`'s own header are illustrative examples (not
  real 40-hex hashes), so a marker scan correctly ignores them.

## Doxygen

The changelog lives under `notes/`, which is in the Doxyfile `INPUT` (recursive), so the changelog builds
into the generated doc site (the index plus a page per month, under Related Pages). The
`<!-- commit: ... -->` markers are HTML comments and don't show up in the rendered output. See
`reference/documentation.md`.
