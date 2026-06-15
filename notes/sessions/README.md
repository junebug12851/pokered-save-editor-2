# Session Logs {#pse_sessions_about}

The running history of the project — **what changed each working session and why.** One file per
calendar day, grouped into **month folders**: `sessions/YYYY-MM/YYYY-MM-DD.md` (e.g.
`sessions/2026-06/2026-06-14.md`).

`../status.md` holds the *current* state (build/runtime health, open issues). These files hold the
*story* of how it got there. Deep root-cause mechanics live in `../reference/` (especially
`qt-patterns.md`) and `../decisions/`; the session logs are the narrative and the pointer.

## The system (how to keep this updated)

**Each working day gets one file: `sessions/YYYY-MM/YYYY-MM-DD.md`.** When you do work worth
recording, append an entry to *today's* file (create it — and the `YYYY-MM/` month folder, if it's a
new month — if it's the first entry of the day). Within a day, newest entry on top. This is the
living history — keep it current by default, the same way the [changelog](../version.md) is kept
current.

Day file skeleton:

```markdown
# 2026-06-15 — Session Log

## <short title of the change> — <one-line outcome>

What changed, the root cause if it was a bug, the files touched, the test/verification result,
and any follow-up. Keep the technical specifics (file names, root causes, test names) — those are
the valuable part. Plain English, no diff noise.
```

### Conventions

- **One file per day**, `YYYY-MM-DD.md`, inside a **`YYYY-MM/` month folder**. Both the folders and
  the files sort chronologically on their own. (The full date is kept in the filename even inside the
  month folder, so a file is self-identifying on its own.)
- **Newest entry on top** within a day; **newest day** is the highest-numbered file in the newest
  month folder.
- **Cross-link, don't duplicate.** Root-cause depth → `../reference/qt-patterns.md` /
  `../decisions/`. Current health/open issues → `../status.md`. The session log says *what happened
  and points at the detail.*
- **Voice:** plain and matter-of-fact, written from the project's perspective. Names only where attribution matters; prefer neutral phrasing ("agreed"/"decided") over singling anyone out.
- When a fix or pattern is worth reusing, also add it to the right reference file
  (`../reference/fix-patterns.md`, `qt-patterns.md`, `ui-patterns.md`, etc.) — the session log
  records *that it happened*, the reference file records *the reusable lesson*.

### Relationship to the changelog

The [changelog](../version.md) (`version/YYYY-MM.md`) is **one entry per git commit**, written
*inside* the commit. The session log is **one entry per working session**, broader than any single
commit (a session often spans several commits, or none yet). They overlap but serve different
readers: the changelog is the commit-by-commit record; the session log is the day-by-day story.

## History before this system

The per-day system was adopted **2026-06-14**. The pre-corruption 2026 revival work was tracked as
numbered sessions (`s13`, `s13b` … `s13z11`, `s14`) with no reliable calendar dates — that work was
never committed (HEAD was the corrupted commit until the 2026-06-06 recovery). That narrative is
preserved verbatim in [`revival-s13-series.md`](revival-s13-series.md). Dated work from the
2026-06-06 recovery onward lives in the per-day files.
