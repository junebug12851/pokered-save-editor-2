# Future Plans and Ambitions

## The Vision

This project was built with a clear ambition: a polished desktop save editor that understands
the data it's presenting — not just showing raw bytes. The owner wants people to be able to
open a save file and immediately have fun with it, guided by smart defaults.

## Randomization Feature (High Priority)

The most exciting feature even in its early buggy state. Goals:
- Place player in a random but **playable** new-game state
- Random map, random Pokemon team, random name, random starter pack of items
- Constrained to avoid glitches: no glitch items, no glitch Pokemon, no broken maps
- Always includes an "escape Pokemon" (HM slave with relevant field moves)
- No progress: no beaten trainers, no events completed, clean start
- Playability is the constraint — randomization within those bounds

Quote from session 1: _"even with this incredibly early feature it was so fun to play I found
myself re-rolling new save files even throughout the bugs."_

The spirit, in the owner's own early-playtest words (from `NewGoals.md`): being _"dropped into
Pokemon Tower as Paddles with a ghost Pikachu named Wolvoom that knows the wildest of moves,"_
navigating out to start the adventure, _"only to encounter a level 5 Zapdos along the way and the
whole map being darker than normal."_ Curious, surprising, a little broken-feeling — but the kind
of thing that makes you laugh and re-roll. **That feeling is the design target.**

## Map Editor (Medium Priority)

This is one of the reboot's headline ambitions (from the original `NewGoals.md`). A Gen 1 map
carries hundreds of variables — connecting maps, warps, sprites, positions, music, wild Pokémon,
tileset, and more — and the goal is to expose all of it **without complicating anything**: let
users do far more with "the area you're in" than was ever possible before.

Imagined UX:
- Pick **any** map in the game and click **"load player here"** to drop your character onto it.
- Edit connections, warps, sprites, positions, music, and wild Pokémon — "editing options you
  probably didn't even know about."
- The app does the grunt work in the background — sensible **default** warps/sprites/connecting
  data and all the **pointer calculations** — so it just works...
- ...while **still exposing full power** to anyone who wants to go all the way. Smart defaults,
  not locked doors. The intent is to *encourage* the user to go out and have fun with the map.

## Parked idea — map commit / history / diff system (Twilight, 2026-07-14)

Reminder only, so it isn't forgotten — **not briefed, not planned, not designed.** The map system
wants a **commit + history + diff** system (revision snapshots of the map edits, a browsable history,
and a diff view between states). Un-briefed per the "adjacency is not a brief" rule — it gets its own
conversation, then research, then a design in the plan, before any code. Nothing here authorizes work.

## Full Screen Coverage

Snapshot as of session 13j (see `status.md` for the live, authoritative list):

Working / data flows + persists:
- Font keyboard (filters, search, str propagation, DetailView)
- Trainer Card (name + rival render/persist, ID, money, coins, starter, badges, playtime)
- Pokémon box screen (sprites/levels, hover name+pen, click → details, no crash)
- Pokédex (toggle/click)
- Bag / Items (read + edit + count)
- Pokémart (looked correct)
- File open/save

Working but needs polish (mostly Qt 6 Material-height layout — see `status.md` open issues):
- Pokémon details editor (opens + data persists; **re-test moves/DV/EV after the s13h GC fix**;
  level box + box heights still need layout work)
- Full name editor (right-side tileset preview blank; description not showing; empty rows when
  items unchecked)

Not yet exercised/verified end-to-end:
- Rival data (name shows; verify starter saves)
- Maps / Map Details (verify `appBody.push` navigation)
- Pokémon storage PC boxes beyond party
- Hall of Fame

## Technical Ambitions

The owner wanted this rewrite specifically because the JS/Electron version couldn't be extended
further without becoming unmanageable. Qt gives:
- Fast builds (compared to 45-minute Electron builds)
- Native performance
- Full access to game data through the C++ DB layer
- A path to features impossible in the browser sandbox

