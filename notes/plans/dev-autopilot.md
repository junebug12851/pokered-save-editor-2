# The dev autopilot — pathfinding, auto-navigation & scenario setup over MCP

Briefed by Twilight, 2026-07-16 (same-day follow-on to the dev MCP server): the MCP server
must carry **comprehensive, high-level control of the running game and the running app**, so
that an AI session can say *"set this up and take me there"* in ONE call instead of driving
hundreds of button presses over the transport. The wishes, in spirit:

- describe a location → the server **forges a save, boots it, and navigates there** —
  including **cross-map journeys** (Mt. Moon 1F entrance → B2F exit; town → town),
- **find a battle** in an area and **execute the battle a certain way**,
- menus, warps, text boxes — handled server-side,
- **short bursts or long runs**, both first-class,
- the same idea for the **app**: the map screen is complex; multi-step app driving should be
  one MCP call too,
- generally: whenever moving smarts into the MCP server makes an AI session's life easier,
  move them.

The game side is the priority; the app side rides along with a generic flow-runner.

---

## The shape (three layers, one law)

The law is inherited from the dev MCP server and does not bend: **one PyBoy per process,
every loop frame-bounded, every child owned and tree-killed on overrun.** Navigation lives
INSIDE the session child (one process does the hundreds of steps); the MCP layer only plans,
forwards, and reports.

| Layer | File | What it holds |
|---|---|---|
| **World model + planner** | `scripts/emu/navigate.py` | Pure data, no PyBoy. Loads `maps.json` + `tileTraits.json` + `.blk`/`.bst` (all imported verbatim from `pret/pokered`). Builds each map's **square-grid passability** (feet = the square's bottom-left tile, same sample as `MapSim::tilePassable` / `forge_map_save.walkable()`), **ledge one-way jumps** (the global (standing-on, facing, front-tile) triples), pair collisions where the data carries them, and the **portal graph** — every warp (`warpOut` → `toMap`/`toWarp`, arrival = `warpIn[k]`) and every connection edge (crossing column ↔ `stripOffset` mapping, both sides checked walkable offline). In-map = **A\*** (4-dir + ledge hops); cross-map = **Dijkstra over portals** with A\* leg costs. |
| **Executor** | `scripts/emu/drive_session.py` (new commands) | The live PyBoy walks the plan **step-verified**: press → poll the console's own coords → confirm. Blocked square (a wandering NPC) → bounded retries → mark + **re-plan**. Text box (`wFontLoaded`) → dismiss. Battle (`wIsInBattle`) → apply the battle policy → resume. Warp square reached but no transition → nudge into the mat direction. Connection = step across the edge, confirm map id. Every command carries a hard frame budget. |
| **MCP tools** | `scripts/mcp/pse_dev_mcp.py` | `emu_goto` (forge base if needed + boot + navigate, by map **name or id**), `emu_walk_to`, `emu_talk_to`, `emu_battle`, `emu_hunt_encounter`, `emu_dismiss`, and **`emu_play`** — a step-list composite (goto/walk/hunt/battle/button/poke/shot/…) executed server-side in one call. App side: **`app_flow`** — a step-list over the DEBUG harness (screen/get/set/tap/click/invoke/**wait-until-property**/shot) in one call. |

## What the executor trusts

**WRAM, never pixels.** Position `wYCoord`/`wXCoord` (`0xD361`/`0xD362`), map `wCurMap`
(`0xD35E`), battle `wIsInBattle` (`0xD057`; 1 wild, 2 trainer), facing (sprite slot 0
StateData1+9, `0xC109`), text box `wFontLoaded` (`0xCFC4` bit 0), menu cursor
`wCurrentMenuItem` (`0xCC26`), live NPC squares (StateData2 mapY/mapX per slot) for
re-planning around people. The menu/text addresses sit below `wMainDataStart`, are not in
the save, and so were never pinned by the save-field probes — **the autopilot probe
validates them live** (move the cursor, watch the byte) before anything relies on them.

## Battle policies

- `mash` — A until `wIsInBattle == 0` (first move, level-ups dismissed). The blunt default.
- `run` — pin the cursor at FIGHT (UP+LEFT), DOWN+RIGHT → RUN, A; retry on "can't escape"
  (bounded). Refused with a clear reason when `wIsInBattle == 2` (trainer battles can't run).
- `move:N` — pin at FIGHT, A, cursor to slot N, A; repeat each turn until it ends.
- Hunting: `hunt` paces a passable two-square shuttle (bounded steps) until
  `wIsInBattle != 0`, then hands over to the policy.

## NPCs — moving collision AND interaction targets (Twilight, 2026-07-16)

NPCs are both obstacles and destinations. The walker treats their **live** squares
(StateData2 mapY/mapX, re-read every step) as soft collision — bounded retries, then
re-plan around. And `talk_to` makes them targets: pick the NPC (by sprite slot, or by its
`maps.json` name), **chase its live square** (it wanders — re-plan each approach), path to
a 4-adjacent square, turn to face it, press A, and confirm the text box actually opened
(`wFontLoaded`), retrying as it moves. Options: dismiss the text to the end, or leave it
open (cutscene handoff).

## Honest v1 limits (stated, not hidden)

No HM routing (Surf/Cut/Strength/Flash — Rock Tunnel still walks fine: WRAM doesn't need
light), no bike, no spinner tiles / forced-slide maps (best effort + honest failure), no
elevators (menu-driven maps), Fly/Dig/Escape Rope not used. Trainer line-of-sight can start
a battle mid-walk — that is handled (policy), not avoided. Warpless-route legs cross real
connections; refused destinations stay refused (link rooms, glitch copies).

## Verification (the console is the oracle)

`scripts/emu/probe_autopilot.py` — one session child per case, NDJSON, tree-kill-guarded:
menu-address validation · in-map A\* walk (Pallet corners) · warp leg (Pallet → Red's 1F →
back) · connection leg (Pallet → Route 1 → Viridian) · **Mt. Moon 1F entrance → B2F exit
run** (the brief's named case) · Route 1 encounter hunt + `run` + `mash` policies. Each case
asserts from WRAM (map id + coords), never from pixels.

## Owed / notes

- Docs land in [`../reference/dev-mcp.md`](../reference/dev-mcp.md) (tool table + traps)
  as they are built.
- The conflicts/event work gets `emu_play` recipes for cutscene driving (A-mash windows).
- Future (un-briefed, listed only): HM routing, bike, elevator menus, a "default map
  library" tie-in once the script import lands.
