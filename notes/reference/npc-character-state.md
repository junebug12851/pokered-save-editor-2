# The Character-state flags — the map-global NPC / control / battle bits (`AreaNPC`)

**Status:** researched **and console-verified** 2026-07-15. Real names, bytes, bits verified against
the `pret/pokered` disassembly; the **persistence of all nine settled on the real cartridge**
(`scripts/emu/probe_npc_character_state.py`, output in §5). Read this before designing or building the
right-hand **Character panel**. The design that consumes it is
[`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 9 (Character State)**.

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format),
> [`player-state.md`](player-state.md) (the `wStatusFlags3` whole-byte wipe and the load path live
> there too — this probe independently re-confirmed it), [`warps.md`](warps.md) (the same wipe kills
> warp flags), and [`emulator-verification.md`](emulator-verification.md) (how the console gets asked).

---

## 0. The one-paragraph version

v1's **"NPC" page** (`area-npcs`) exposed **eight checkboxes + a hex field**, grouped Sprites /
Controls / Battle, and a hidden ninth field the UI never drew. These are **not per-NPC** — each is a
single **map-global** bit (or, for the trainer pointer, a 2-byte word) that governs how *all* the
map's characters behave. v2 ported the page's model (`areanpc.{h,cpp}`) **verbatim, wrong names and
all.** This pass took each of the nine to `pret/pokered`, found that **three of the labels are wrong
or misleading** and the two `wd730` source-comments are off, then **asked the cartridge** what
survives a Continue. The console's answer overturned the natural source-read: it is *not* "all
transient bits vanish". **Four are wiped on load (momentary); five keep their raw value across the
save (durable).** That split is the whole point of the panel — every field is editable (hack values
included, per the project rule — and there are **no hidden fields**, Twilight 2026-07-15), but each
row must say whether an edit sticks or is erased on the next load.

**The address maths** (identical to warps/player): the save's Main Data block starts at file offset
`0x25A3` = `wMainDataStart` = `$D2F7`, so

> **`WRAM address = save offset + 0xAD54`**

which resolves v1's four flag bytes to `0x29D9`→`$D72D` **`wStatusFlags3`**, `0x29DA`→`$D72E`
**`wStatusFlags4`**, `0x29DC`→`$D730` **`wStatusFlags5`**, `0x29DF`→`$D733` **`wStatusFlags7`**, and
`0x2CDC`→`$DA30` **`wTrainerHeaderPtr`** (inside the saved block; `wram.asm` 1751–2223). v1's
comments guessed `wd730` for two of these; `pret` renamed old `wd730` → `wStatusFlags5`.

---

## 1. The complete table — every character-state field, one row each

**Legend (console-verified):** **`!`** = **rewritten on load** — an edit is *momentary* (the game
zeroes/clears it the instant the save loads); gets the yellow-exclamation treatment. **`kept`** = the
raw value **survives the Continue** — an edit is durable (it is still scratch the game manages during
play, but it persists across a load). **`DBG`** = a flag only the disassembly's debug menu ever sets.

| | Save | Bit | Real WRAM name | new field name | v1 label | What it really is | On load ✅ |
|---|---|---|---|---|---|---|---|
| **!** | `0x29D9` | 0 | `BIT_INIT_TRADE_CENTER_FACING` (`wStatusFlags3`) | `initTradeCenterFacing` | *(hidden)* | Link **Trade Center** only: the two trade sprites have been turned to face each other. Meaningless outside a cable trade. | **zeroed** (whole byte) |
| **!** | `0x29D9` | 5 | `BIT_NO_NPC_FACE_PLAYER` | `npcsDoNotFacePlayer` | Face Away on Interaction | While set, NPCs **do not turn to face you** when talked to. (Not "turn away" — "don't turn".) | **zeroed** (whole byte) |
| `kept` | `0x29DA` | 7 | `BIT_INIT_SCRIPTED_MOVEMENT` (`wStatusFlags4`) | `initScriptedMovement` | Scripted Movement Init | "A scripted movement is **starting**." Set/cleared around `auto_movement`. **Survives the load.** | **kept** |
| `kept` | `0x29DC` | 0 | `BIT_SCRIPTED_NPC_MOVEMENT` (`wStatusFlags5`) | `scriptedNpcMoving` | Scripted Movement Running | "A sprite is **currently being walked by a script**." Set by `pathfinding`/`trainers`. **Survives.** | **kept** |
| **!** | `0x29DC` | 5 | `BIT_DISABLE_JOYPAD` (`wStatusFlags5`) | `disableJoypad` | Ignore Controls | Player input is **ignored** (a cutscene has the controls). **Cleared on load** so you get control back. | **cleared** |
| **!** | `0x29DC` | 7 | `BIT_SCRIPTED_MOVEMENT_STATE` (`wStatusFlags5`) | `scriptedMovementActive` | Scripted Controls | "**Executing** a scripted-movement sequence" (the running state). **Cleared on load.** Not joypad *simulation*. | **cleared** |
| `kept` DBG | `0x29DF` | 0 | `BIT_TEST_BATTLE` (`wStatusFlags7`) | `testBattle` | Scripted Battle | **Debug test-battle** flag — set only by `debug_menu.asm`. Not "scripted battle". `wStatusFlags7` isn't wiped, so the raw bit **survives**. | **kept** |
| `kept` | `0x29DF` | 3 | `BIT_TRAINER_BATTLE` (`wStatusFlags7`) | `trainerBattle` | Trainer Battle | "The battle being entered is a **trainer** battle." Set in `trainers.asm`; cleared post-battle. Raw bit **survives** a load. | **kept** |
| `kept` | `0x2CDC`–`0x2CDD` | — (dw) | `wTrainerHeaderPtr` | `trainerHeaderPtr` | Trainer Pointer | 2-byte **pointer into the map's trainer-header data** for the trainer being fought. Scratch, but the stored word **survives** (sentinel `0x5AA5` read back intact). | **kept** |

**Two things the table makes plain, both now console-checked:**

1. **`wStatusFlags3` is wiped whole on every Continue** — `SpecialEnterMap` writes 0 to
   `wCableClubDestinationMap`, the same byte as `wStatusFlags3`. So **both** "Sprites" bits die on
   load. The probe saw both read back 0 — an independent second confirmation of the
   [`player-state.md`](player-state.md) result from this byte's other bits.
2. **Not everything transient is erased.** Five of the nine (`initScriptedMovement`,
   `scriptedNpcMoving`, `testBattle`, `trainerBattle`, `wTrainerHeaderPtr`) **keep the raw value you
   write** across a load, because nothing on the Continue path clears them; only `wStatusFlags5`'s two
   control bits are actively cleared (to hand you the controls) alongside the `wStatusFlags3` wipe.
   **A source-read alone would have mis-predicted this** — the reason the doctrine says to ask the
   console (see §5, and the sprite-pass precedent).

---

## 2. What each group actually governs (so the panel can explain it)

- **Sprites group** (`wStatusFlags3` bits) — cutscene facing. `npcsDoNotFacePlayer` suppresses the
  turn-to-face-you courtesy; `initTradeCenterFacing` is a link-trade bookkeeping bit with no meaning
  in a single-player save. **Both zeroed on load** — editable, but momentary.
- **Controls group** — the machinery of **scripted / forced-walk cutscenes**: an *init* pulse
  (`initScriptedMovement`, kept), a *running* state (`scriptedMovementActive`, cleared), the
  *NPC-is-walking* flag (`scriptedNpcMoving`, kept), and *ignore-the-player's-buttons* (`disableJoypad`,
  cleared). None is a durable "setting" in spirit, but the save keeps two of them and clears two.
- **Battle group** — `trainerBattle` marks the pending battle as a trainer's; `trainerHeaderPtr` says
  *which* trainer; `testBattle` is a debug switch. All scratch — you can't save mid-battle — but all
  three (and the pointer) **survive** the load as raw values.

The honest headline for a user: **none of these nine is a thing a real game leaves set; two the game
erases the instant you load, two more it clears to give you back the controls, and the other five it
simply keeps.** Every one is editable — no hidden fields — and the panel owes that truth next to each.

---

## 3. The bugs this found in the v2 model (fixed in Phase 9b)

`areanpc.{h,cpp}` is a straight port of v1, so it carried v1's guesses. The **byte offsets and bit
numbers were all correct**; the **names and the two `wd730` source-comments were the problem.**

| | v2 had | The truth | new name |
|---|---|---|---|
| 1 | `runningTestBattle` — *"Scripted Battle"* | **`BIT_TEST_BATTLE`** — a **debug** flag (`debug_menu.asm`), nothing to do with scripted battles. | `testBattle` |
| 2 | `joypadSimulation` — *"Scripted Controls"* | **`BIT_SCRIPTED_MOVEMENT_STATE`** — the *executing-a-scripted-movement* state bit, not joypad simulation. | `scriptedMovementActive` |
| 3 | `npcsFaceAway` — *"Face Away on Interaction"* | **`BIT_NO_NPC_FACE_PLAYER`** — NPCs **don't turn to face** you; not "turn away". | `npcsDoNotFacePlayer` |
| 4 | `scriptedNPCMovement` vs `npcSpriteMovement` | Conflatable: the first is **`BIT_INIT_SCRIPTED_MOVEMENT`** (`wStatusFlags4` b7), the second **`BIT_SCRIPTED_NPC_MOVEMENT`** (`wStatusFlags5` b0). | `initScriptedMovement`, `scriptedNpcMoving` |
| 5 | `ignoreJoypad` / `tradeCenterSpritesFaced` | `BIT_DISABLE_JOYPAD` / `BIT_INIT_TRADE_CENTER_FACING`. | `disableJoypad`, `initTradeCenterFacing` |
| 6 | source comment: bits live in `wd730` | Two of the four bytes are **`wStatusFlags5`**/`wStatusFlags7` (`0x29DC`/`0x29DF` → `$D730`/`$D733`); `pret` renamed old `wd730` → `wStatusFlags5`. | — |
| 7 | every field implied durable | **Four are rewritten on load** (console-verified). The panel must mark each. | — |

This is **not** a save-corruption bug — `AreaNPC::save()` wrote the right bits to the right addresses,
and (unlike `AreaWarps::setTo()`) there is **no loaded gun**: `randomize()`/`setTo()` are no-ops here.
It was a **truth-in-labelling + persistence-honesty** fix; the rename + persistence-doc landed in
Phase 9b, offsets/bits unchanged, save output **byte-identical**.

---

## 4. How the panel should carve this up (Phase 9d)

Three titled `FieldGroup`s (v1's own split, which maps cleanly to the flag bytes), **every field
shown — none hidden**, each a toggle with a one-line plain-English "what this does" and the
console-verified persistence mark:

- **Sprites** — *Don't face player on talk* (`!` zeroed on load), *Trade-center sprites faced* (`!`
  zeroed on load; link-trade only — say so). Both first-class controls; the `!` is a *label*, not a
  reason to hide them. (They may still sit behind the warp/player `[ ⚠ rewritten on load ]` disclosure
  as a *grouping*, but the disclosure is opened by one click and the fields are always present.)
- **Controls** — *Scripted movement — init* (kept), *— running* (kept), *Ignore player input*
  (`!` cleared), *Scripted-movement active* (`!` cleared) — each with the "scripted-cutscene
  machinery" note and its persistence mark.
- **Battle** — *Trainer battle* (kept), *Test battle* (kept; **debug** — labelled as such), and
  **Trainer pointer** (`wTrainerHeaderPtr`, kept). Per the `area-map-state` pointer doctrine
  (2026-07-15): a raw address is the **last** resort — resolve the pointer to the map's trainer-header
  entry it names where we can; raw hex behind "Something else…".

**Every value stays full-range and editable, hack values included, and nothing is hidden** — the
project rule (and Twilight's explicit 2026-07-15 "there shouldn't be hidden fields"). The panel never
refuses and never silently rewrites; it only tells the truth about what the console does with each on
the next load. Because these are **map-global** (not per-object) they belong in the Area-state dock
panel, not the on-canvas per-NPC inspector (Phase 4b) — the plan keeps the two apart.

---

## 5. The console's testimony

`scripts/emu/probe_npc_character_state.py` stamps **all nine** values to 1 (and the pointer to a
sentinel `0x5AA5`), seals the checksum, boots the **real ROM** headless, lands in the overworld, and
reads back each bit/word. Verbatim:

```
===== Character-state flags on Continue (all stamped to 1) =====
  SF3.b0 initTradeCenterFacing   wrote 1  read 0   REWRITE
  SF3.b5 npcsDoNotFacePlayer      wrote 1  read 0   REWRITE
  SF4.b7 initScriptedMovement     wrote 1  read 1   kept
  SF5.b0 scriptedNpcMoving        wrote 1  read 1   kept
  SF5.b5 disableJoypad            wrote 1  read 0   REWRITE
  SF5.b7 scriptedMovementActive   wrote 1  read 0   REWRITE
  SF7.b0 testBattle (debug)       wrote 1  read 1   kept
  SF7.b3 trainerBattle            wrote 1  read 1   kept
  wTrainerHeaderPtr              wrote 0x5AA5  read 0x5AA5   kept
```

> ⚠️ **The probe earned its keep.** From the source alone the natural guess is "these are all
> transient script/battle scratch, so they'll be gone." Wrong: **five of the nine keep the raw value**.
> Only `wStatusFlags3` (wiped whole) and `wStatusFlags5`'s two control bits (cleared to return the
> controls) are rewritten. This is the same lesson the sprite-persistence pass learned
> ([`sprites.md`](sprites.md) Part 5) — the cartridge is the oracle.

---

## 6. Sources

| What | Where |
|---|---|
| v1's NPC page + its 9-field model | `assets/references/pokered-save-editor/src/app/screens/area-npcs/*` and `.../sections/Area/AreaNPC.ts` |
| v2's model (renamed + doc'd in Phase 9b) | `projects/savefile/src/pse-savefile/expanded/area/areanpc.{h,cpp}` |
| The four flag bytes + `wTrainerHeaderPtr`, in the saved block | `assets/references/pokered/ram/wram.asm` (`wStatusFlags1..7` ~2107-2122; `wTrainerHeaderPtr` 2189; `wMainDataStart` 1751 / `wMainDataEnd` 2223) |
| Every bit constant + comment | `assets/references/pokered/constants/ram_constants.asm` (`wStatusFlags3/4/5/7`, lines 82-132) |
| `wStatusFlags3` whole-byte wipe on Continue | `home/overworld.asm` `SpecialEnterMap`; cross-confirmed in [`player-state.md`](player-state.md) |
| Scripted-movement / joypad bit set + clear | `auto_movement.asm`, `home/movement.asm`, `home/npc_movement.asm`, `pathfinding.asm`, `trainers.asm`, `home/play_time.asm` |
| `BIT_TEST_BATTLE` set only by debug | `engine/debug/debug_menu.asm` (`core.asm` reads it) |
| `BIT_TRAINER_BATTLE` cleared post-battle | `home/overworld.asm` `.battleOccurred` (~line 330) |
| `wTrainerHeaderPtr` written/read | `engine/battle/trainers.asm` |
| **The persistence verdicts, byte by byte** | ✅ `scripts/emu/probe_npc_character_state.py` (§5) |
