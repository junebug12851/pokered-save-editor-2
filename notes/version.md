# Changelog (Version History)

A plain-English history of the project, **one entry per commit, newest first**. Each entry expands the
original commit message into a short narrative of what actually changed and why — the kind of summary
you'd want when skimming the project's evolution, with the raw diff details left out. It spans the
whole history: the 2019 origins, the 2020 library/DB refactor, and the 2026 revival.

> **Not to be confused with [`reference/versioning.md`](reference/versioning.md).** *This* file (and
> `version/`) is the **changelog** — the narrative of what changed, per commit. `versioning.md` is the
> **version-number scheme** (SemVer, the `VERSION` file, how `0.7.0-alpha` propagates into the build).
> One is the story; the other is the label.

The changelog is **split by month** under [`version/`](version/2026-06.md) so no single page gets unwieldy. This
file is the index; pick a month below (newest first).

## Months

| Month | Commits |
|-------|---------|
| [July 2026](version/2026-07.md) | 1 |
| [June 2026](version/2026-06.md) | 4 |
| [March 2020](version/2020-03.md) | 84 |
| [February 2020](version/2020-02.md) | 163 |
| [January 2020](version/2020-01.md) | 216 |
| [December 2019](version/2019-12.md) | 49 |
| [October 2019](version/2019-10.md) | 9 |
| [June 2019](version/2019-06.md) | 71 |

## How this is kept updated (the inline rule)

**Write each entry as part of the commit it describes — before committing, not after** (adopted
2026-06-14). When you make a change, add its entry to the top of the current month's file under
`version/` and stage it in the **same commit** as the change. One commit = the work plus its own
changelog entry. There is no automated script and no separate "update the changelog" commit.

This is a deliberate root-cause fix. The old model wrote entries after the fact and tied each to a
`<!-- commit: <40-hex> -->` marker; keeping it current then meant a follow-up commit to document the
prior commits — but that follow-up is itself an undocumented commit, which needs another, forever.
Folding the entry into the same commit removes the recursion entirely.

A commit cannot contain its own hash, so **inline entries carry no marker and no short-hash byline**.
To find the commit for an entry, `git blame` the entry's line — since the entry ships inside the
change, blame lands exactly on it.

Inline entry format (newest on top; create `version/YYYY-MM.md` and add a row to the table above when
the month rolls over):

```
### YYYY-MM-DD — Short human title

One or two paragraphs in plain English. More for big/meaningful commits, a sentence or two for trivial
ones. No diff noise.
```

### Exceptions and the historical backlog

- **Changelog/notes-only maintenance commits are not self-documented.** A commit that just edits
  `notes/version*` (or otherwise tidies the notes) doesn't get its own entry — documenting the
  documentation is the noise this whole scheme avoids.
- **Pre-2026-06-15 entries are after-the-fact** and *do* carry `<!-- commit: <full hash> -->` markers
  plus a `` `short` - Author`` byline, since their commits already existed when written. Those markers
  still let you spot any old commit that was never written up (inline entries are markerless and won't
  appear, which is expected, not a gap):

  ```
  comm -23 <(git rev-list HEAD | sort) \
           <(grep -rhoE 'commit: [0-9a-f]{40}' version.md version/ | awk '{print $2}' | sort -u)
  ```

  For an old commit, `git show -s --format='%an%n%s%n%b' <hash>` + `git show --stat <hash>` gives the
  message and file list to write from.

## Style notes

- Length scales with significance: routine/tiny commits get 1–2 sentences; major ones (milestones, big
  refactors, the revival) get a fuller paragraph.
- Merge commits, README/version bumps, and "fml"-style frustration commits still each get an entry —
  summarize plainly and keep the human flavor where it's part of the story.
- ASCII only, to match the rest of the codebase.
- The `<!-- commit: ... -->` lines in this file's own header examples are illustrative (not real
  40-hex hashes), so a marker scan correctly ignores them.

## Doxygen

The changelog lives under `notes/`, which is in the Doxyfile `INPUT` (recursive), so it builds into the
generated doc site (this index plus a page per month, under Related Pages). The
`<!-- commit: ... -->` markers are HTML comments and don't show up in the rendered output. See
[`reference/documentation.md`](reference/documentation.md).

## Relationship to the session logs

The changelog is **one entry per git commit**; the [session logs](sessions/README.md) are **one entry per
working day**, broader than any single commit. The changelog is the commit-by-commit record; the
session log is the day-by-day story. They overlap but serve different readers.
