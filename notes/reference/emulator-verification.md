# Emulator verification — checking the editor against the actual Game Boy

Every other test in this project asks *"is the editor consistent with itself?"*. This one asks the only
question that really settles it: **does the real game agree?**

`tst_emu_parity` boots the genuine ROM in an emulator with one of our save files, lets the game load it,
and then reads the **console's own RAM** back out — and demands that what `MapEngine` built matches it
**byte for byte**. The editor can be internally consistent and still be wrong. The console can't.

> **It found a real gap the first time it was pointed at us.** Our border ring was the map's border block;
> the game's was Route 1 and Route 21 bleeding in over it. See "What it caught immediately" below.

## STANDING METHOD — forge a synthetic save at ANY map state to test (2026-07-15, leadership)

**We can, and should, generate a fake save file placed at any state we want and boot it on the console.**
We now hold the full map data — every map's id, tileset/blockset, dimensions, objects (coords/kind),
warps, connections, wild tables, and all 2,560 event flags — so a probe can **forge a save that starts
the player at any map, any position, with any flag/state combination**, seal its checksum, and boot the
real ROM straight into that situation. No walking a fresh game to the spot, no save-scumming — total
control over the starting state.

**Use it for testing by default** whenever a behaviour only manifests in play (a flag combo that crashes
on a specific map, a warp, an encounter): forge the exact save and let the console adjudicate. The forge
recipe (verified): write the save bytes, set `wCurMap` (file `0x260A`), `wYCoord`/`wXCoord`
(`0x260D`/`0x260E`), the event-flag bytes (`0x29F3`+, 320 bytes) and any other Main-Data byte, then
**re-seal the checksum** (`0xFF`-running-subtract over `0x2598`..`0x2598+0xF8B`, stored at `0x3523`) or
the game rejects the save. Boot via PyBoy (`window="null"`), mash start/A to Continue, and read
`wCurMap`/map dims as a **crash/health signal** (`scripts/emu/probe_event_flag_crashes.py` is the
template). Optionally drive a few `button` presses to cross a coord trigger. **Never commit the ROM.**

**The shared implementation is `scripts/emu/forge_save.py`** (importable + CLI: map/coords/flag names/
raw indices/arbitrary byte pokes, checksum resealed) — probes, `drive_session.py` and the dev MCP
server's `emu_forge_save`/`emu_boot` all use it instead of carrying private copies.

### ⚠️ CORRECTION (2026-07-16): a map-id-only forge is a CHIMERA, and a wedged console WEDGES PyBOY

Two hard-won facts that overturn earlier readings:

1. **Forging ONLY `wCurMap` (+coords) onto a different map does NOT give you that map.** The save's
   whole Area block — dimensions, tileset, script step, sprites, warps — still belongs to the *old*
   map, and the `BIT_NO_PREVIOUS_MAP` linchpin means the console TRUSTS it on Continue. The result is a
   chimera (Route 22's id wearing Pallet's body: the emulator really shows `map=0x21, w=10, h=9`), and
   it **hard-crashes within ~100 frames of entering the overworld**. The 2026-07-15 "forge-onto-any-map
   PROVEN (boots onto Route 22)" claim was a mirage: `on_overworld()` returns true for a moment — then
   the crash lands. Isolated 2026-07-16 (`forge+mash` wedges; `forge` alone and `mash` alone both run
   6,000 frames clean at ~10,000 fps). **Same-map forges (flags, coords, any Main-Data byte) are fully
   consistent and safe.** A *consistent* cross-map forge means writing the whole Area block for the
   target map (everything the editor's own model knows how to write) — a properly-briefed future phase.
2. **A hard-crashed console wedges PyBoy itself.** The runaway CPU executes `STOP`, the clocks halt,
   the frame never completes — and **`pb.tick()` never returns**, spinning a full core forever (the
   glitch-music bad-bank probe recorded the same console behaviour). No in-process guard can help
   (the wedge is inside the C extension); **the OWNER's timeout + process-TREE kill is the watchdog**,
   and "hang" is recorded as a legitimate scenario verdict. ⚠️ When killing, kill the **tree**
   (`taskkill /T /F`): the venv `python.exe` is a *launcher* whose child interpreter (and its spinning
   PyBoy) can survive a plain kill — this launcher+interpreter pair was the second source of the
   2026-07-15 leaks. `tst_flag_scenarios`, the runner, and the dev MCP server all encode this now.

**The standing transport for all of this is the dev MCP server** ([`dev-mcp.md`](dev-mcp.md)): jobs
with hard tree-kill timeouts, one PyBoy per process, an interactive owned session
(`scripts/emu/drive_session.py`), and a stray-process sweeper. The interactive shell (with its ~44 s
per-call cap and no process ownership) is not to be used for driving PyBoy again.

## Flag-scenario test framework — and the process-lifecycle lesson (2026-07-15)

**What was going wrong (diagnosed, not guessed).** The emulator and the test logic are fine — a clean
single boot works, and the `control-pallet` + `all-flags-on-pallet` scenarios ran correctly. The failures
were **100% process lifecycle**: driving PyBoy from an interactive shell with a **~44 s per-call timeout**.
A boot+drive run exceeds that, so it gets backgrounded; but when a *management* command (kill/verify)
times out mid-execution, the PyBoy process **leaks**. Leaks accumulate (we saw 6–10), starve the CPU, and
slow everything — which causes *more* timeouts. A feedback loop. **Not an emulator problem, not a code
problem — an orchestration problem.**

**The right pattern (reliable, already proven here).** `tst_emu_parity` never has this problem because it
is a **single-shot subprocess** managed by **`QProcess`** with `waitForFinished(180000)` + `kill()` on
timeout — the child always boots once, writes files, and **exits**, and Qt reaps it. No persistent
session, no IPC, no interactive driving. So the flag testing framework follows that exact shape:

- `scripts/emu/run_flag_scenarios.py` — a **single-shot batch runner**. Reads `flag_scenarios.json`,
  forges a save per scenario (any map/pos/flags), boots on Continue, optionally drives the player, and
  classifies **healthy / crash / no-boot**. Every scenario gets its **own** PyBoy stopped in a `finally`;
  every loop is **frame-bounded** (nothing can hang); results are written **incrementally**; exit code
  **2** when unavailable (no ROM / not under the venv) — same contract as `dump_state.py`.
- `scripts/emu/flag_scenarios.json` — the scenarios (controls + the Route 22 rival conflict + all-on).
- **Wrap it as a CTest test** `tst_flag_scenarios` — **written** at `projects/tests/emu/tst_flag_scenarios.cpp`
  (a `QProcess` wrapper cloned from `tst_emu_parity`: skip without the ROM, else launch the runner with
  `waitForFinished(600000)` + `kill()` on overrun, read `tmp/emu/flag_scenarios_result.json`, assert
  `control-pallet == healthy` and that every scenario produced a result; surface crash outcomes via
  `qInfo`). Then it runs inside the normal `ctest` pipeline — which the project already launches
  **detached-to-a-log-and-polled**, reliably, for the whole suite. **This is the fix: run it where long
  tests belong, with Qt owning the child's lifecycle — never through the interactive, time-capped
  transport.**

  ✅ **Wired into CMake and build-verified (2026-07-16).** It runs inside the normal `ctest` pipeline
  (SKIPs cleanly without the ROM/venv, so CI stays green). Two lessons from wiring it live:
  `QTEST_FUNCTION_TIMEOUT=900000` rides in its test ENVIRONMENT (initTestCase runs every scenario, and
  a wedged console is a 180 s "hang" verdict each — QtTest's default 300 s function watchdog aborts the
  run otherwise), and a timed-out scenario is **recorded as `hang` and the batch carries on** (only the
  control gate hard-fails) — see the CORRECTION above for why a hang is a verdict, not a harness bug.

⚠️ **PyBoy can't be re-instantiated in one process (found 2026-07-15).** Running several scenarios by
constructing a new `PyBoy(...)` per scenario *in one process* hangs on the 2nd/3rd instance (the Pallet
controls pass, then the first map-override scenario wedges). So the batch must be **one scenario per
process**: the runner takes `--only <name>` (run just that scenario, exit), and the CTest wrapper launches
it **once per scenario** via `QProcess` — fresh PyBoy, fresh process, Qt reaps each. This also means an
interactive single-scenario run still exceeds the shell's ~44 s cap (one boot + drive ≈ 30–60 s under any
load), which is *why* this belongs in CTest, not the interactive transport.

**Rejected alternative:** switching emulators (mGBA / BizHawk / …). That adds a dependency and a new
integration and would hit the *same* lifecycle issue through the same transport — **more** fragility, not
less. PyBoy was never the problem.

**To get a clean live run:** on a machine with no leaked python/PyBoy processes, either run
`scripts\emu\run_flag_scenarios.py` once under `tmp/emu-venv` (it self-terminates), or build + `ctest`
`tst_flag_scenarios`. If runs are crawling, check for leaked `python.exe` first (`Get-Process python`).

## The ROM — read this first

`assets/references/backup.gb` is a dump of a cartridge **Twilight owns**, kept locally for verification.

- It is **git-ignored** (`/assets/references/` plus explicit `*.gb` / `*.gbc` / `*.gba` / `*.rom` rules, so
  it cannot be added by accident even if it moves). It has **never** been committed — verified against the
  full history, not just the working tree.
- It is **never** published, released, copied into a build artifact, or uploaded anywhere. The harness
  copies it only into `tmp/emu/` (also git-ignored) because PyBoy wants the battery save beside the ROM.
- It is **not required**. Without it — CI, a fresh clone, anybody else's machine — every case **SKIPs** and
  the suite stays green. Nothing about building, testing or shipping the editor depends on it.

Its SHA-1 is `ea9bcae617fdf159b045185467ae58b2e4a48b9a`, which is **exactly the `pokered.gbc` SHA-1 in the
disassembly's own `roms.sha1`**. The ROM, the disassembly we read the map format out of, and the editor are
therefore all the *same game* — which is what makes the emulator a valid oracle rather than an approximation.

## Licensing

- **PyBoy** (LGPL-3.0) is a **developer tool**: installed separately into a throwaway venv, invoked as its
  own process, **never linked into or shipped with the editor**. The app remains Apache-2.0 and ships
  nothing of PyBoy's. It is credited in `credits.json` → Tools Used, with that distinction stated.
- Its dependencies are all permissive and equally dev-only: **numpy** (BSD-3), **Pillow** (MIT-CMU, already
  credited — the GIF scripts use it), **PySDL2** (CC0) and the **SDL2** binaries it bundles (zlib).
- Nothing in `tmp/emu-venv/` is committed, redistributed, or bundled into a release.

> ⚠️ Windows Defender flagged something during the first install (it passed on its own). Bundled binary
> DLLs like SDL2's are a common false positive. If it recurs, `pwsh -File scripts/emu/setup.ps1 -Recreate`
> rebuilds the venv from scratch, and deleting `tmp/emu-venv` removes every trace of it.

## Setup

```powershell
pwsh -File scripts\emu\setup.ps1        # creates tmp/emu-venv, installs PyBoy, verifies the ROM's SHA-1
ctest -R tst_emu_parity --output-on-failure
```

## How it works

`scripts/emu/dump_state.py` (PyBoy) boots the ROM with a copy of one of our `.sav` fixtures and dumps what
the game actually did into `tmp/emu/`:

| File | What it is | WRAM |
|------|-----------|------|
| `state.json` | the game's own map, tileset, size, coords — and the view pointer **it** computed | — |
| `wOverworldMap.bin` | the block buffer it built: the map ringed by its 3-block border | `$C6E8` |
| `wSurroundingTiles.bin` | the 6×5-block scratch area, expanded to 24×20 **tile** ids | `$C508` |
| `wTileMap.bin` | the 20×18 tile ids actually **on screen** | `$C3A0` |
| `screen.png` | the 160×144 framebuffer (BG + sprites + palette) | — |

`tst_emu_parity` then compares each against `MapEngine`. **`wTileMap` is the strongest of them**: it is the
entire view pipeline in one array — blocks → tiles → scratch area → half-block screen offset — with no
sprites and no palettes in the way. If that matches, the map emulator is right.

### Booting the front end — mechanically, not hopefully

Getting from the title screen to the overworld has to work *every* time, so it is driven by the game's own
memory rather than by pressing buttons and hoping:

- **The obvious check is a trap.** "Does WRAM hold the save's map and coordinates?" passes **while the title
  screen is still up** — the main menu reads the save into WRAM just to decide whether to offer CONTINUE.
  This harness was fooled by exactly that, and reported a beautifully wrong parity failure. The emulator
  *screenshot* is what gave it away: WRAM said Pallet Town, the screen said "Pokémon — Red Version".
- **`wOverworldMap` is the honest signal.** Nothing fills it but `LoadTileBlockMap`, and nothing calls that
  but `EnterMap` — so it stays blank right up until the player is genuinely standing on the map.
- **A is only ever pressed before the map loads** (A is what takes CONTINUE). Once we're on the overworld,
  **no button is touched at all** — A there would talk to a sign or an NPC and change the very state we came
  to measure.

### A trap worth knowing: bit 7 of `wCurMapTileset`

The **live** `wCurMapTileset` carries `BIT_NO_PREVIOUS_MAP` in bit 7 (`home/overworld.asm`), so Overworld
reads back as **128**, not 0. The save file stores the id clean, which is why the editor never sees it. The
dumper masks it. (Our `.sav` fixtures were checked: bit 7 clear.)

## What it caught immediately

On its first honest run, the console agreed with us about the view pointer, the map's blocks, the 24×20
scratch area and the 20×18 screen — and disagreed about **the border ring**.

We fill the ring with the map's border block. The game does that *too*, and then bleeds the **connected
maps' edge strips** over the top (`LoadNorthSouthConnectionsTileMap` / `LoadEastWestConnectionsTileMap`).
Pallet Town connects north to Route 1 and south to Route 21, so its ring is really Route 1's bottom rows and
Route 21's top rows — not a wall of trees.

That is the connection-strips feature, and it is the next step of the map emulator. It is held as an
explicit **`QEXPECT_FAIL`**, not a skip: the moment connections land, the test starts passing and the
expectation *must* be deleted — so the gap cannot be quietly forgotten.

## Save files: you may invent them

Twilight has said (2026-07-12) that **creating `.sav` files to experiment with is allowed**. Purpose-built
saves are how this harness gets interesting: put the player on a map edge, in a cave, in a copy/glitch map,
next to a connection — and check the console still agrees. Keep experiments in `tmp/` (git-ignored) unless a
fixture earns a permanent place in `assets/saves/`, and never modify the natural saves in place.

## The audio tools (2026-07-12)

Two more scripts hang off the same ROM, same rules (local-only, exit 2 without it):

| Script | What it does |
|--------|--------------|
| `scripts/emu/analyze_music_ids.py` | **Static.** Parses all 256 music ids × 3 audio banks straight out of the cartridge exactly as `AudioN_PlaySound` does (`SFX_Headers_N + id × 3`), disassembles each channel's command stream, follows `sound_call`/`sound_loop`, and classifies every id. Writes `tmp/music_ids.json`. |
| `scripts/emu/probe_glitch_music.py` | **Live.** Patches a music id/bank into a real save (**recomputing the checksum** — without it the game rejects the save and you end up silently testing a *new game*, which fooled this once), boots the cartridge, walks onto the map, and reads back `wChannelSoundIDs`, `wAudioROMBank`, NR51/NR52 and the engine's own pitch bytes at `$C066`. |

Two traps worth remembering, both learned the hard way here:

- **PyBoy reads battery RAM from `<rom>.gb.ram`.** Passing a `.sav` path to your own script does nothing;
  you must copy it next to the ROM copy. Otherwise the game just starts a **new game** and happily reports
  Pallet Town's music — a green-looking result that means nothing.
- **The APU's frequency registers are write-only** (they read back `$FF`). To watch a tune move, read the
  *engine's own* copy: `wChannelFrequencyLowBytes` at `$C066`.

What they found: [`glitch-music.md`](glitch-music.md) — including the fact that a bad music **bank** makes
the console execute arbitrary cartridge bytes as code and **hang**, which the probe demonstrates.

## Where this goes next

The same oracle answers the questions still open:

- **Connection strips** — implement, then delete the `QEXPECT_FAIL`. Verification is already written.
- **Palettes / "contrast"** — `screen.png` is the ground truth. The 4 contrast levels and the 6 glitch
  palettes can be checked against real emulator output instead of reasoned about.
- **The player sprite, warps, signs, animation frames** — all visible in the framebuffer.
- **Save-file acceptance** — hand the game a save the *editor wrote* and prove the console loads it and
  agrees with every field. The editor's byte-fidelity promise, checked by the machine that has to honour it.
