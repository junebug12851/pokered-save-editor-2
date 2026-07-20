# Project Principles

The convictions behind the project — what it should be, what to avoid,
and the constraints that matter. These should inform every decision.

---

## The Goal

Let the user edit **every bit and byte of a Red/Blue save** through a clean, simple, stable, and
dependable GUI — and make that same app scale smoothly across the whole range of use. At the quick,
casual end: re-roll a name in seconds, tweak money or badges. At the deep, advanced end:
import/export the raw `.bin` (including the normally-unused bytes), work with map state, and do
assisted code injection into the save. Lightweight and easy on the surface; comprehensive and solid
underneath.

---

## Background

An open-source Pokémon Red & Blue save editor. The first version was shelved around 2020 when its
complexity outgrew what the project could sustain; it was revived in 2026. The constraints below are
what keep it focused.

---

## Core Philosophy: Graceful Degradation

The app must never:
- Crash or freeze unexpectedly
- Trap the user behind a blocking error dialog in production (release builds)
- Leave the user stuck with no way forward

This is **not** about hiding errors or pretending everything is fine. It's about handling failures
with care: catch even an otherwise-fatal problem, present it through a clean, clear UI, degrade the
affected feature, and — where it's possible — offer a way to keep going (e.g. reloading what failed)
instead of crashing or stranding the user. Debug builds surface problems to the developer in detail;
release builds handle them gracefully and clearly. Above all, a failure must never put save data at
risk — degradation is always preferred over any chance of corruption.

---

## What the App Should Feel Like

**Polished desktop software, not a hacker tool.**

The user should be able to open a save file and immediately have fun. The app should:
- Present data in a way that's intuitive without needing documentation
- Make the fun stuff easy to find (randomize, edit name, see your team)
- Guide the user rather than dump raw data at them
- Feel like something that belongs in their desktop application folder

---

## Every Byte, None of Them Raw

_project leadership, 2026-07-12. A top-tier rule, and it has two halves that only work together._

**1. Every byte of the save must be editable.** Nothing is withheld because it looks obscure,
transient, or "not really data the user needs". If the save file holds it, the app lets you change
it. This is a save *editor*; a byte we refuse to expose is a byte we have decided the user isn't
allowed to own.

**2. And none of them may be raw.** *"Users typically don't find raw fields very useful or
interesting."* A byte is never a spin box with a number in it. Work out what the byte actually **is**
and build the control for **that thing**:

| The byte is… | So the control is… |
|---|---|
| a tile id | a picker of the actual, rendered tiles |
| a block id | a picker of the actual blocks |
| a sprite slot | the actual sprites, by name and picture |
| a ROM pointer | a **named** choice — "the collision list Mart uses" |
| a flag with a real-world meaning | that meaning, in words, as a switch |
| a byte doing two jobs | an editor that says **both** jobs out loud |

**The escape hatch is what reconciles the two.** Every one of those pickers carries a **Custom…**
option that can still reach *any* value the byte can hold — including the nonsense ones, because a
glitch hunter is a legitimate user and the save is theirs. The intuitive choice is the **default**;
the arbitrary value is always still **reachable**; and when the value disagrees with what the
cartridge would have there, we **say so** and never silently rewrite it (the same doctrine as the
music bank and the glitch palettes).

So: no field is hidden, and no field is a number box. If a byte seems to have no meaningful editor,
that means the research isn't finished — not that the field should be dropped.

---

## The Randomization Feature — What It Must Be

This is the most ambitious feature and must be done right. Constraints:

**Non-negotiable goals:**
- The randomized save must be **playable** — the game should not crash after loading it
- The player should be able to take their first step and navigate
- Must include an HM-capable Pokemon somewhere in the team (escape slave)
- Wild Pokemon encounters must be balanced (no instant-kill level 100s)

**Intentional constraints (not arbitrary):**
- No glitch items, no glitch Pokemon
- No beaten trainers, no completed events — clean start
- Money, items, and team should feel like "starting conditions" not cheats
- Maps must have valid warps — player must be able to leave

**The spirit of it:** You should laugh when you see what you got. Curious, surprising,
but never immediately broken. The fun is discovery within a playable sandbox.

**Randomization is a growing system, not one fixed feature.** Today's basics (a quick name re-roll and
an early new-game randomizer) are just the foundation. The planned system is built around three modes:
**Constrained Random** (randomized but kept within sensible, playable bounds), **Unconstrained Random**
(anything that still loads — chaotic by design), and **Synthetic Natural** (a generated save crafted to
look like one genuinely earned by playing the real game). Don't describe randomization as if the simple
constrained version is all there is.

---

## Save File Integrity Is Sacred

**Bit- and byte-exact fidelity has been a core value since day one.** The editor changes *only* the
exact bits and bytes an edit needs and leaves every other bit and byte of the save totally untouched
— even unused or unallocated bits are treated as precious; a single unintended bit flip is
unacceptable. No rewriting or "normalizing" the whole file, no reordering/repacking, no touching
checksums or regions you weren't told to change. Corrupting a save is among the worst possible
outcomes.

This isn't a revival-era rule bolted on; it's why the **expanded-data object model flattens
only changed bytes** (`decisions/architecture.md`). The early corruption bugs were hunted down
precisely because a single stray byte is unacceptable — a save-location typo that overwrote
hidden items (`e20c167`), party data being mangled on write (`cb6fc99`), missables saved back
wrong (`ff76662`). Each got a dedicated fix. Treat the save bytes with that same care.

## No Hacks — But Accept Necessary Workarounds

The quality bar is high: **no hacks, no temporary fixes, no bad fallbacks.** Prefer the correct,
clean solution even when it's the longer route. If you can only see a hacky path, surface it and
ask rather than commit it.

**But** the history draws an important line. When the *framework itself* leaves no clean path, a
well-understood, documented workaround is not a hack — it's engineering. Qt forced several of
these and they were accepted deliberately: the `encodeBeforeUrl` hex trick around Qt Quick's
broken URL decoding (`8fe8447`), reporting a specific image size so Quick won't rescale/blur
(`0fb0106`), a small compromise to get the models working at all (`8fa9dca`). The distinction:
reject hacks of *convenience*; accept and **document** workarounds for genuine framework defects
(see the inline comments and `reference/qt-patterns.md`). A few QML quirks were even left
deliberately unfixed when the cost outweighed the benefit (a rare ListView glitch in `3e9e367`)
— a conscious trade, not neglect.

## Native Desktop, Not a Web App

Very early on (`49594d7`), the app moved off a pure Qt Quick `Window` into a real `MainWindow`
with a **native menu bar**, specifically to escape the "web page"-like feel of a QML menu. The
conviction holds: this should feel like polished desktop software that belongs in the
applications folder — a `QQuickWidget` hosted in a native window, native menus, native behavior.
Not a browser pretending to be an app.

## Don't Over-Engineer — A Lesson Learned the Hard Way

"Keep it simple" here isn't a platitude; it's scar tissue. The project was rewritten from
scratch **three times** (`context/origins.md`): a pure-C++ attempt, a JavaScript/QML version,
and the final C++ design. The JavaScript version in particular was torn out wholesale
(`df68676`) once it proved the wrong foundation. The takeaway carried forward: build the
simplest thing that's genuinely powerful, and finish it before adding more — complexity is what
overwhelmed the project the first time and shelved it for six years.

## Historical Context

The codebase carries the marks of multiple from-scratch rewrites and a long fight with Qt's quirks
(see `context/origins.md`). That's why some code looks the way it does — much of it is hard-won, so
check the notes before assuming a given choice is accidental.

---

## Things to Avoid

- **Random for the sake of random** — randomization without playability is a waste
- **Data dumps** — screens that just list raw values without context or guidance
- **Crashes as a feature** — "it's a dev tool, crashes are fine" is not acceptable here
- **Blocking dialogs in production** — the user should never see an unexpected popup
- **Over-engineering** — the original JS/Electron version collapsed under its own weight;
  keep things as simple as they can be while still being powerful
- **Feature creep without finish** — the editor needs to work fully before adding more screens

---

## Screens Priority (What Matters Most)

The editor is most useful when these core screens work well:
1. Trainer Card — name, money, badges, starter, time
2. Pokemon — team editing with moves, stats, DVs, nickname
3. Pokédex — seen/caught states
4. Items — bag and PC storage
5. Randomization — the flagship feature

Maps, Hall of Fame, and Rival are secondary. The font keyboard (name editing) should feel
native and in-game, not like a table of hex values.

