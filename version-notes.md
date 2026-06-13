# Version History

A plain-English history of the project, **one entry per commit, newest first**. Each entry
expands the original commit message into a short narrative of what actually changed and why --
the kind of summary you'd want when skimming the project's evolution, with the raw diff details
left out.

The changelog is **split by month** under [`version-notes/`](version-notes/) so no single page gets
unwieldy. This file is the index; pick a month below (newest first).

## Months

| Month | Commits |
|-------|---------|
| [June 2026](version-notes/2026-06.md) | 4 |
| [March 2020](version-notes/2020-03.md) | 84 |
| [February 2020](version-notes/2020-02.md) | 163 |
| [January 2020](version-notes/2020-01.md) | 216 |
| [December 2019](version-notes/2019-12.md) | 49 |
| [October 2019](version-notes/2019-10.md) | 9 |
| [June 2019](version-notes/2019-06.md) | 71 |

## How this is kept updated

This changelog is maintained **by hand, on request** -- there is no automated script. When you
want it refreshed, just ask and the new commits get written up.

Each entry carries a hidden marker comment with the commit's full hash, e.g.
`<!-- commit: 0123abc... -->`. Those markers are what make a refresh cheap: comparing them
against the git log shows exactly which commits don't have an entry yet. A quick way to list the
undocumented ones (both lists sorted so `comm` lines up):

```
comm -23 <(git rev-list HEAD | sort) \
         <(grep -rhoE 'commit: [0-9a-f]{40}' version-notes.md version-notes/ | awk '{print $2}' | sort -u)
```

Then add an entry for each new commit at the **top of the current month's file** under
`version-notes/` (create `version-notes/YYYY-MM.md`, and add a row to the table above, when the month rolls
over). Newest entries go on top, within the newest month's file.

Entry format:

```
### YYYY-MM-DD -- Short human title
<!-- commit: <full hash> -->
`<short hash>` - Author

One or two paragraphs, in plain English, describing what changed.
```
