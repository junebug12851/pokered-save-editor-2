# Project Principles and Will

The convictions behind the project — what it should be, what to avoid,
and the constraints that matter. These should inform every decision.

---

## What This Is To the Owner

This is **Twilight's most prized coding project of her career** — built solo, meticulously, line by
line. It matters deeply and personally: Twilight is autistic and has a lifelong obsession with this
game, and the project is an expression of that. It was abandoned not from lack of care but from the
complexity growing past what one person could sustain alone. The revival in 2026 is a genuine
second chance to finish something important. Treat it that way — the standards here are higher than
a normal app, on purpose.

**The defining ambition:** a beautiful, modern, capable editor that exposes **every byte** of the
save file in a pretty GUI — not a hex table, an actual designed interface for all of it. Twilight has
never seen another editor do this, and that completeness-with-polish is the point of the project.

---

## UX Is the Prime Directive

**UX is the single most important thing in this project — explicitly #1, above cost and effort.**
Twilight (s13k): _"its so absolutely #1 that i will go the long route on something just because the ux
wouldn't allow a cheaper way... the ux absolutely needs to flow and work very well and smoothly."_

What this means in practice:
- A cheaper implementation that compromises the *feel* is the wrong answer here, even if it's
  "good enough" functionally. Do not propose the cheap path as the recommendation when it leaves a
  UX seam — name the seam and propose the path that flows.
- Twilight is **not** attached to most specific UX *decisions* (she'll weigh options), but she is
  absolutely attached to the *outcome*: smooth, coherent, never clunky or confusing.
- "Solves the UI but leaves UX out" is a failure mode to watch for — e.g. a fix that works
  mechanically but introduces a janky interaction, a moving/clickable artifact, or a mode the user
  has to think about. Reject those.
- When two approaches differ mainly in build cost, lead with the one that gives the better
  interaction and be honest that it's the longer route.

## Save File Integrity Is Sacred (top-tier priority)

**Not corrupting save files is among the highest priorities in the project — avoid it at all costs,
always, if at all humanly possible.** The app's signature property: it changes **only the exact
bytes it was instructed to change and leaves every other byte of the save completely and totally
untouched** — something hardly any editor does. Twilight verified this by hand over dozens of hours of
micro- and macro-level testing, and is proud enough of it to brag about it in the README.

Hard rules that follow from this (also in top-level `CLAUDE.md`):
- Never rewrite, normalize, reorder, or repack the whole save. Touch only the targeted bytes.
- Never alter checksums, padding, or regions you weren't explicitly asked to change.
- When in doubt about whether a write is in-scope, **don't** — confirm first. A corrupted save is
  one of the worst outcomes this project can produce.
- Any change to save read/write paths deserves extra scrutiny and verification against this rule.

## The Quality Bar (No Hacks)

Twilight's standard, stated plainly: **no garbage hacks, no temporary fixes, no low-quality stuff, no
bad fallbacks, no interrupting the UX — not even a tiny bit of room for it.** Outside of UX flow she
expects very high quality and essentially no runtime errors.

Working implications:
- Prefer the correct, clean, durable solution even when it's the longer route (see "UX Is the Prime
  Directive"). The cheap-but-seamful option is the wrong answer here.
- If the only path you can see is a hack/temporary patch, **surface it and ask** rather than commit
  it. Don't quietly leave a TODO-grade fix in this codebase.
- "Works mechanically" is not the bar; "works and feels right and won't bite later" is.
- Accessibility