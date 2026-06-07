# Doc-Comment Style (house rules)

The conventions every documentation pass in this project follows, so the comments
read as one consistent voice. Paired with the Doxygen setup in
[documentation.md](documentation.md).

## Where the comment goes

- **API docs live in the header** (`.h`), on the declaration. Doxygen associates
  the comment with the next declaration, so the comment stays human and never
  restates the signature.
- **Implementation files** (`.cpp`) get a short `@file` line at the top and then
  ordinary `//` notes inside bodies explaining *why* a step is done. They do NOT
  repeat the header's `/** */` blocks (that would double-document).
- Every file opens (after the licence header) with a one-line `@file` + `@brief`.

## Syntax

- Multi-line API doc: `/** ... */`. One-liner: `///`. Trailing member doc: `///<`.
- `QT_AUTOBRIEF`/`JAVADOC_AUTOBRIEF` are on, so **the first sentence is the brief** --
  no explicit `@brief` needed (though it's fine to use for emphasis).
- Common tags: `@param`, `@return`, `@note`, `@see`, `@ref`, `@c code`, `@e emphasis`.

## Voice

- Describe **purpose and role**, not the obvious. "Process-wide source of
  randomness" beats "the Random class".
- Call out the non-obvious: ownership, singletons, Qt 6 gotchas, byte-fidelity
  implications, why a guard exists. Link related types with `@see`.
- Depth setting for this project (Twilight's call): **document everything**,
  including trivial getters/setters and private members -- but keep each line
  genuinely informative rather than padding.

## Preserving Twilight's existing comments (hard rule)

- **Never delete an existing comment written by Twilight.** If it overlaps a new
  Doxygen block, *merge* its content into the block (keep the meaning/wording);
  otherwise leave it exactly where it is, untouched.
- Adding new explanatory `//` notes on the code is **encouraged** -- as the
  architecture is mapped out, record the non-obvious "why" inline next to the code,
  not just in the headers. These ride alongside the Doxygen API docs.

## Hard rules

- **ASCII only** in comments. No em/en dashes, arrows, or `>=` glyphs -- use
  `--`, `->`, `>=`. The codebase is ASCII; non-ASCII also tripped a write/encoding
  issue once (see verification note below).
- **Comments only.** A documentation pass must never change a single line of code.
  Verify this every time (below).
- Preserve the existing licence header verbatim at the top of every file.

## Verifying a pass is comments-only

Strip comments from the old and new versions and diff the remaining code:

```
strip() { gcc -fpreprocessed -dD -E "$1" | grep -v '^#' \
          | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//' | grep -v '^$'; }
git show HEAD:"$f" > /tmp/orig; strip /tmp/orig > /tmp/a; strip "$f" > /tmp/b
diff /tmp/a /tmp/b   # empty == comments-only
```

**Caveat (important):** the Cowork bash sandbox sometimes serves a *stale,
falsely-truncated* view of a file right after it's written, which makes the diff
above report bogus "code removed" lines. When that happens, trust the **Read
tool** (it shows true on-disk state), not bash. Re-run the bash check in a later
turn once the mount catches up. See the memory note "Bash mount stale reads".
