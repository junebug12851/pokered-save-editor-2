# Version History

A plain-English history of the project, **one entry per commit, newest first**. Each entry
expands the original commit message into a short narrative of what actually changed and why --
the kind of summary you'd want when skimming the project's evolution, with the raw diff details
left out.

The changelog is **split by month** under [`version/`](version/) so no single page gets
unwieldy. This file is the index; pick a month below (newest first).

## Months

| Month | Commits |
|-------|---------|
| [June 2026](version/2026-06.md) | 4 |
| [March 2020](version/2020-03.md) | 84 |
| [February 2020](version/2020-02.md) | 163 |
| [January 2020](version/2020-01.md) | 216 |
| [December 2019](version/2019-12.md) | 49 |
| [October 2019](version/2019-10.md) | 9 |
| [June 2019](version/2019-06.md) | 71 |

## How this is kept updated

**Write the entry as part of the commit it describes -- before committing, not after.** When you
make a change, add its changelog entry to the top of the current month's file and stage it in the
**same commit** as the change. One commit carries both the work and its own entry. This is the
rule that keeps the changelog current by default and avoids a recursive trap: a separate
"document the last commit" commit would itself be an undocumented commit, needing another, forever.

A commit cannot contain its own hash, so **inline entries carry no `<!-- commit: ... -->` marker
and no short-hash line** -- the entry living inside the commit *is* the record. To find the commit
for an entry later, `git blame` the entry's line: because the entry ships with the change, blame
points straight at it.

Inline entry format (newest on top; create `version/YYYY-MM.md` and add a row to the table above
when the month rolls over):

```
### YYYY-MM-DD -- Short human title

One or two paragraphs, in plain English, describing what changed.
```

### Exceptions and the historical backlog

- **Changelog/notes-only maintenance commits are not self-documented.** A commit that just edits
  `notes/version*` (or tidies the notes) doesn't get its own entry -- documenting the documentation
  is the noise this whole scheme avoids.
- **The pre-2026-06-15 entries are after-the-fact** and *do* carry `<!-- commit: <full hash> -->`
  markers plus a `` `short` - Author`` byline, since their commits already existed when written.
  Those markers still let you spot any old commit that was never written up:

  ```
  comm -23 <(git rev-list HEAD | sort) \
           <(grep -rhoE 'commit: [0-9a-f]{40}' version.md version/ | awk '{print $2}' | sort -u)
  ```

  (Inline entries won't appear here -- they have no marker -- which is expected, not a gap.)
