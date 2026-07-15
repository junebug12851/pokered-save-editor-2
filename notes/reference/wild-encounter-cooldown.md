# The wild-encounter cooldown flag (`wStatusFlags2` bit 0) — v2's `pauseMons3Steps`

**Status:** researched **and console-verified** 2026-07-15. Real name/address/bit verified against the
`pret/pokered` disassembly; persistence + the "3 steps" effect **settled on the real cartridge**
(`scripts/emu/probe_wild_encounter_cooldown.py`, §4). Read this before putting the checkbox on the map
details page. The design that consumes it: [`../plans/map-screen.md`](../plans/map-screen.md) → the map
details page (Phase 8 territory — but **only this one flag** is briefed; the encounter tables are not).

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format + the `wram = sav + 0xAD54`
> maths), [`gen1-sound-engine.md`](gen1-sound-engine.md) (the *other* bit of this same byte — bit 1 =
> `BIT_NO_AUDIO_FADE_OUT`), and [`emulator-verification.md`](emulator-verification.md) (the oracle).

---

## 0. The one-paragraph version

The save byte v2 calls **`pauseMons3Steps`** (`AreaPokemon`, save `0x29D8` bit 0) is **`wStatusFlags2`
bit 0**, whose real name in `pret/pokered` is **`BIT_WILD_ENCOUNTER_COOLDOWN`**. It is the game's
**post-battle wild-encounter cooldown**: the console **sets it after *every* battle** (`end_of_battle.asm`),
and when a map is entered (`EnterMap`) *if the bit is set* the game loads
**`wNumberOfNoRandomBattleStepsLeft = 3`** — commented in the source as "minimum number of steps between
battles". Each completed step then decrements that counter, and when it reaches 0 the bit clears itself.
So in **save-editing terms** the effect of setting the bit is exact and simple: **on the next Continue,
the player gets 3 steps with no wild encounters.** The flag is **durable** — it is **not** rewritten on
load (console-verified) — so an edit sticks. v2's name/description ("Suppress encounters for 3 steps") is
**accurate in effect** but hides what the flag really *is* (a post-battle cooldown, normally set
automatically, self-clearing); this note is the truth-in-labelling record and the honest user wording.

**The address maths** (identical to warps/player/npc): `wram = sav + 0xAD54`, so `0x29D8` → **`$D72C`** =
`wStatusFlags2`. Same byte as the audio-fade flag, one bit over — **no collision** (`getBit(0x29D8, 1, 0)`
vs `getBit(0x29D8, 1, 1)`).

---

## 1. What the console actually does (verified against the disassembly)

`constants/ram_constants.asm` — `wStatusFlags2`:

```
; wStatusFlags2
	const_def
	const BIT_WILD_ENCOUNTER_COOLDOWN ; 0   <- v2's pauseMons3Steps
	const BIT_NO_AUDIO_FADE_OUT       ; 1   <- v2's noAudioFadeout (AreaAudio)
```

**Set — after every battle** (`engine/battle/end_of_battle.asm`):

```
	ld hl, wStatusFlags2
	set BIT_WILD_ENCOUNTER_COOLDOWN, [hl]
```

**Read + the "3" applied — on map entry** (`home/overworld.asm`, `EnterMap`, which the Continue path
reaches — see §2):

```
	ld hl, wStatusFlags2
	bit BIT_WILD_ENCOUNTER_COOLDOWN, [hl]
	jr z, .skipGivingThreeStepsOfNoRandomBattles
	ld a, 3 ; minimum number of steps between battles
	ld [wNumberOfNoRandomBattleStepsLeft], a
.skipGivingThreeStepsOfNoRandomBattles
```

**Counted down — per step** (`home/overworld.asm`, step-counting):

```
	ld a, [wStatusFlags2]
	bit BIT_WILD_ENCOUNTER_COOLDOWN, a
	jr z, .doneStepCounting
	ld hl, wNumberOfNoRandomBattleStepsLeft
	dec [hl]
	jr nz, .doneStepCounting
	ld hl, wStatusFlags2
	res BIT_WILD_ENCOUNTER_COOLDOWN, [hl]   ; clears itself when the counter hits 0
.doneStepCounting
```

`wram.asm` states it outright: *"after a battle, you have at least 3 steps before a random battle can
occur"* (`wNumberOfNoRandomBattleStepsLeft`). And `TryDoWildEncounter` (`home/overworld.asm:13`) bails
while the cooldown is active, which is *how* the 3 steps stay encounter-free.

**Two counters, and only the flag is in the save.** `wStepCounter` (`$D13B`) and
`wNumberOfNoRandomBattleStepsLeft` (`$D13C`) live **below** `wMainDataStart` (`$D2F7`), so **neither is
saved**. The save carries **only the flag** (`$D72C` bit 0); the "3" is a ROM constant re-applied at
every map entry. That is why editing this is a **checkbox, not a number** — there is no per-save step
count to edit.

---

## 2. Why it survives a Continue (the persistence question)

The Continue path is `SpecialEnterMap` → (falls through to) **`EnterMap`** (`engine/menus/main_menu.asm`
ends `SpecialEnterMap` with `jp EnterMap`). `SpecialEnterMap` zeroes `wCableClubDestinationMap` (the byte
that *aliases* `wStatusFlags3`, which is why **that** byte dies on load — see
[`warps.md`](warps.md)/[`player-state.md`](player-state.md)) but **does not touch `wStatusFlags2`**.
`ClearVariablesOnEnterMap` doesn't touch it either. So the loaded bit reaches `EnterMap` intact and the
"give 3 steps" branch runs on it. **Prediction: durable, and honoured on load.** Confirmed on the
cartridge (§4).

---

## 3. What this means for OUR model

- **No save-corruption bug, no loaded gun.** `AreaPokemon::load()/save()` read and write `0x29D8` bit 0
  correctly (`getBit/setBit(0x29D8, 1, 0)`), preserving the rest of the byte (the audio-fade bit
  included). `randomize()`/`reset()` clear it; `setTo()` doesn't touch it. All correct.
- **It is a truth-in-labelling matter only.** The name `pauseMons3Steps` and the doc "Suppress
  encounters for 3 steps" describe the *effect* but not the *thing*. Recommended: keep the field, add a
  doc comment naming it `BIT_WILD_ENCOUNTER_COOLDOWN` and explaining set-after-battle / self-clearing;
  optionally rename the property to `wildEncounterCooldown` (Twilight's call — guarded naming; low blast
  radius, only `areapokemon.{h,cpp}` + `tst_area_pokemon` reference it).
- **Persistence marking: DURABLE.** Unlike the warp/player/npc scratch, this flag is **kept** on load —
  so on the map details page it gets **no yellow "!"**. The honest note is the opposite: an edit sticks,
  and the game will act on it on the very next Continue.
- ⚠️ **The clean fixture already carries it set.** `BaseSAV` reads back bit 0 = 1 / steps-left = 3 (§4) —
  it was saved right after a battle. So "on by default" is *normal*, not an edit; a UI that flagged a set
  bit as unusual would cry wolf on the base save.

**The label (Twilight, 2026-07-15):** **"3-step wild encounter cooldown"**, with the honest one-line
note: *"Gives 3 encounter-free steps when the save loads — the game's post-battle cooldown. Normally set
automatically right after a battle, and it clears itself once you've walked those steps off."*
**The property is renamed `wildEncounterCooldown`** (Twilight, 2026-07-15) with a doc comment naming
`BIT_WILD_ENCOUNTER_COOLDOWN`.

---

## 4. The console's testimony

`scripts/emu/probe_wild_encounter_cooldown.py` stamps the bit (and a cleared control), boots the **real
ROM** headless, lands in the overworld **before any movement**, and reads back the flag +
`wNumberOfNoRandomBattleStepsLeft`. Verbatim:

```
  A  baseline (untouched save)
    wStatusFlags2                 0x01
    BIT_WILD_ENCOUNTER_COOLDOWN   1
    wNumberOfNoRandomBattleStepsLeft  3
  B  cooldown bit SET in the save
    BIT_WILD_ENCOUNTER_COOLDOWN   1      (kept)
    wNumberOfNoRandomBattleStepsLeft  3
  C  cooldown bit CLEARED in the save (control)
    BIT_WILD_ENCOUNTER_COOLDOWN   0
    wNumberOfNoRandomBattleStepsLeft  0

  VERDICT
    Flag survives Continue?              YES -- read back set
    EnterMap granted 3 no-battle steps?  YES -- wNumberOfNoRandomBattleStepsLeft == 3
    Control (bit clear) steps-left == 0? YES
```

The **cleared control** does double duty: steps-left = 0 with the bit clear proves the effect is *caused*
by this bit and confirms the `$D13C` address (the value swings 3 ↔ 0 exactly with it).

---

## 5. Sources

| What | Where |
|---|---|
| v2's model (the flag) | `projects/savefile/src/pse-savefile/expanded/area/areapokemon.{h,cpp}` (`pauseMons3Steps`, `0x29D8` bit 0) |
| The bit constant + the byte's other bit | `assets/references/pokered/constants/ram_constants.asm` (`wStatusFlags2`: `BIT_WILD_ENCOUNTER_COOLDOWN` 0, `BIT_NO_AUDIO_FADE_OUT` 1) |
| Set after battle | `assets/references/pokered/engine/battle/end_of_battle.asm` |
| Read + "3 steps" applied on entry; per-step decrement + self-clear; `TryDoWildEncounter` gate | `assets/references/pokered/home/overworld.asm` (`EnterMap`; step-counting; line 13) |
| Continue path `SpecialEnterMap` → `EnterMap` | `assets/references/pokered/engine/menus/main_menu.asm` |
| `ClearVariablesOnEnterMap` doesn't touch `wStatusFlags2` | `assets/references/pokered/engine/overworld/clear_variables.asm` |
| The two counters + the "3 steps" comment | `assets/references/pokered/ram/wram.asm` (`wStepCounter`, `wNumberOfNoRandomBattleStepsLeft`) |
| **The persistence + effect verdict** | ✅ `scripts/emu/probe_wild_encounter_cooldown.py` (§4) |
