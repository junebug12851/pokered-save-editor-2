# Forged saves — total custom state, authored by the console itself

The testing system can produce a **real, bootable save file at ANY map, position,
event-flag combination, or raw byte state** — the "total custom state resume" (briefed by
Twilight, 2026-07-16). This is the grunt-work layer under the dev MCP server: an AI session asks
for "Route 22, x=25 y=8, with these three flags on" and gets a save the console has already run.

## Why a naive forge cannot work (the chimera)

Poking a map id (+coords) into a save does **not** move the player — it makes a **chimera**. The
save's whole Area block (dims, tileset pointers, `wMapDataPtr`/`wMapTextPtr`/`wMapScriptPtr`,
sprites, warps, signs, connections, wild tables) still belongs to the *old* map, and the
`BIT_NO_PREVIOUS_MAP` linchpin means Continue **trusts** it. The console really enters the new map
id wearing the old map's body and **hard-crashes ~100 frames in** — the runaway CPU executes
`STOP`, the clocks halt, and PyBoy's `tick()` never returns (see
[`emulator-verification.md`](emulator-verification.md) → CORRECTION). Even a bare coords poke used
to leave a stale view pointer + block coords behind. **`forge_save.forge()` now refuses a cross-map
id and routes coords through `relocate()`.**

## The design: the game authors the state, we photograph it

`scripts/emu/forge_map_save.py` never *constructs* map state — it makes the **game itself walk
there**, then replicates `SaveSAVtoSRAM` (engine/menus/save.asm) off the settled console:

| Save section | Source | File range |
|---|---|---|
| `sPlayerName` | BaseSAV's (kept) | `0x2598` +11 |
| `sMainData` | WRAM `wMainDataStart` `0xD2F7` | `0x25A3` +`0x789` |
| `sSpriteData` | WRAM `wSpriteDataStart` `0xC100` | `0x2D2C` +`0x200` |
| `sTileAnimations` | tileset data (verified 1:1) | `0x3522` |
| `sMainDataCheckSum` | resealed | `0x3523` |

Party/items/box/flags stay BaseSAV's coherent state — the result is byte-for-byte what saving
there in-game would produce. Cached under `tmp/emu/map-saves/map<NNN>.sav` (+ sidecar json).

**Getting there — two routes, both console-driven:**

1. **The door hijack** (any map with a warp, ~236 of 248): boot BaseSAV (the player stands one
   square below Red's front door at Pallet (5,6)), rewrite that warp's destination **in live
   WRAM** (`wWarpEntries`, `0xD3AF`, 4 bytes `y,x,destWarpID,destMap`), step in. The game runs its
   own full map load; the player lands on the target's chosen warp (`--warp N`).
2. **The edge hop** (the 8 warpless routes — 3, 9, 13, 14, 17, 19, 21, 24): generate the connected
   neighbour's save first, relocate offline to a **collision-checked column** on the connection
   edge, boot, step once across — the console performs the connection transition itself.
   Candidate columns come from our own shipped data (`.blk` + `.bst` + the tileTraits `passable`
   list, sampling the square's **bottom-left tile** — the feet, same as `MapSim::tilePassable`),
   middle-out along the shared span; each attempt is one fresh single-shot process.

**Refused destinations** (stability, stated out loud): Trade Center / Colosseum (link rooms,
special-warp only) and the unfinished glitch copies.

## ⚠️ The SAME-TILESET trap (a real Gen 1 mechanism, found on the console)

`LoadTilesetHeader` (engine/overworld/tilesets.asm) only runs `LoadDestinationWarpPosition` when
the destination **tileset differs** from `hPreviousTileset` (or is a dungeon tileset). Warp
Pallet → Route 22 — both tileset 0 — and the game **keeps the old coords and view pointer**: the
player materialises at Pallet's door square on Route 22's grid, drawing from a garbage buffer
offset. (This is the machinery behind Gen 1's same-tileset warp glitches.) The generator therefore
warps **from a different-tileset source**: outdoor targets (tileset 0) are reached from **inside
Red's house** (tileset 1 — walk in for real, hijack the exit mat, press DOWN into the edge: mats
fire on the standing-on-warp collision path, `CheckWarpsCollision`, not on step-on); everything
else goes direct from Pallet.

## The side-calculations (`forge_save.py`)

`relocate(sav, x, y)` moves the player within a map with every derived byte kept in sync:

- `wYBlockCoord`/`wXBlockCoord` = `coord & 1` (BaseSAV-verified),
- `wCurrentTileBlockMapViewPointer` = `0xC6E8 + (y//2+1)·(w+6) + (x//2+1)`, little-endian —
  `MapEngine::viewPointer` verbatim, the formula `tst_map` proved byte-for-byte,
- bounds-checked against the map's own dims (coords are squares = 2× blocks) — refusing an
  off-map coord is a stability guarantee.

`view_pointer()` and `relocate()` are the shared implementations; the runner, the session child
and the MCP server all import them.

## Self-checks — a save this system emits has already run on the console

Before a generated save is written: arrival map id must match, dims must match `maps.json`, the
stored view pointer must equal the formula's, and the state must still be sane 120 frames after
settling (the crash-after-load guard). Console-verified across map classes (2026-07-16): Viridian,
Pewter, Lavender (outdoor two-hop), Rock Tunnel (dungeon), Viridian Forest (cave-class), Route 22
(outdoor via gate) — all generated, booted, and walked; Route 3 via the Pewter edge-hop.

## How to use it

| From | How |
|---|---|
| MCP (the normal way) | `emu_make_map_save(map_id)` → cached console-authored save; `emu_boot(map_id=…, x=…, y=…, flags=[…], pokes={…})` → interactive session at that exact state; `emu_forge_save(map_id=…, …)` → write the save to a path |
| Scenario tests | a `flag_scenarios.json` entry with `"map"` builds on the cached base automatically (`run_flag_scenarios.py` generates missing bases in a fresh subprocess) |
| CLI | `tmp\emu-venv\Scripts\python.exe scripts\emu\forge_map_save.py --map 0x21 [--warp N] [--x 25 --y 8] [--out p.sav] [--force]` |

Flags go by pret `EVENT_*` name (the canonical import), anything else by raw poke — **"world flags
at least generally right"** is inherited from BaseSAV's genuine progression, with the scenario's
specific flags layered on top.

## Total trigger control — scripts + filter flags (2026-07-16)

`forge()` also takes **`scripts`** (per-map script step, keyed by map name or entry ind:
`{"Route 22": 0}` → `wRoute22CurScript`) and **`filter_flags`** (missable visibility, keyed by
`"Map/Name"` or bit index: `{"Route 22/Rival 1": "show"}` → `wToggleableObjectFlags`, bit set =
hidden). Together with coords + event flags, that is enough to **arm a scripted cutscene** — the
last piece that a bare map+flags forge was missing. Exposed on `emu_boot(scripts=…,
filter_flags=…)` and `emu_forge_save(…)`.

### What a forge CAN and CANNOT drive (learned reproducing the Route 22 ambush)

A forge boots a *static* state; a scripted event is a *multi-frame cutscene*. Reproducing one taught
the boundary, and it is worth knowing:

- **Coord triggers fire from a booted save.** `Route22DefaultScript` runs every overworld frame and
  checks the player's coords — so spawning on the trigger tile (with the script byte = DEFAULT and
  the arming flags set) fires the cutscene; you do not have to walk in from the connecting map.
- **Geometry matters — a bad spawn softlocks.** The Route 22 rival walks RIGHT from (25,5) to (29,5).
  Spawn the player **on the upper trigger (29,4)** so row 5 is clear; spawn on (29,5) and the rival's
  walk is blocked forever (a forge artifact, not a game bug). `dbmapcoord x,y` stores `db y,x`.
- **Cutscenes have text boxes — you must press A.** The ambush shows *"HIM: Hey!"* and waits. A
  settle-only harness sits on that box forever and **misreports "healthy"** — the exact trap that made
  the 2026-07-15 run inconclusive. The interactive probes mash A/B; `run_flag_scenarios` (settle-only)
  cannot, so cutscene battles are driven by a dedicated probe, not the batch runner.
- **A hidden-sprite armed battle stalls.** Arming a battle whose rival object is filtered *hidden*
  leaves the cutscene waiting on a walk that never happens (`ghost` variant) — an invalid combo you
  could not reach in normal play, and a **stall, not a crash**.

### The Route 22 rival conflict: REFUTED on the cartridge

The long-suspected conflict (event-flags plan Phase 11 / `conflicts.json`) — both rival-battle flags
on + both `SPRITE_BLUE` stacked at (25,5) crashes — is **refuted**. Armed and driven into the battle
(`scripts/emu/probe_route22_conflict.py`): both flags on + both sprites shown → the coord trigger
fires, the rival walks over, and a **normal trainer battle engages** (`wIsInBattle=2`, sane tilemap
for 960+ frames, no crash/garble). The code says why: `DefaultScript` checks `EVENT_1ST` *before*
`EVENT_2ND` (an ordered if/else, not a collision), so the second flag is never consulted while the
first is set; the two sprites merely overlap. `conflicts.json` updated `suspected → refuted` with the
console evidence. This is the whole point of the conflicts system working: a suspicion, armed with
real state, adjudicated by the machine.

## Limits / future

- **Entry scripts:** a map with an entry cutscene dumps whatever state the settle window reaches;
  BaseSAV's progression keeps most quiet. The sidecar json records the generation route.
- **Default map library:** per Twilight, proper *default* map states ("set up like the scripts
  would") wait on the script import — this system is the console-authored stand-in until then, and
  the generation machinery (hijack → settle → dump) is exactly what that library will be built with.
- The cache is git-ignored (`tmp/`); saves regenerate on demand from the ROM + BaseSAV.
