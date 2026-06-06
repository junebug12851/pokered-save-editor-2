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

## Map Editor (Medium Priority)

Maps in Gen 1 carry hundreds of variables — warps, sprites, connections, music, wild Pokemon,
tileset, etc. Goal is to let users:
- Pick any map, click "load player here"
- Edit connections, warps, sprites visually
- Have the app handle pointer calculations automatically

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

