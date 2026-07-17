# The dev MCP server — one reliable transport for the whole dev loop

`scripts/mcp/pse_dev_mcp.py` (launcher `scripts/mcp/run.cmd`, venv `tmp/mcp-venv` via
`scripts/mcp/setup.ps1`) is an MCP (Model Context Protocol) server that gives an AI
session — Claude/Cowork — a **standardized, reliable, leak-proof** way to drive the
entire rapid-development workflow: configure/build/clean-rebuild (both build dirs),
run tests, launch and drive the app through the DEBUG harness, take screenshots (app
and emulator, returned inline as images), forge saves, and drive PyBoy interactively
or single-shot — including installing/updating/health-checking PyBoy itself.

**Why it exists (2026-07-16).** The 2026-07-15 flag-scenario session wedged the
machine: PyBoy driven through the interactive shell's ~44 s per-call transport leaked
6–10 processes, starved the CPU, and every later call timed out — a feedback loop
(diagnosed in [`emulator-verification.md`](emulator-verification.md) → "Flag-scenario
test framework"). The problem was never PyBoy or the test logic; it was
**orchestration through a transport with a hard per-call cap and no process
ownership**. An MCP server has neither flaw: it is a long-lived local process that
**owns every child it spawns**, applies real timeouts, and kills whole process trees
on any overrun.

## The rules it encodes (all four are project law)

1. **Background by default; foreground is an explicit act.** Builds, tests, emu runs
   are hidden console-less children logged to `tmp/mcp-jobs/`. `app_launch` defaults
   to a minimized window (or fully headless `mode='offscreen'`); **`app_foreground()`**
   is the deliberate "it's ready for her to look at" moment (the standing 2026-07-12
   rule), and `app_background()` puts it away again.
2. **Jobs, never blocking calls.** Anything long returns a `job_id` immediately;
   `job_wait`/`job_status`/`job_log` poll it; a job that overruns its timeout has its
   **entire process tree killed** (`taskkill /T /F`). The server kills all live jobs
   and sessions at shutdown, and `procs_cleanup()` sweeps strays (the exact leak class
   of 2026-07-15). Crash-fast error mode (`SetErrorMode(0x0003)`) is set in the server
   and inherited by every child, so a crashing exe exits instead of hanging on a
   debugger dialog.
3. **One PyBoy per process — always.** PyBoy wedges if re-instantiated in one process
   (proven 2026-07-15). Single-shot probes stay single-shot (`emu_run_script`);
   the batch runs **one scenario per child** (`emu_flag_scenarios`); and interactive
   driving is a fresh owned child REPL per boot: `scripts/emu/drive_session.py`,
   NDJSON over stdin/stdout, every loop frame-bounded, EOF ⇒ clean stop, parent
   timeout ⇒ tree-kill. (This replaces the abandoned file-IPC `emu_server.py`
   experiment — right idea, wrong transport/ownership.)
4. **The exact kit toolchain, verbatim.** Jobs run with
   `C:\Qt\Tools\llvm-mingw1706_64\bin;C:\Qt\6.11.0\llvm-mingw_64\bin;C:\Qt\Tools\CMake_64\bin`
   prepended to PATH and `CC=clang CXX=clang++`. The **kit dir**
   (`projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`, Makefiles — what the
   app runs from) and the **test build** (`build/`, Ninja) are separate tools and
   never mixed up. Headless children get `QT_QPA_PLATFORM=offscreen` +
   `QT_QPA_FONTDIR=C:\Windows\Fonts`.

## Tool map

| Area | Tools | Notes |
|---|---|---|
| Info | `workspace_info` | VERSION, branch/HEAD, what exists, live jobs/sessions |
| Build | `build_app` · `build_tests` · `configure_tests` | kit dir vs `build/`; `clean` = `--clean-first`; jobs |
| Test | `run_ctest` · `run_test_exe` | offscreen + fonts + crash-fast; jobs |
| Shots | `capture_screenshots` | the headless screenshooter batch (`screenshots.md`) |
| Jobs | `job_status` · `job_wait` · `job_log` · `job_kill` · `job_list` | logs under `tmp/mcp-jobs/` |
| App | `app_launch` · `app_stop` · `app_foreground` · `app_background` · `app_cmd` · `app_screen` · `app_title` · `app_load_sav` · `app_get` · `app_set` · `app_click` · `app_tap` · `app_invoke` · `app_list` · `app_reload` · `app_shot` | the DEBUG TCP harness (`dev-harness.md`), traps encoded (below) |
| Emu | `emu_status` · `emu_setup` · `emu_check_updates` · `emu_update` · `emu_run_script` · `emu_flag_scenarios` · `emu_make_map_save` · `emu_forge_save` · `emu_boot` · `emu_button` · `emu_tick` · `emu_mem` · `emu_poke` · `emu_state` · `emu_screenshot` · `emu_stop` | ROM-gated, local-only; clean unavailability without it |
| **Autopilot** | `emu_goto` · `emu_walk_to` · `emu_talk_to` · `emu_battle` · `emu_hunt_encounter` · `emu_dismiss` · `emu_play` · `emu_set_flag` · `emu_give_item` · `emu_move_sprite` | pathfinding + auto-navigation + progression levers, below |
| App flows | `app_flow` | multi-step app driving (get/set/tap/wait-until/shot…) in ONE call |

**Total custom state resume (2026-07-16).** `emu_boot(map_id=…, x=…, y=…, flags=[EVENT_*…],
pokes={…})` boots an interactive session at ANY map/position/flag/byte state: the map base is a
**console-authored save** (`emu_make_map_save` → `scripts/emu/forge_map_save.py`: the game itself
walks there through a hijacked door and the settled WRAM is dumped), coordinates relocate with the
derived bytes kept in sync, flags/pokes layer on top, checksum resealed. First generation of a map
takes ~30–60 s (then cached under `tmp/emu/map-saves/`). Full system + traps:
[`forged-saves.md`](forged-saves.md).
| Hygiene | `procs_cleanup` | strays list/kill; `include_app` for orphan editors |

`app_shot` and `emu_screenshot` return the PNG **inline as an MCP image** (and save a
copy under `tmp/mcp-shots/`), so the reviewing session sees the pixels directly — the
mandatory screenshot-review pass without any focus juggling.

## The harness traps, encoded so they cannot recur

- **One fresh TCP connection per command** (`dev-harness.md` trap #2 — a held-open
  socket returns stale `shot` frames). `_tcp()` reconnects every call.
- **Never navigate to the screen you're already on** (trap #1 — the router pushes a
  dead duplicate). `app_screen` refuses when the title already matches; `app_list` is
  the duplicate detector.
- **A minimized window stops rendering** (trap #3) — `app_shot`'s doc says so; use
  `mode='offscreen'` for headless shots or `app_foreground()` first.
- **`click` emits the signal; `tap` is a real mouse event** — both exposed, the
  difference documented on the tools (it's a whole class of bug).
- **PyBoy reads battery RAM from `<rom>.gb.ram`** — `drive_session.py` copies the
  (optionally forged) sav there itself; callers never touch it.

## The autopilot — pathfinding & auto-navigation (2026-07-16)

Briefed by Twilight the same day: *describe the destination and the server takes you there* —
plus find-a-battle, execute-a-battle-a-certain-way, talk to a (moving) NPC, and whole runs in
one call. Design + verification battery: [`../plans/dev-autopilot.md`](../plans/dev-autopilot.md).

- **`emu_goto(map, x?, y?)`** — map by name/modernName/id (`"Mt Moon B1F"`, `"Mt. Moon 2"`,
  `60`, `"0x3C"`). No live session → boots one automatically (target map's console-authored
  base, or `start_map=` for a real journey). Cross-map: warps + connections + doors + ladders,
  planned by Dijkstra-over-portals with A\* legs (`scripts/emu/navigate.py`), walked
  step-verified against WRAM (`scripts/emu/autopilot.py` inside the session child).
- **`emu_walk_to(x, y)`** — in-map A\* (ledge hops included); live NPCs are moving collision
  (bounded retries → re-plan); wild battles follow `on_battle` (`run`/`mash`/`stop`), trainers
  `on_trainer` (`stop`/`mash`).
- **`emu_talk_to(target)`** — chases the NPC's LIVE square (StateData2, re-read as it wanders),
  stands adjacent, faces, presses A, confirms `wFontLoaded`. Trainer battles reported.
- **`emu_hunt_encounter()`** — paces a grass shuttle (the map's own `wGrassTile`, read live;
  relocates to the nearest grass patch if needed; caves pace anywhere) until `wIsInBattle`;
  reports enemy species/level; `policy` can resolve it on the spot.
- **`emu_battle(policy)`** — `mash` | `run` (wild only; retries "can't escape") | `move:N`
  (B declines level-up move prompts — never silently overwrites a moveset).
- **`emu_play(steps)`** — a whole run in one call, executed inside the child (goto/walk/hunt/
  battle/talk/button/tick/poke/mem/shot/dismiss), per-step results back.
- **`app_flow(steps)`** — the same idea for the APP: screen/get/set/tap/click/invoke/
  **wait-until-property**/assert/shot/sav/reload as one batch over the DEBUG harness (the map
  screen without twenty round-trips).

**Console-verified 7/7** (`scripts/emu/probe_autopilot.py`; one session child per case):
sub-save WRAM addresses validated live (`wFontLoaded 0xCFC4`, `wCurrentMenuItem 0xCC26`,
`wGrassTile 0xD535` — grass came back 82 on Route 1) · Pallet A\* walk · door + "Last Map" mat ·
connection crossings · **Mt Moon 1F → B2F** · Route 1 hunt + flee · moving-NPC talk. Plus the
long haul: **Pallet → Pewter straight through Viridian Forest and back to a Route 1 hunt won
with `move:1`** — every leg on the real console.

**The traps it encodes (each found by the probe, each now structural):**

- **A held direction is a DOUBLE step.** A square is 16 frames; holding longer starts a second
  step before release — on a connection edge row that second step walks clean off the map. The
  walker turns first (3-frame tap), then presses 12 frames, and verifies coords after every step.
- **`wCurMap` updates FIRST in a transition** — dims/coords/blocks lag it. Reading position
  right after the map byte flips hands back the old map's coordinates (an off-grid read).
  `_settle_arrival` requires the destination's own dims in WRAM + in-bounds coords + free
  controls, 20 stable frames.
- **A warp square can be SOLID** (gate doorway halves, cave ladders): the console warps on the
  walk ATTEMPT (`CheckWarpsNoCollision`) — so A\* treats a warp goal as enterable, the planner
  disprefers solid squares when a passable twin exists, and a doorway that refuses to open after
  3 bumps is blacklisted and re-planned around, honestly.
- **maps.json's connection pair is POST-CLAMP** (`stripMove` == tgt−3, `stripOffset` == src) —
  the real signed offset is `(-stripOffset − 3)` when stripOffset ≠ 0, else `stripMove` (the
  cartridge-verified `MapEngine::connectionOffset()` rule). Crossing column p arrives at
  p − 2·offset.
- **Ledges are Overworld-tileset-only** (`HandleLedges` bails unless `wCurMapTileset == 0`) — a
  "ledge tile id" in a cave is just a rock.

### The progression layer (2026-07-16, same-day follow-on brief — "progress normally when asked")

The v1 limits fell the same day. All of it **console-verified** (the probe battery is 16/16):

- **Natural drop-in (the default).** `emu_goto` with no session boots ONE MAP OUT — a map that
  warps/connects into the target (`World.approaches_of`) — and **walks in for real**, so
  `wLastMap` and the entire entry state are **authored by the walk** (arrive in Mt Moon *from
  Route 4*; the Pokécenter doorstep, not a teleport). `approach='direct'` for the old boot-at-
  target; `start_map=` for a full journey. Verified: entering Mt Moon left `wLastMap = Route 4`.
- **Saffron gate guards** — the block is `wStatusFlags1` (`0xD728`) **bit 6**
  `BIT_GAVE_SAFFRON_GUARDS_DRINK` (pret `scripts/Route5Gate.asm`), NOT an event flag. A gate on
  the route → the bit is set, reported in `prep`. (BaseSAV already carries it — the probe clears
  it first to prove the mechanism.)
- **Elevators** — planner edges car → every floor with a warp into it; ridden by re-aiming the
  car's own door warps in live WRAM (the floor menu's own technique) and stepping out. Verified:
  Celadon Mart 1F → car → 5F.
- **Surf** — ⭐ research finding: **poking `wWalkBikeSurfState` (`0xD700`) = 2 right before
  stepping onto water WORKS** — the console walks onto the water surfing, keeps state 2, and the
  autopilot dismounts ashore. `surf='auto'` plans dry first and opens water only when no dry
  route exists (reported). Verified: Pallet → Route 21, a water-only connection crossing.
- **Cut trees** — our own `cutTreeBlocks` data carries the replacement block: `clear_tree()`
  pokes the live block buffer + the on-screen tiles (collision reads the screen buffer; the
  player screen anchor (8,8) is verified against our grid before any tile poke). `cut='auto'`
  (tried BEFORE surf — the smaller intervention). Verified: Vermilion Gym's mandatory tree.
- **Spinner mazes** — the forced-movement coords are imported from pret's own scripts
  (`map_coord_movement`; Viridian Gym, Rocket Hideout B2F/B3F, Pokémon Tower 7F), priced at
  SPIN_COST 400 (avoid, never refuse), and a slide is followed to rest (the joy-ignore wait
  tolerates motion) then re-planned. Verified: across Rocket Hideout B2F.
- **Bike** — Cycling Road on the route → a BICYCLE lands in the bag (reported; BaseSAV already
  carries one). Verified: Route 16 → Route 17.
- **Win on request** — `emu_battle('sweep')` holds the enemy's HP at 1 in WRAM and attacks; a
  trainer's whole team falls hit by hit (B declines move-learning). Verified: **Brock, beaten on
  request** (`EVENT_BEAT_BROCK` cleared live, talked to, swept — 2 HP pokes, both mons).
- **New levers**: `emu_set_flag` (EVENT_* by name, live), `emu_give_item`, `emu_move_sprite`
  (the Strength-boulder lever — put the rock where it's needed). Every intervention is
  **reported in `prep`/events, never silent**, and every one is opt-out.

**Two more traps the probes caught (now structural):**

- ⚠️ **`wXCoord`/`wYCoord` update at step START, not completion** — coords said "arrived" while
  the glide was still running, and a button down at the glide boundary chained a second step
  (the Route 4 cave approach coasted PAST its goal after `walk_to` returned). The step is over
  when **`wWalkCounter` (`0xCFC5`) == 0** — the console's own signal; the settle waits for it.
- ⚠️ **BaseSAV's real progression hides mechanisms**: the guard already had his drink, Brock was
  already beaten — a probe that doesn't RESET the state it tests proves nothing.

Still honest limits: Strength boulder puzzles aren't auto-solved (use `emu_move_sprite`
deliberately); spinner mazes are avoided, not ridden (per-square arrow directions = a future
import); Flash is cosmetic (WRAM navigation doesn't need light).

## Forging saves — shared, importable

`scripts/emu/forge_save.py` is the STANDING METHOD (any map/position/flags, checksum
resealed) extracted into one importable module + CLI, so probes, the session child and
the MCP server share a single implementation. `emu_forge_save` / `emu_boot` take
map/x/y/flags/`all_flags`/raw `pokes` and reseal automatically.

## Setup & registration

```powershell
pwsh -File scripts/mcp/setup.ps1      # tmp/mcp-venv: mcp + psutil (dev-only, git-ignored)
```

Register `scripts\mcp\run.cmd` as a local (stdio) MCP server in the Claude desktop
config; the launcher bootstraps the venv if missing and keeps stdout clean for the
protocol. Licensing footing is identical to the emu venv
([`emulator-verification.md`](emulator-verification.md) → Licensing): dev-only,
never linked into or shipped with the editor.

✅ **Registered on this machine (2026-07-16):** `mcpServers.pokered-dev` added to the
Claude desktop config (the physical file lives at
`C:\Users\juneh\AppData\Local\Packages\Claude_pzs8sxrjxfjjc\LocalCache\Roaming\Claude\claude_desktop_config.json`
— the `%APPDATA%\Claude` path is the same file through MSIX virtualization; a
`.bak-2026-07-16` sits beside it), pointing at the venv python + `pse_dev_mcp.py`
directly. **Restart the Claude desktop app to load it.** Launch smoke-tested: the
registered command comes up and stays alive on stdio.

## Limits / honesty

- The server assumes this machine's kit paths (they are constants at the top of
  `pse_dev_mcp.py`) — it is a dev tool for this workstation, not a shipped artifact.
- `emu_*` needs the local-only ROM and `tmp/emu-venv`; without them the tools say so
  and everything else still works.
- One emulator session at a time (deliberate — one PyBoy per process, one process at
  a time keeps the machine honest). A new `emu_boot` recycles the child cleanly.
