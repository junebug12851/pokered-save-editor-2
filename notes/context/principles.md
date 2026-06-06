# Project Principles and Will

The convictions behind the project — what it should be, what to avoid,
and the constraints that matter. These should inform every decision.

---

## What This Is To the Owner

This is one of Twilight's proudest projects. It was built solo, meticulously, line by line.
It was abandoned not from lack of care but from the complexity growing past what one person
could sustain alone. The revival in 2026 is a genuine second chance to finish something
important. Treat it that way.

---

## Core Philosophy: Graceful Degradation

**Inspired by The Sims 2.** The app must never:
- Crash or freeze unexpectedly
- Show blocking error dialogs in production (release builds)
- Leave the user stuck with no way forward

When something fails, it should fail silently and gracefully. Log it, degrade the feature,
but keep the app alive and usable. Debug builds show error dialogs (visible to the developer).
Release builds catch and log quietly.

This isn't about hiding bugs — it's about treating the user with respect.

---

## What the App Should Feel Like

**Polished desktop software, not a hacker tool.**

The user should be able to open a save file and immediately have fun. The app should:
- Present data in a way that's intuitive without needing documentation
- Make the fun stuff easy to find (randomize, edit name, see your team)
- Guide the user rather than dump raw data at them
- Feel like something that belongs in their desktop application folder

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

---

## The Owner's Voice on This Project

From the original chat:

> "It was in the days before AI, i was meticulously coding everything line by line... Eventually
> a great many commits never got pushed and then the project was trashed. All I have is whatever
> is online now, its in a broken buggy state. I've wondered for a long time if it could be
> salvaged... i didn't realize AI had come as far as you."

This is a project that matters personally. The technical work serves something real.

