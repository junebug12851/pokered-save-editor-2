# Project Overview

## What It Is

A desktop save file editor for Pokemon Red & Blue (Game Boy), built in Qt C++/QML.
This is the second version — a from-scratch rewrite after the first version (JS/Electron) became
unmaintainable. It is open source; Twilight has been its only developer so far.

## History

- Version 1: JS/Electron/Angular — mature but became a maintenance nightmare
- Version 2: Qt C++/QML reboot — 592 commits, last pushed March 2020
- Abandoned when complexity overwhelmed solo development capacity
- Revived in 2026 with development tooling assistance

## Repository

https://github.com/junebug12851/pokered-save-editor-2

## Intended Features

- Full save file editing across all screens:
  - Trainer Card (name, money, badges, play time, starter)
  - Pokédex (seen/caught state for all 151)
  - Pokemon team and boxes (stats, moves, DVs/EVs, nickname)
  - Items (bag, PC storage, Pokemart shopping)
  - Map/location (where you are, warps, sprites)
  - World state (events, missables, scripts, trades)
  - Hall of Fame records
  - Rival data
- **Map randomization** — place player on any valid map with correct warp/sprite/music setup
- **Full randomization** — generate a playable random new-game state (random map, Pokemon, items,
  name, but carefully constrained to stay playable — not a glitch fest)
- **Font keyboard** — in-game font character picker for name editing

## Philosophy

- **Graceful degradation** — never crash, never block the user; handle even otherwise-fatal errors
  through a clean, clear UI and, where possible, offer a way to continue
- Debug builds show error dialogs; release builds degrade gracefully and clearly — never silently
  swallowing errors, and never at the cost of save data
- The app should feel like polished software, not a hacker tool

## Maintainer

Twilight (sole developer so far; contributions welcome — the project is open source)
