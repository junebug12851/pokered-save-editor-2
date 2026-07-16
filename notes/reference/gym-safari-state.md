# Gym & Safari minigame state — six saved event bytes

_The Vermilion Gym trash‑can switches, the Cinnabar Gym "wrong answer" opponent, and the Safari
Zone run counters. Six bytes Twilight asked to put on a new **Map Storage** panel (2026‑07‑15).
Verified against `pret/pokered`, cross‑checked against the v1 editor's own offsets, and
**console‑verified** (see below)._

## The headline — global state variables that are clearly map‑specific

Twilight's mental model is the right one: these are **global state variables that are clearly
map‑specific** — each byte is stored once, save‑wide, but belongs unambiguously to **one map**
(Vermilion Gym, Cinnabar Gym, the Safari Zone). That "global storage, map‑specific meaning" split is
exactly why they get a **Map Storage** panel keyed by map. Two refinements to the first‑pass wording,
and only two:

- **They're persistent save data, not RAM scratch.** They live in the game's global **Main Data**
  block (the same saved region as the badges, event flags and the play‑time clock), and the byte you
  see in the `.sav` **is** the durable copy — nothing is mirrored somewhere else. So "is the scratch
  saved persistently?" — there's no separate scratch; this is the persistent place. Their *effect* is
  scoped to one activity, but the storage itself endures.
- **Stored as global singletons, not a per‑map array.** There's exactly one of each — not one copy
  per map — so the panel is a **view** that files each global byte under its owning map, rather than a
  per‑map Area field. (The bytes are **not** in the per‑map Area block `0x25Fx`–`0x3522`.) The
  map‑specificity is real; it just isn't expressed by a per‑map slot in the save.

v1 read this the same way — it filed them under `AreaPlayer` (safari steps/balls/game‑over) and
`AreaPuzzle` (the two trash cans + the Cinnabar opponent), i.e. grouped by the map/activity they
belong to. Same instinct; we just keep the model home **global** and let the panel do the per‑map
grouping.

## The six bytes (all offsets **verified**; behaviour from the disassembly, probe owed)

`file` = byte offset in the 32 KB `.sav`. `WRAM` = Game Boy address. The map is linear across Main
Data: **`WRAM = file + 0xAD54`** (anchored on `wMainDataStart` `0x25A3→0xD2F7` and `wCurMapScript`
`0x2CE5→0xDA39`). All four that v1 also modelled match v1's offsets exactly.

| Twilight's name | pokered name | file | WRAM | size | v1 home |
|---|---|---|---|---|---|
| Trashcan switch 1 | `wFirstLockTrashCanIndex` | `0x29EF` | `0xD743` | 1 | `AreaPuzzle.firstTrashcanLock` |
| Trashcan switch 2 | `wSecondLockTrashCanIndex` | `0x29F0` | `0xD744` | 1 | `AreaPuzzle.secondTrashcanLock` |
| Cinnabar next wrong answer | `wOpponentAfterWrongAnswer` | `0x2CE4` | `0xDA38` | 1 | `AreaPuzzle.oppAfterWrongAnsw` |
| Safari steps left | `wSafariSteps` | `0x29B9` | `0xD70D` | **2** | `AreaPlayer.safariSteps` |
| Safari game‑over flag | `wSafariZoneGameOver` | `0x2CF2` | `0xDA46` | 1 | `AreaPlayer.safariGameOver` |
| Safari balls | `wNumSafariBalls` | `0x2CF3` | `0xDA47` | 1 | `AreaPlayer.safariBallCount` |

### ⚠️ Two traps in that table

1. **`wSafariSteps` is a 2‑byte word stored BIG‑endian** — the game writes `HIGH(502)` to
   `0xD70D` then `LOW(502)` to `0xD70E` (`scripts/SafariZoneGate.asm`, and the decrement in
   `safari_game.asm`). Most of the save is little‑endian; this one is not. A little‑endian word read
   (v1's `getWord(0x29B9)`) reads the **high byte first** and gets a wrong number — **check v2 reads
   this high‑byte‑first.** Legal fresh value is **502** (`0x01F6`).
2. **`wGymTrashCanIndex` (`0xCD0D`) is a decoy — it is NOT saved.** It sits in general scratch WRAM
   (the `"WRAM"` section, well outside Main Data) and only holds *the can you just bumped into* for
   the length of one `GymTrashScript` call. Do **not** confuse it with the two saved lock indices.

## What each one actually does, and when it matters (the "armed vs inert" cut)

The recurring question — *"are they rewritten or used even in their respective maps?"* — has the
same shape as warps/player‑state: a byte is **durable** in the file, but only **armed** (actually
consulted by the console) inside a narrow window; outside it, the game **re‑initialises** the byte
the next time you enter, so an edit is stored but inert.

### Vermilion Gym trash cans — `wFirstLockTrashCanIndex`, `wSecondLockTrashCanIndex`
The two‑switch puzzle. `engine/.../vermilion_gym_trash.asm` + `scripts/VermilionCity.asm`:
- **First** lock index is **randomised** (`and $e` → an even can 0–14) by the gym's
  `.setFirstLockTrashCanIndex` script step, and **re‑randomised** on a wrong second‑lock guess.
- **Second** lock index is **derived from a ROM table** off the first, the moment you open the first
  lock (and written to `0xD744`).
- Both are **read every time you flip a switch** to check "is this the right can?".
- **Armed** iff you save **mid‑puzzle inside Vermilion Gym**. Elsewhere the gym script overwrites the
  first index on entry, so an edit is inert. (There's a famous ROM bug here — trash can 0 can hold
  the second lock regardless of the first — but it doesn't affect the save bytes.)

### Cinnabar Gym — `wOpponentAfterWrongAnswer`
The trainer you must battle if you get the **next** quiz‑gate answer wrong. `scripts/CinnabarGym.asm`:
- `CinnabarGymResetScripts` **zeros it** (alongside `wCurMapScript`) when the gym resets its scripts —
  i.e. on (re)entry and after a battle.
- `CinnabarGymDefaultScript` **reads it**: non‑zero ⇒ spawn that trainer to battle.
- **Armed** iff you save **mid‑quiz inside Cinnabar Gym**. (The disassembly notes the same write in
  `PokemonTower7F.asm` is *"not used here; likely a mistake copied from CinnabarGym"* — dead there.)

### Safari Zone — `wSafariSteps`, `wNumSafariBalls`, `wSafariZoneGameOver`
The 500‑step / 30‑ball run. `scripts/SafariZoneGate.asm` + `engine/.../safari_game.asm`:
- Entering through the gate **sets `wSafariSteps = 502` and `wNumSafariBalls = 30`**; leaving sets
  balls to **0**. So both counters are **re‑initialised on every entry** — editing them only matters
  while a save **sits inside the Safari Zone** (`EVENT_IN_SAFARI_ZONE` set).
- `wSafariSteps` decrements per step; `wNumSafariBalls` decrements per throw; either hitting 0 ends
  the run.
- **`wSafariZoneGameOver` is genuinely transient — and it never survives a Continue.**
  `OverworldLoop` **`farcall`s `SafariZoneCheck` every frame** (`home/overworld.asm`), and when
  `EVENT_IN_SAFARI_ZONE` is not set it jumps straight to `SafariZoneGameStillGoing`, which
  **`xor a; ld [wSafariZoneGameOver], a`** — i.e. **zeroes it unconditionally**. Inside the zone it's
  rewritten per step (`0` while the run continues, `1` at game‑over, which then warps you out). Either
  way an edit is overwritten **within a frame of gaining control**. **Console‑verified** (below):
  written `1`, read back `0` in Pallet Town before any step. This is the one byte here that is **truly
  inert** — it belongs in the *"rewritten on load/use — editing does nothing"* grouping with an amber
  **!**, exactly like the reloaded‑on‑load player bytes.

### Does anything wipe them on Continue?
For **five of the six**, no — no load‑time zeroing touches them (unlike `wStatusFlags3`, which shares
a byte with a cleared variable). They're plain Main‑Data event bytes, so **on the Continue where you
saved they survive as‑is** (console‑verified) — the "reset" is always a *gameplay script* on area
(re)entry, never the load. That's what makes an in‑zone / in‑puzzle edit live. **The exception is
`wSafariZoneGameOver`**, which `OverworldLoop` zeroes every frame outside the zone — it does **not**
survive Continue (console‑verified).

## ✅ Console‑verified (`scripts/emu/probe_gym_safari_state.py`, 2026‑07‑15)
Boot the real ROM (PyBoy, headless) with a BaseSAV where all six bytes were written to distinctive
values (checksum re‑sealed), land in the overworld (Pallet Town — not the zone/a gym), read WRAM back
before any movement:

| byte | written | read back | verdict |
|---|---|---|---|
| `wFirstLockTrashCanIndex` | `0x0A` | `0x0A` | ✅ survives; address exact |
| `wSecondLockTrashCanIndex` | `0x0C` | `0x0C` | ✅ survives; address exact |
| `wOpponentAfterWrongAnswer` | `0x07` | `0x07` | ✅ survives; address exact |
| `wSafariSteps` | `502` (hi `01` lo `F6`) | `502` (hi `01` lo `F6`) | ✅ survives; **big‑endian confirmed** |
| `wNumSafariBalls` | `30` | `30` | ✅ survives; address exact |
| `wSafariZoneGameOver` | `0x01` | **`0x00`** | ⚠️ **zeroed on load** (OverworldLoop → SafariZoneCheck) |

So: **five are durable** (edit survives Continue, addresses + endianness pinned); **`wSafariZoneGameOver`
is inert** (forced to 0 within a frame). The probe also caught BaseSAV's resting values —
`wFirstLockTrashCanIndex=4`, `wSecondLockTrashCanIndex=7`, `wSafariSteps=468`, `wNumSafariBalls=30` —
so **BaseSAV already carries nonzero safari/trash state** (worth knowing for fixtures/tests).

Not automated (script‑driven; needs an in‑zone/mid‑puzzle save + real movement): the 502/30 re‑set on
gate entry, the per‑step decrements, the trash‑can/opponent re‑randomise — read from the disassembly,
they define the *armed window*, not the persistence claim the probe pins.

## What this means for our model & the panel

- **Home:** these are **global**, not `Area*`. They do not belong on `AreaMap`/the Area block. In v2
  terms they want a global home (`WorldGeneral`/`WorldLocal`/a small new "minigame state" class), and
  `MapModel` reads them from there — the panel is a *view*, not a per‑map object. (Design decision
  for Twilight — see the report.)
- **A "Maps" panel is fine as a UI grouping** (by location: Vermilion Gym, Cinnabar Gym, Safari Zone)
  even though the bytes are global — but the panel should be honest that it's showing **global run
  state**, with a plain note of the **armed window** for each (like the warps "live on Continue /
  restored on re‑entry" line), and `wSafariZoneGameOver` behind the reloaded/inert treatment.
- **Full byte range, hack values shown not refused** (project rule). Sensible real values for the
  helpers: trash cans 0–14 (first even), safari balls 0–30, steps 0–502 (big‑endian!).
- **Every setter writes only its own byte(s)** — byte‑fidelity as always; the word write for steps
  touches exactly `0x29B9`+`0x29BA`.

## Sources
- `pret/pokered`: `ram/wram.asm`, `ram/sram.asm`, `scripts/VermilionCity.asm`,
  `engine/events/hidden_events/vermilion_gym_trash.asm`, `scripts/CinnabarGym.asm`,
  `scripts/SafariZoneGate.asm`, `engine/events/hidden_events/safari_game.asm`, `engine/.../overworld.asm`.
- v1 editor clone: `AreaPuzzle.ts`, `AreaPlayer.ts` (offset cross‑check).
