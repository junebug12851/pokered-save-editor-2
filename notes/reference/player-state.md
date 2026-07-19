# The Player — position, facing, and the twenty-six bytes of map state

**Status:** researched 2026-07-14, **verified against the cartridge**
(`scripts/emu/probe_player_state.py`). Read this before fleshing out the Player details
panel. The design that consumes it is [`../plans/map-screen.md`](../plans/map-screen.md) →
**Phase 5e**.

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format),
> [`warps.md`](warps.md) (the warp-adjacent trio and the `wStatusFlags3` wipe live there too),
> [`sprites.md`](sprites.md) (the persistence doctrine), and
> [`emulator-verification.md`](emulator-verification.md) (how the console gets asked).

---

## 0. The one-paragraph version

`AreaPlayer` is the richest Area child: **where the player stands, which way he faces, how
he's getting around, what he may do here, whether a battle is on, and a fistful of mid-step
scratch bits.** v1's "Player" page (`area-player`) grouped them Direction / Coordinates /
HMs / Battle / Warps / Other and shipped every one with a plausible label and **no word about
which the game keeps.** That last part is the whole story: of the 26 values, **ten are
rewritten the instant the save loads**, **three are dead** (written by the game, never read),
several are **misnamed**, and only about half are durable state a person can actually edit and
keep. The console was asked, byte by byte, and it settled every case.

**The address maths for everything below** (same as warps): the save's Main Data block starts
at file offset `0x25A3` and maps to `wMainDataStart` = `$D2F7`, so

> **`WRAM address = save offset + 0xAD54`**

---

## 1. What happens to the player on a Continue — the load path

Three routines run between "press A on CONTINUE" and "you're standing on the map", and each
one stamps on some of these bytes. This is *why* the ten rewrites happen.

**1. The Continue handler forces the facing.** `MainMenu` (`engine/menus/main_menu.asm`),
the moment you pick Continue and press A:

```asm
.pressedA
	...
	ld a, PLAYER_DIR_DOWN
	ld [wPlayerDirection], a     ; <- your saved facing is GONE. You always wake up facing down.
	...
	jp SpecialEnterMap
```

**2. `SpecialEnterMap` zeroes `wStatusFlags3`.** It writes `0` to `wCableClubDestinationMap`,
which is **the same byte** as `wStatusFlags3` (`ram/wram.asm` puts the labels back-to-back):

```asm
SpecialEnterMap::
	xor a
	...
	ld [wCableClubDestinationMap], a   ; <- wStatusFlags3 = 0, in its entirety
	...
	call ResetPlayerSpriteData
	jp EnterMap
```

So **`isBattle` and `isTrainerBattle` (both misnamed — see §3) can never survive a save.** This
is the same wipe that kills the warp panel's `scriptedWarp`/`isDungeonWarp` and the NPC panel's
`npcsFaceAway`/`tradeCenterSpritesFaced` — the whole byte dies. See [`warps.md`](warps.md) §3.

**3. `EnterMap` resets the strength bit and clears battle-over.** `home/overworld.asm`:

```asm
	ld hl, wStatusFlags4
	bit BIT_BATTLE_OVER_OR_BLACKOUT, [hl]
	res BIT_BATTLE_OVER_OR_BLACKOUT, [hl]   ; <- battleEndedOrBlackout ALWAYS cleared
	call z, ResetUsingStrengthOutOfBattleBit ; <- if battle-over was 0: strength cleared too
	call nz, MapEntryAfterBattle             ; <- if battle-over was 1: strength SURVIVES
```

`ResetUsingStrengthOutOfBattleBit` is two instructions — `res BIT_STRENGTH_ACTIVE, [wStatusFlags1]`.

> ⚠️ **A genuine interaction, and the probe caught it.** On an ordinary Continue the battle-over
> bit is clear, so **`strengthOutsideBattle` is reset to 0**. But if the save happens to hold the
> battle-over bit *set*, `EnterMap` takes the after-battle branch and **strength survives** — while
> battle-over itself is still cleared. Two saves, two answers, both console-verified (§5).

The sprite mirror (`ResetPlayerSpriteData`) also clears the OAM copy at `wSpriteStateData1/2`, but
that lives outside the Main Data block and isn't in the save — it doesn't touch any field here.

---

## 2. The complete table — every player byte, one row each

Legend: **`!`** = **rewritten/zeroed on load** (gets the yellow-exclamation treatment).
**`❌`** = written by the game, **never read** by it. **`~`** = kept at load but **recomputed as
soon as the player moves** (transient step state; editing it is momentary). No mark = durable
state a person can edit and keep.

| | Save | Bit | Real WRAM name | v1/v2 field | Plain-English | On load (console) |
|---|---|---|---|---|---|---|
| | `0x260D` | — | `wYCoord` | `yCoord` | **Tile Y** | kept |
| | `0x260E` | — | `wXCoord` | `xCoord` | **Tile X** | kept |
| | `0x260F` | — | `wYBlockCoord` | `yBlockCoord` | **Which half-block, Y** (0/1) | kept |
| | `0x2610` | — | `wXBlockCoord` | `xBlockCoord` | **Which half-block, X** (0/1) | kept |
| `~` | `0x27D4` | — | `wPlayerMovingDirection` | `playerMoveDir` | **Moving** (0 = still; scripts write it to turn him) | kept (0 in any real save) |
| | `0x27D5` | — | `wPlayerLastStopDirection` | `playerLastStopDir` | **Last stop** (facing before he last stopped) | kept |
| **!** | `0x27D6` | — | `wPlayerDirection` | `playerCurDir` | **Current direction** | **forced to DOWN** |
| | `0x29AC` | — | `wWalkBikeSurfState` | `walkBikeSurf` | **Walk / Bike / Surf** (0/1/2) | kept (some maps force it) |
| **!** | `0x29C0` | — | `wPlayerJumpingYScreenCoordsIndex` | `playerJumpingYScrnCoords` | **Jumping Y** (ledge-hop animation index) | **zeroed** |
| ❌ | `0x278E` | — | `wYOffsetSinceLastSpecialWarp` | `yOffsetSinceLastSpecialWarp` | **Y offset since special warp** | kept — but nothing reads it |
| ❌ | `0x278F` | — | `wXOffsetSinceLastSpecialWarp` | `xOffsetSinceLastSpecialWarp` | **X offset since special warp** | kept — but nothing reads it |
| **!** | `0x29D4` | 0 | `BIT_STRENGTH_ACTIVE` (`wStatusFlags1`) | `strengthOutsideBattle` | **Using Strength** | **reset** (unless battle-over set) |
| `~` | `0x29D4` | 1 | `BIT_SURF_ALLOWED` | `surfingAllowed` | **Surfing allowed here** | kept (recomputed near water) |
| ❌ | `0x29D4` | 7 | `BIT_UNUSED_CARD_KEY` | `usedCardKey` | **Used Card Key** | kept — **`; never checked`** in source |
| **!** | `0x29D9` | 6 | `BIT_TALKED_TO_TRAINER` (`wStatusFlags3`) | `isBattle` | **Battle ongoing** *(misnamed)* | **zeroed** (whole byte) |
| **!** | `0x29D9` | 7 | `BIT_PRINT_END_BATTLE_TEXT` | `isTrainerBattle` | **Trainer battle** *(misnamed)* | **zeroed** (whole byte) |
| | `0x29DA` | 4 | `BIT_NO_BATTLES` (`wStatusFlags4`) | `noBattles` | **Prevent battles** | kept |
| **!** | `0x29DA` | 5 | `BIT_BATTLE_OVER_OR_BLACKOUT` | `battleEndedOrBlackout` | **Battle ended / blackout** | **cleared** (always) |
| **!** | `0x29DA` | 6 | `BIT_LINK_CONNECTED` | `usingLinkCable` | **Using link cable** | **cleared** |
| `~` | `0x29DF` | 7 | `BIT_USED_FLY` (`wStatusFlags7`) | `flyOutofBattle` | **Arrived by Fly** *(misnamed "Using Fly")* | kept (transient) |
| **!** | `0x29E2` | 0 | `BIT_STANDING_ON_DOOR` (`wMovementFlags`) | `standingOnDoor` | **Standing on a door** | **cleared** |
| **!** | `0x29E2` | 1 | `BIT_EXITING_DOOR` | `movingThroughDoor` | **Walking through a door** | **cleared** |
| `~` | `0x29E2` | 2 | `BIT_STANDING_ON_WARP` | `standingOnWarp` | **Standing on a warp** | kept (transient) |
| **!** | `0x29E2` | 6 | `BIT_LEDGE_OR_FISHING` | `finalLedgeJumping` | **Ledge-hop / fishing** *(mis-narrowed)* | **cleared** |
| `~` | `0x29E2` | 7 | `BIT_SPINNING` | `spinPlayer` | **On a spin tile** (Rocket HQ / gyms) | kept (transient) |

**Notes on the trickier ones:**

- **`wPlayerDirection` uses RAW bit flags** — `RIGHT = 1, LEFT = 2, DOWN = 4, UP = 8`, the same
  encoding as sprite facing. ⚠️ v2's `PlayerDir` enum is `None/Right/Left/Down/Up = 0..4`, a
  **different encoding** (the header already flags this). The three direction bytes store the raw
  flags; the enum is a QML convenience. Don't confuse them.
- **`BIT_LEDGE_OR_FISHING`** is not "final ledge jump" — it's set for **the whole ledge hop *and*
  while fishing**. At load, `OverworldLoop` sees it set and calls `HandleMidJump`, which clears it —
  which is exactly why the probe reads it back as 0.
- **`BIT_USED_FLY`** means *"you arrived by Fly, play the landing animation"*, not "Fly is usable
  here". v1's HM grouping ("Using Fly") is misleading.

---

## 3. The bugs this found in the v2 model (fix before building UI on them)

Same shape as the sprite and warp passes: `AreaPlayer` is a straight port of v1, so it carries
v1's guesses. The **byte offsets and bit numbers are all correct** — the *names* are the problem.

| | v2 has | The truth |
|---|---|---|
| 1 | `isBattle` (`0x29D9` b6) — *"In a battle"* | **`BIT_TALKED_TO_TRAINER`**. Not a battle flag, and **zeroed on load** with the rest of `wStatusFlags3`. |
| 2 | `isTrainerBattle` (`0x29D9` b7) — *"In a trainer battle"* | **`BIT_PRINT_END_BATTLE_TEXT`**. Also **zeroed on load**. |
| 3 | `flyOutofBattle` (`0x29DF` b7) — *"Fly usable here"* | **`BIT_USED_FLY`** — "arrived by Fly, play the drop-in". |
| 4 | `finalLedgeJumping` (`0x29E2` b6) — *"Mid final ledge jump"* | **`BIT_LEDGE_OR_FISHING`** — ledge hop **or fishing**; cleared at load by `HandleMidJump`. |
| 5 | `usedCardKey` (`0x29D4` b7) — *"Card key just used"* | **`BIT_UNUSED_CARD_KEY`**. The setter is annotated **`; never checked`** — a **dead bit**. |
| 6 | `strengthOutsideBattle` presented as durable | **Reset on every ordinary Continue** (`ResetUsingStrengthOutOfBattleBit`); survives *only* if the battle-over bit is set. |
| 7 | `yOffsetSinceLastSpecialWarp` / `xOffsetSinceLastSpecialWarp` as meaningful | **`; they don't seem to be used for anything`** (`wram.asm`). **Dead**, like `wWarpedFromWhichWarp/Map`. |
| 8 | `noBattles` / `battleEndedOrBlackout` fine as-is | Names OK. But `battleEndedOrBlackout` is **cleared on every map entry**, and `noBattles` is durable — the panel must say which. |

None of this is a *save-corruption* bug (unlike the tileset `collPtr` or the sprite-mobility
inversion) — `AreaPlayer::save()` writes the correct bits to the correct addresses. It's a
**truth-in-labelling** problem: the fields lie about what they are and never say what the game
keeps. The fix is a rename + doc pass in a phase of its own, before the panel is built on them —
exactly the warp/sprite precedent. `setTo()`/`randomize()` only touch coords + directions, so
unlike `AreaWarps::setTo()` there's **no hazard here** to guard against.

---

## 4. How the panel should carve this up

Four honest buckets, plus the collapse-behind-a-switch group for the rewrites — the same
doctrine as the warp panel (`[ ⚠ Show N fields the game rewrites on load ]`).

- **Position** — Tile X/Y, half-block X/Y, **Current direction** (⚠ forced DOWN), Last stop,
  Moving, and the **Walk / Bike / Surf** mode. This is the durable, meaningful core.
- **Here he may…** — Surfing allowed (`~`), Arrived by Fly (`~`), **Using Strength** (⚠ reset).
- **Battle** — Prevent battles (durable), **Battle ongoing** (⚠), **Trainer battle** (⚠),
  **Battle ended** (⚠).
- **Warp-adjacent** — Standing on warp (`~`), **Standing on door** (⚠), **Walking through door**
  (⚠); these three are the trio [`warps.md`](warps.md) already points at from the door panel.
- **The `!` group, collapsed** — everything marked `!` or `❌` above, each with the yellow `!`
  (rewritten) or the `❌` line (dead: "the game writes this and never reads it"). Ten `!` fields,
  three `❌`. A wiped byte and an unread byte are **different facts** and the panel says which.

Every value stays **full-range and editable, hack values included** — the project rule. The panel
never refuses a value and never silently rewrites one; it only *tells the truth* about what the
console will do with it on the next load.

---

## 5. The console's testimony

`scripts/emu/probe_player_state.py` stamps a marker into all 26 values, seals the checksum, boots
the **real ROM** twice — once with the battle-over bit clear (the ordinary Continue), once set
(the after-battle path) — and reads back every byte and bit. Verbatim:

```
===== N  ordinary Continue (battle-over CLEAR) =====
  wPlayerDirection (curDir)       wrote 0x08  read 0x04   REWRITE   <- forced to DOWN
  wPlayerJumpingYScreenCoordsIdx  wrote 0x33  read 0x00   REWRITE   <- zeroed
  SF1.b0 strengthOutsideBattle    wrote 1  read 0   REWRITE        <- reset
  SF1.b1 surfingAllowed           wrote 1  read 1   kept
  SF1.b7 usedCardKey (dead)       wrote 1  read 1   kept
  SF3.b6 isBattle                 wrote 1  read 0   REWRITE        <- wStatusFlags3 wiped
  SF3.b7 isTrainerBattle          wrote 1  read 0   REWRITE
  SF4.b4 noBattles                wrote 1  read 1   kept
  SF4.b5 battleEndedOrBlackout    wrote 0  read 0   kept
  SF4.b6 usingLinkCable           wrote 1  read 0   REWRITE
  SF7.b7 flyOutofBattle           wrote 1  read 1   kept
  MOVE.b0 standingOnDoor          wrote 1  read 0   REWRITE
  MOVE.b1 movingThroughDoor       wrote 1  read 0   REWRITE
  MOVE.b2 standingOnWarp          wrote 1  read 1   kept
  MOVE.b6 finalLedgeJumping       wrote 1  read 0   REWRITE
  MOVE.b7 spinPlayer              wrote 1  read 1   kept
  (coords, blockcoords, moveDir=0, lastStop, walkBikeSurf, the two offsets: all KEPT)

===== B  after-battle Continue (battle-over SET) =====
  SF1.b0 strengthOutsideBattle    wrote 1  read 1   kept        <- SURVIVES on this path
  SF4.b5 battleEndedOrBlackout    wrote 1  read 0   REWRITE     <- still cleared
  (everything else identical to N)
```

> ⚠️ **The probe earned its keep — a careful asm read would have got `wMovementFlags` wrong.**
> Bits 0/1/6 of that one byte are **cleared** on load while bits 2/7 are **kept**. It is not a
> whole-byte wipe; individual routines (door handling, `HandleMidJump`) clear individual bits, and
> the spin/warp bits are only ever recomputed *while walking*. Reading the source alone, the
> natural guess is "the byte behaves as a unit" — and that guess is wrong. This is exactly the
> mistake the sprite persistence pass made (see [`sprites.md`](sprites.md) → Part 5); the cartridge
> is the oracle.

---

## 6. Sources

Everything above is read out of `pret/pokered` and then **checked against the cartridge**:

| What | Where |
|---|---|
| The player-state bytes + comments (`; they don't seem to be used`, walk/bike/surf) | `ram/wram.asm` |
| Every flag-bit constant (`wStatusFlags1/3/4/7`, `wMovementFlags`) | `constants/ram_constants.asm` |
| The forced facing on Continue | `engine/menus/main_menu.asm` → `MainMenu` `.pressedA`, `SpecialEnterMap` |
| Strength reset + battle-over clear on entry | `home/overworld.asm` → `EnterMap`, `ResetUsingStrengthOutOfBattleBit` |
| `wStatusFlags3` wipe (the alias) | `ram/wram.asm` (`wCableClubDestinationMap::` above `wStatusFlags3::`) |
| Card-key bit `; never checked` | `engine/items/item_effects.asm` → `ItemUseCardKey` |
| `BIT_LEDGE_OR_FISHING` handled + cleared at load | `home/overworld.asm` → `OverworldLoop` → `HandleMidJump` |
| The whole thing, byte by byte | `scripts/emu/probe_player_state.py` (local-only, ROM-gated) |
