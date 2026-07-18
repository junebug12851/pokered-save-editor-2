# The "completed" one-shots — rods, Lapras, the starter, the guards, and one badly named bit {#world-completed}

The research behind [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 19**.

> **Fairy Fox, 2026-07-17:** *"Add in the fishing rod obtained old/good/super checkboxes again on the
> corresponding maps with x/y box. Obtained Lapras / Starter / Ever healed pokemon and for ever
> healed pokemon im guessing the nurse script activates it? … Satisfied safron guards and defeated
> loreli. These need research are they used, scratch, read from, written to. Where and when."*

Eight flags, and the answers are: **all eight are used, all eight are durable persistent save data
(none are scratch), every one is both written and read** — with **one exception that is written and
never read**, and **one whose name is a lie**.

⚠️ **`WorldCompleted` already models all eight, at the correct offsets.** The bits are right. **One
label is wrong**, and it is wrong in a way that would make a checkbox do something close to the
opposite of what it says (§4).

## 1. The one-line answer

| Flag (our name) | Byte · bit | Real name | Where it's SET | Where it's READ | Located? |
|---|---|---|---|---|---|
| `obtainedOldRod` | `0x29D4` b3 | `BIT_GOT_OLD_ROD` | Vermilion Old Rod House (163) | same map | ✅ **(2, 4)** |
| `obtainedGoodRod` | `0x29D4` b4 | `BIT_GOT_GOOD_ROD` | Fuchsia Good Rod House (164) | same map | ✅ **(5, 3)** |
| `obtainedSuperRod` | `0x29D4` b5 | `BIT_GOT_SUPER_ROD` | Route 12 Super Rod House (189) | same map | ✅ **(2, 4)** |
| `satisfiedSaffronGuards` | `0x29D4` b6 | `BIT_GAVE_SAFFRON_GUARDS_DRINK` | Route 5/6/7/8 Gates | all four | ✅ **4 maps** (§3) |
| `obtainedLapras` | `0x29DA` b0 | `BIT_GOT_LAPRAS` | Silph Co 7F (212) | same map | ✅ **(1, 5)** |
| `everHealedPokemon` | `0x29DA` b2 | `BIT_USED_POKECENTER` | the **Poké Center engine** | same routine | ⚠️ **every nurse** (§5) |
| `obtainedStarterPokemon` | `0x29DA` b3 | `BIT_GOT_STARTER` | Oak's Lab (40) | Oak's Lab **+ Red's House 1F (37)** | ✅ **2 maps** (§3) |
| `defeatedLorelei` 🐞 | `0x29E0` b1 | **`BIT_STARTED_ELITE_4`** | **Lorelei's Room, on MAP LOAD** | Indigo Plateau Lobby | ⚠️ **no x/y** (§4) |

**The bytes** (`wram = sav + 0xAD54`), each cross-checked against an already-verified neighbour:

- **`wStatusFlags1` = `$D728` → file `0x29D4`** — confirmed by `AreaPlayer` already reading
  `strengthOutsideBattle`/`surfingAllowed`/`usedCardKey` there.
- **`wStatusFlags4` = `$D72E` → file `0x29DA`** — confirmed by `AreaPlayer`
  (`noBattles`/`battleEndedOrBlackout`/`usingLinkCable`) and `AreaNPC` (`initScriptedMovement`).
- **`wElite4Flags` = `$D734` → file `0x29E0`** — the byte right after `wStatusFlags7` (`0x29DF`,
  verified), and `HallOfFame.asm` **asserts the adjacency itself**:
  `ASSERT wStatusFlags7 + 1 == wElite4Flags`.

## 2. The rods — three bits, three houses, one Fishing Guru each

Identical shape, all three (`VermilionOldRodHouse.asm`, `FuchsiaGoodRodHouse.asm`,
`Route12SuperRodHouse.asm`):

```asm
VermilionOldRodHouseFishingGuruText:
	text_asm
	ld a, [wStatusFlags1]
	bit BIT_GOT_OLD_ROD, a
	jr nz, .got_old_rod          ; already given -> the "how's it going" line
	...
	lb bc, OLD_ROD, 1
	call GiveItem
	jr nc, .bag_full
	ld hl, wStatusFlags1
	set BIT_GOT_OLD_ROD, [hl]    ; only AFTER the item actually lands in the bag
```

⚠️ **The flag means "the Guru has given you this rod", NOT "you have this rod."** The rod itself is an
ordinary bag item. Toss the Old Rod and the flag stays set — the Guru will never give you another.
**So clearing the flag is the repair**: it re-arms the Guru and you can be handed a replacement. That
is the editing effect worth putting in words on the checkbox.

⚠️ **`set` happens only on `GiveItem` success** — a full bag means no rod *and* no flag. The game is
careful here; a save editor that sets the flag without the item silently locks the rod away forever.
**Ticking the box does not put a rod in your bag**, and the UI must say so.

**Alike group: "Fishing rods"** — three different bits, the same kind of thing, one per place.
Exactly leadership's alike-group case.

## 3. Two flags that live on several maps — and they are SHARED, not ALIKE

**This is the distinction the group system rests on, so it is worth being exact:**

- **ALIKE** = *different bits*, same kind of thing (the 3 rods; the 11 towns; the 10 trades).
- **SHARED** = *one bit*, appearing on several map pages (Silph Co's flags across 12 floors).

Both of the following are **SHARED**, not alike:

**`BIT_GAVE_SAFFRON_GUARDS_DRINK`** — **one bit, five sites across four maps**: `Route5Gate.asm`
(**twice** — lines 21/46 and 73/90), `Route6Gate.asm`, `Route7Gate.asm`, `Route8Gate.asm`. Every gate
reads it, and every gate sets it, because handing *any* one guard a drink satisfies *all* of them.
The guard is an `object_event` (Route 5 Gate's is at **(1, 3)**), so each gate gets a real box on its
own map — **the same bit, five places**.

**`BIT_GOT_STARTER`** — **one bit, two maps**: set + read in `OaksLab.asm`, and read in
`RedsHouse1F.asm` (line 12 — Mum's "you should go see Prof. Oak" gate). One bit, two pages.

> ⭐ **Leadership called this in advance** — *"these may be called from multiple maps and liekly need
> grouping too group alike"*. The instinct was right; the group kind is **shared**. Which is a good
> sign for the two-kind model: the cases really do split that way in the data.

## 4. 🐞 `defeatedLorelei` IS NOT "defeated Lorelei" — and the mistake has teeth

**`wElite4Flags` bit 1 is `BIT_STARTED_ELITE_4`.** It is not about beating anybody.

**Where it is set** — `scripts/LoreleisRoom.asm`, and note *when*:

```asm
LoreleiShowOrHideExitBlock:
; Blocks or clears the exit to the next room.
	ld hl, wCurrentMapScriptFlags
	bit BIT_CUR_MAP_LOADED_1, [hl]     ; <- on MAP LOAD ...
	res BIT_CUR_MAP_LOADED_1, [hl]
	ret z
	ld hl, wElite4Flags
	set BIT_STARTED_ELITE_4, [hl]      ; ... just for WALKING IN
```

**It is set the moment you enter Lorelei's room.** You have not fought her. You have not beaten her.
You opened the door.

**Where it is read** — `scripts/IndigoPlateauLobby.asm`, and this is the part that matters:

```asm
	; Reset Elite Four events if the player started challenging them before
	ld hl, wElite4Flags
	bit BIT_STARTED_ELITE_4, [hl]
	res BIT_STARTED_ELITE_4, [hl]
	ret z
	ResetEventRange INDIGO_PLATEAU_EVENTS_START, EVENT_LANCES_ROOM_LOCK_DOOR
```

So its actual job is: **"did you start a run and come back to the lobby? then wipe the whole Elite 4
run and make them fight it again."**

> 🚨 **The consequence of the wrong label, stated plainly:** a checkbox reading *"Defeated Lorelei"*
> that a user ticks does **not** record a victory. It arms an **Elite-Four-progress reset** that fires
> the next time they walk into the Indigo Plateau lobby. **The label promises progress and the bit
> delivers erasure.** This is not a cosmetic naming nit.

**Inherited from v1 verbatim** — `WorldCompleted.ts` line 17: `this.defeatedLorelei =
saveFile.getBit(0x29E0, 1, 1);`. Same offset, same wrong name, carried into v2 without a second look.
**Same shape as the `fly.json` find**: right bits, wrong words, and nothing ever asked the game.

**The real "defeated Lorelei" already exists** and we already ship it: **`EVENT_BEAT_LORELEIS_ROOM_TRAINER_0`**,
one of the 2,560 event flags — and `LoreleiShowOrHideExitBlock` is right there checking it to decide
whether to unblock the exit. So this is not a missing feature; it is **one flag wearing another's
name**, while the real one sits on the Events section of the same panel.

⚠️ **It has NO x/y.** It is written by the map-load script, not by talking to anyone or standing
anywhere. It belongs to **Lorelei's Room (245)** and **Indigo Plateau Lobby (174)** as a page entry
with **no canvas box** — per the standing rule, *a spot whose location cannot be established gets no
box, never a guessed one*.

### 💀 And the bit actually named "beat the Elite 4" is dead

**`wElite4Flags` bit 0 = `BIT_UNUSED_BEAT_ELITE_4`.** `HallOfFame.asm` sets it — and pret's own
comment on that very line says `; unused`. **Nothing reads it, ever.** It is not modelled today, and
that is correct; if it is ever shown it must wear the grey **💀** and say so, because its name is the
single most misleading string in this byte.

**So `wElite4Flags` is two bits and both are traps:** bit 0 is named for a thing it doesn't do and is
never read; bit 1 does something real that its v1 name misdescribes.

## 5. `everHealedPokemon` — leadership's guess was essentially right, with one wrinkle

> *"for ever healed pokemon im guessing the nurse script activates it?"*

**Yes — but it is not a per-map script.** It is `DisplayPokemonCenterDialogue_` in
`engine/events/pokecenter.asm`: the **one nurse routine every Poké Center shares**.

```asm
DisplayPokemonCenterDialogue_::
	call SaveScreenTilesToBuffer1
	ld hl, PokemonCenterWelcomeText
	call PrintText
	ld hl, wStatusFlags4
	bit BIT_USED_POKECENTER, [hl]
	set BIT_UNKNOWN_4_1, [hl]
	set BIT_USED_POKECENTER, [hl]
	jr nz, .skipShallWeHealYourPokemon      ; <- the ONLY thing it does
	ld hl, ShallWeHealYourPokemonText
	call PrintText
.skipShallWeHealYourPokemon
```

⚠️ **All it controls is one line of dialogue.** If the bit is clear, the nurse adds *"Shall we heal
your Pokémon?"*; if set, she skips it. **It does not gate healing**, it does not gate anything else,
and nothing else in the game reads it. Clearing it makes the next nurse you meet greet you like a
first-timer. That is the whole feature, and the checkbox should be honest that it is a cosmetic
first-visit marker rather than progress.

⚠️ **It is set unconditionally, before the Yes/No** — walking in and declining still sets it.
"Ever *healed*" is therefore a slight overclaim: the real meaning is **"ever talked to a Poké Center
nurse."** Our name is inherited from v1 and is subtly wrong; the fix is words, not bits.

📝 **`BIT_UNKNOWN_4_1` (bit 1) is set on the same two lines** and pret has no name for it. Nothing in
the disassembly reads it. Noted, not modelled — a genuine unknown, and the honest thing is to say so
rather than invent a meaning.

**Location:** its writer is an engine routine, so its "where" is **every Poké Center's nurse** — a
real `object_event` on each of them. That is many maps, one bit → the **shared** group again, and the
biggest one. ⏳ **Not yet decided:** whether it earns a box on all ~10+ Poké Centers or belongs on
**General**. Leadership's call; both are defensible and the note refuses to guess.

## 6. Are they scratch? No — and here is why that needed asking

All eight live in `SECTION "Main Data"`, inside the saved block, alongside the event flags. **None is
scratch WRAM.** But "in the saved block" has never been sufficient on this project —
`wTownVisitedFlag` is in the saved block and the current town's bit is still re-set on every Continue
([`town-visited.md`](town-visited.md)), and `wStatusFlags3` is in the saved block and gets **zeroed
whole** on load.

⚠️ **And two of these bytes are known to be partly rewritten**: `AreaPlayer` already marks
`strengthOutsideBattle` (`0x29D4` b0) as *reset on load* and `battleEndedOrBlackout`/`usingLinkCable`
(`0x29DA` b5/b6) as *cleared on entry* — **the same two bytes these eight flags live in**. So a
loader that clears a whole byte would take the rods and Lapras with it.

✅ **CONSOLE-VERIFIED — all eight are durable** (`scripts/emu/probe_world_completed.py`, 2026-07-17).
The prediction held: the rewrites are **per-bit, not whole-byte**.

| Scenario | Result |
|---|---|
| **B** all eight set → Continue | **8/8 survive** |
| **C** all eight cleared → Continue | **stay clear** — the loader sets none of them (the control) |
| **D** the three bytes handed `0xFF` | `wStatusFlags1` → **`0xFF` unchanged** · `wStatusFlags4` → **`0x9F`** · `wElite4Flags` → **`0xFF` unchanged** |

⭐ **D is the one that earned the probe**, and it reported two things nobody had to take on trust:

1. **Only bits 5 and 6 of `wStatusFlags4` were cleared** — `BIT_BATTLE_OVER_OR_BLACKOUT` and
   `BIT_LINK_CONNECTED`, i.e. **exactly the two bits `AreaPlayer` already documents as cleared on
   entry**, reproduced independently from the opposite direction. The clears are surgical. Nothing
   in these three bytes is zeroed whole, so the rods, the guards, Lapras, the starter, the nurse
   flag and the Elite 4 flag are never collateral.
2. **`strengthOutsideBattle` (`0x29D4` b0) SURVIVED** — and that is not a contradiction of
   [`player-state.md`](player-state.md), it is **that note's own finding, reproduced by accident**:
   strength is reset on an ordinary Continue *but survives when the battle-over bit is set*. D sets
   every bit, including battle-over — so `EnterMap` kept strength, then cleared battle-over on its
   way past. Two probes, written months apart for different features, agreeing on a subtle
   interaction. That is what a verified note is worth.

So: **durable, all eight, no `!` marks owed, no conditional exception** — unlike
[`town-visited.md`](town-visited.md), whose current-town bit is re-marked. The UI can treat these as
ordinary persistent switches.

## 7. Traps, collected

1. **`defeatedLorelei` is `BIT_STARTED_ELITE_4`** (§4) — the label promises progress; the bit arms a
   progress **wipe**. v1's name, carried in verbatim.
2. **The bit actually named "beat elite 4" is dead** (§4) — written by Hall of Fame, never read.
3. **The real "defeated Lorelei"** is `EVENT_BEAT_LORELEIS_ROOM_TRAINER_0`, an event flag we already
   ship (§4).
4. **A rod flag ≠ owning the rod** (§2) — it means "the Guru already gave you one". Ticking it does
   **not** put a rod in the bag; it takes one away from you forever.
5. **`everHealedPokemon` means "ever talked to a nurse"** (§5) — set before the Yes/No, and it only
   controls one line of dialogue.
6. **Saffron guards + starter are SHARED, not alike** (§3) — one bit, several maps.
7. **`BIT_UNKNOWN_4_1`** is set by the Poké Center and read by nothing (§5).
8. **These flags share bytes with known rewritten-on-load bits** (§6) — which is exactly why the
   probe is not optional.

---

**See also:** [`town-visited.md`](town-visited.md) (the other v1-carryover mislabel, and the group
kinds) · [`in-game-trades.md`](in-game-trades.md) · [`event-flags.md`](event-flags.md) (where the
real Lorelei victory lives) · [`player-state.md`](player-state.md) (`0x29D4`/`0x29DA`'s other
tenants, and their rewrite marks).
