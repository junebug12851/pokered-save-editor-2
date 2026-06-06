# Project Overview

## What It Is

A desktop save file editor for Pokemon Red & Blue (Game Boy), built in Qt C++/QML.
This is the second version — a rewrite from scratch after the first version (JS/Electron) became
unmaintainable. The owner wrote the original solo, meticulously, line by line.

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

- **Sims 2-style graceful degradation** — never crash, never block the user
- Debug builds show error dialogs; release builds degrade silently
- The app should feel like polished software, not a hacker tool

## Owner

Twilight
