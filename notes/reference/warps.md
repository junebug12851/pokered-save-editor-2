# Warps — the doors, and the twelve bytes around them

**Status:** researched 2026-07-14, **verified against the cartridge** (`scripts/emu/probe_warp_persistence.py`).
Read this before any warp work. The design that consumes it is
[`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 5a**.

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format),
> [`sprites.md`](sprites.md) (the same persistence story, by a different mechanism),
> [`emulator-verification.md`](emulator-verification.md) (how the console gets asked).

---

## 0. The one-paragraph version

A warp is a **door**. The map's warp list says *which tile* is a door and *where it leads* — and
that list is **32 entries of 4 bytes**, sitting right there in the save. Around it are **twelve
state bytes** that describe a warp *in flight*: whether you're falling down a hole, whether you
Flew, where Dig will drop you, which door you came in by. v1 shipped all twelve with **invented
names** and no explanation, and three of them are **wrong**, two are **dead**, two more are
**hazards**, and two of the most useful ones **aren't on the warps screen at all**.

**The address maths for everything below:** the save's Main Data block starts at file offset
`0x25A3` and maps to `wMainDataStart` = `$D2F7`, so

> **`WRAM address = save offset + 0xAD54`**

---

## 1. THE LINCHPIN — why an edited warp actually works

This is the whole reason a warp editor is possible, and it is not obvious.

`LoadMapHeader` (`home/overworld.asm`) rebuilds the map's warp list **from ROM** on every map
load — and unlike the sprite block, it has **no escape-hatch bit**:

```asm
; load warp data
	ld a, [hli]
	ld [wNumberOfWarps], a      ; <- ROM
	and a
	jr z, .loadSignData
	ld c, a
	ld de, wWarpEntries         ; <- ROM, 4 bytes x N
.warpLoop
	...
```

So on the face of it, every warp byte an editor writes is erased the instant the map loads. But
`LoadMapHeader` never gets that far on a Continue, because it opens with:

```asm
	ld a, [wCurMapTileset]
	ld b, a
	res BIT_NO_PREVIOUS_MAP, a
	ld [wCurMapTileset], a
	bit BIT_NO_PREVIOUS_MAP, b
	ret nz                      ; <- RETURNS. No header. No warps. No signs. No sprites.
```

…and `LoadMainData` (`engine/menus/save.asm`), the routine that reads the save off the cartridge,
does this the moment it finishes copying:

```asm
	ld hl, wCurMapTileset
	set BIT_NO_PREVIOUS_MAP, [hl]   ; <- bit 7
```

**The game deliberately flags "there is no previous map" as it loads your save**, which makes the
very next `LoadMapHeader` bail out before it can overwrite anything. Your warps are the ones the
console runs on.

> ### The rule, stated the way the screen must state it
>
> **An edited warp is really there.** Load the save and the door is where you put it, going where
> you aimed it.
>
> **And the game restores the map's original doors the moment the player leaves the map and walks
> back in.** By then `BIT_NO_PREVIOUS_MAP` has been cleared (`LoadMapHeader` clears it on the way
> past), so the next load reads the header from ROM and the ROM's warps win.
>
> This is *exactly* the sprite story ([`sprites.md`](sprites.md) → Part 5) reached by a different
> route. Same doctrine: **shown plainly, never hidden, never silently "fixed."**

**The cartridge confirms it.** `probe_warp_persistence.py` moves Pallet Town's warp 0 to (2,2),
re-aims it at Viridian Forest, and **invents a 4th warp in a 3-warp town** — then boots the real
ROM:

```
  B  warp LIST tampered
    wNumberOfWarps  4
      warp 0   tile=( 2, 2)   -> map 0x2A, warp 3     <- ours
      warp 1   tile=(13, 5)   -> map 0x27, warp 0
      warp 2   tile=(12,11)   -> map 0x28, warp 1
      warp 3   tile=( 9, 9)   -> map 0x25, warp 0     <- invented out of nothing
  VERDICT
    Does the console run on OUR warp list?   YES -- the save wins on Continue
```

---

## 2. The warp list — `wWarpEntries`

| | |
|---|---|
| **Count** | `wNumberOfWarps` — save **`0x265A`**, WRAM `$D3AE`. Max **32** (`MAX_WARP_EVENTS`). |
| **The list** | `wWarpEntries` — save **`0x265B`**, WRAM `$D3AF`. **32 × 4 bytes.** |

Each entry is **`Y, X, destWarp, destMap`** — and note the order: **Y comes first.** (`WarpData`
already reads it in that order; it is right.)

- **`Y`, `X`** — the **tile** on this map that is a door.
- **`destMap`** — the map id it leads to. **`$FF` (`LAST_MAP`) means "back outside"** — go to
  whatever map is in **`wLastMap`** (§4). This is how every building's exit works: one warp, and
  it returns you to whichever town you entered from. **A map that `CheckIfInOutsideMap` calls
  "outside" cannot use `$FF`** — the game takes the other branch and would treat `$FF` as a real
  map id.
- **`destWarp`** — **an index into the destination map's `warps_to` list**, *not* into its warp
  entries. This is the thing everyone gets wrong. The ROM gives every map a second, separate table
  — the **arrival points** — and `LoadDestinationWarpPosition` copies **4 bytes** out of it at
  `destWarp × 4`:

  ```asm
  	ld bc, 4
  	ld de, wCurrentTileBlockMapViewPointer   ; viewPtr(2), then wYCoord, then wXCoord
  	call CopyData
  ```

  So arriving through a warp writes the **view pointer and the player's Y/X in one go**. We already
  model both tables: **`MapDBEntryWarpOut`** (the doors) and **`MapDBEntryWarpIn`** (the arrival
  points). `destWarp` indexes the **WarpIn** list of the target map.

> ⚠️ **`destWarp` is not bounds-checked.** Point it past the end of the target map's `warps_to`
> list and the console copies four arbitrary ROM bytes into the view pointer + the player's coords.
> Full range is still offered — but it is **flagged**, and the panel says how many arrival points
> the target map actually has.

---

## 3. The twelve state bytes — real names, and what they really do

v1's page called these things "Scripted Warp", "Non-Normal Warp", "Special Warp", "Skip Joypad".
Here is what each one **is**.

### Group A — *"a warp is happening"* (`wStatusFlags6`, save `0x29DE`, WRAM `$D732`)

| bit | v1/v2 name | **Real name** | What it actually does |
|---|---|---|---|
| 2 | `flyOrDungeonWarp` — *"Non-Normal Warp"* | **`BIT_FLY_OR_DUNGEON_WARP`** | **"A special warp is in progress."** The gate on the whole special-warp path: `HandleFlyWarpOrDungeonWarp` sets it, `PrepareForSpecialWarp` tests it and immediately clears it. Set → the destination comes from `wDestinationMap`; clear → the game thinks it's starting a new game and warps you to **Red's bedroom**. |
| 3 | `flyWarp` — *"Special Warp"* | **`BIT_FLY_WARP`** | **"Arrive with the fly/warp-pad animation."** Set when you step on a **warp pad**, and by Fly. `EnterMap` sees it, plays `EnterMapAnim` (the drop-in), and clears it. |
| 4 | `dungeonWarp` — *"Dungeon Warp"* | **`BIT_DUNGEON_WARP`** | **"You fell down a hole."** Sends the destination lookup to `DungeonWarpList` instead of the fly table — the Seafoam / Victory Road / Pokémon Mansion holes. |
| 6 | ❌ **not on the warps screen** | **`BIT_ESCAPE_WARP`** | **Dig, Escape Rope, and blacking out.** Set by `black_out.asm`, `item_effects.asm` (Escape Rope) and `start_sub_menus.asm` (Dig). Consumed by `PrepareForSpecialWarp`, which sends you to **`wLastBlackoutMap`**. ⚠️ **v2 has this as `AreaMap::blackoutDest` — a bool called "blackout *destination*" that is not a destination at all.** It belongs here, under its real name. |

*(bit 0 = `BIT_GAME_TIMER_COUNTING`, bit 1 = `BIT_DEBUG_MODE`, bit 5 = `BIT_ALWAYS_ON_BIKE` — all
already modelled elsewhere and not warp business.)*

### Group B — *"where does it go"*

| Save | WRAM | v1/v2 name | **Real name** | What it is |
|---|---|---|---|---|
| **`0x26DB`** | `$D42F` | `warpDest` — *"To Warp"* | **`wDestinationWarpID`** | **Which arrival point you land on** in the map you're entering. **`$FF` = "don't move the player"** — every special warp sets `$FF`, because it has already written the coordinates itself. |
| **`0x29C6`** | `$D71A` | `specialWarpDestMap` — *"Special Warp Where"* | **`wDestinationMap`** | **Where Fly / the Hall of Fame / the Cable Club sends you.** ⚠️ **A HAZARD — see §5.** |
| **`0x29C9`** | `$D71D` | `dungeonWarpDestMap` — *"Dungeon Warp Where"* | **`wDungeonWarpDestinationMap`** | **Which floor a hole drops you onto.** Written by exactly six map scripts. ⚠️ **A HAZARD — see §5.** |
| **`0x29CA`** | `$D71E` | `whichDungeonWarp` — *"From Dungeon Warp"* | **`wWhichDungeonWarp`** | **Which hole on the floor above you fell through** — **1-based**, and it must *pair* with the map above. `IsPlayerOnDungeonWarp` sets it from `wCoordIndex`. ⚠️ **Half of the same hazard.** |

### Group C — *"a script wants a warp"* (`wStatusFlags3`, save `0x29D9`, WRAM `$D72D`)

| bit | v1/v2 name | **Real name** | What it actually does |
|---|---|---|---|
| 3 | `scriptedWarp` — *"Scripted Warp"* | **`BIT_WARP_FROM_CUR_SCRIPT`** | **"Warp me right now, no tile needed."** `OverworldLoop` tests it *every frame* and jumps straight to `WarpFound2`. Set by exactly **one** script in the game: **Pokémon Tower 7F** (Mr. Fuji carrying you to his house). |
| 4 | `isDungeonWarp` — *"Is Dungeon Warp"* | **`BIT_ON_DUNGEON_WARP`** | **"You are standing on a hole."** Suppresses wild battles (`NewBattle` returns early) so you can't get jumped mid-fall. Cleared on arrival. |

> ### BOTH OF THESE ARE WIPED THE MOMENT THE SAVE LOADS — and so is the entire byte
>
> `wStatusFlags3` has an **alias**: `wCableClubDestinationMap` is *the same byte*. And
> `SpecialEnterMap` — which is on the Continue path — does:
>
> ```asm
> SpecialEnterMap::
> 	xor a
> 	...
> 	ld [wCableClubDestinationMap], a   ; <- writes 0 to wStatusFlags3. ALL of it.
> ```
>
> **Verified on the cartridge.** Set `wStatusFlags3 = $FF` in a save, boot the real ROM, read it
> back: **`0x00`.**
>
> ```
>   D  wStatusFlags3 = 0xFF  (is the WHOLE byte wiped, or just one bit?)
>     wrote 0xFF -> console has 0x00   WHOLE BYTE ZEROED
> ```
>
> So editing `scriptedWarp` or `isDungeonWarp` in a save **can never do anything**. They get a
> **yellow !**. And this is bigger than warps — **every** bit of `wStatusFlags3` dies on load:
>
> | bit | Where v2 keeps it | Real name |
> |---|---|---|
> | 0 | `AreaNPC::tradeCenterSpritesFaced` | `BIT_INIT_TRADE_CENTER_FACING` |
> | 3 | `AreaWarps::scriptedWarp` | `BIT_WARP_FROM_CUR_SCRIPT` |
> | 4 | `AreaWarps::isDungeonWarp` | `BIT_ON_DUNGEON_WARP` |
> | 5 | `AreaNPC::npcsFaceAway` | `BIT_NO_NPC_FACE_PLAYER` |
> | 6 | `AreaPlayer::isBattle` | **`BIT_TALKED_TO_TRAINER`** (misnamed in v2) |
> | 7 | `AreaPlayer::isTrainerBattle` | **`BIT_PRINT_END_BATTLE_TEXT`** (misnamed in v2) |
>
> All six are **regenerated (zeroed) on load**. All six get the **!**.
>
> *(`BIT_WARP_FROM_CUR_SCRIPT` is doubly dead: `OverworldLoop` clears it on every single
> iteration anyway.)*

### Group D — *"the joypad rule"* (`wStatusFlags7`, save `0x29DF`, WRAM `$D733`)

| bit | v1/v2 name | **Real name** | What it actually does |
|---|---|---|---|
| 2 | `skipJoypadCheckWarps` — *"Skip Joypad"* | **`BIT_FORCED_WARP`** | Normally, stepping onto a warp tile **without holding a direction does nothing** — you have to *walk into* the door. This bit removes that check: **the warp fires the instant you touch the tile.** Set/cleared by the **Seafoam Islands** boulder puzzle (B3F sets it, B4F clears it) — it is how the current sweeps you through. |

*(bit 7 = `BIT_USED_FLY`, which v2 keeps as `AreaPlayer::flyOutofBattle` — also misnamed; it means
"you arrived by Fly, play the landing animation".)*

### Group E — *"where you came from"* — ❌ **DEAD BYTES**

| Save | WRAM | v1/v2 name | **Real name** |
|---|---|---|---|
| **`0x29E7`** | `$D73B` | `warpedFromWarp` — *"From Warp"* | **`wWarpedFromWhichWarp`** |
| **`0x29E8`** | `$D73C` | `warpedfromMap` — *"From Map"* | **`wWarpedFromWhichMap`** |

`WarpFound2` writes them both on every warp:

```asm
WarpFound2::
	ld a, [wNumberOfWarps]
	sub c
	ld [wWarpedFromWhichWarp], a   ; index of the warp used (0-based)
	ld a, [wCurMap]
	ld [wWarpedFromWhichMap], a
```

**And nothing in the entire game ever reads either one.** Grep the whole disassembly: two writes,
zero reads. They are a **breadcrumb the game drops and never picks up** — a debugging leftover.

They survive the save/load perfectly (the console confirms it), they are just **inert**. Show them,
let them be edited, and **say plainly that changing them does nothing**.

---

## 4. The two bytes that ARE the workflow — and were on the wrong screen

Project leadership's instinct — *"from map needs to be in the top toolbar, it's important to workflow"* — is
right, but **`wWarpedFromWhichMap` is not the field they mean.** These two are:

| Save | WRAM | Name | What it is |
|---|---|---|---|
| **`0x2611`** | `$D365` | **`wLastMap`** | **The map that `destMap = $FF` returns you to.** Every building's exit warp is `$FF`. Walk out of Red's house and you're in Pallet Town *because this byte says Pallet Town*. It is the single most consequential warp byte in the save — and it is not the warp you came *through*, it is the **outdoor map you last stood on**. |
| **`0x29C5`** | `$D719` | **`wLastBlackoutMap`** | **Where you wake up.** Blacking out, using **Dig**, and using an **Escape Rope** all send you here. Written by `set_blackout_map.asm` from `wLastMap` (via the map's own "blackout destination" — usually its nearest Poké Center's town). |

**Good news: we already model both.** They live in **`WorldGeneral`** (`worldgeneral.cpp`, as
`lastMap` and `lastBlackoutMap`) — not in `AreaWarps`. **No new C++ modelling is needed.** They
simply have to be *surfaced on the map screen*, where the person editing warps can see them.

Both survive the load untouched (console-verified).

---

## 5. Two hazards — the destination tables have no bounds check

Same class as the **music bank** ([`glitch-music.md`](glitch-music.md)): a value the save holds
that the console will happily use to read arbitrary ROM.

### Hazard 1 — `wDestinationMap` (`specialWarpDestMap`): **only 13 legal values**

`PrepareForSpecialWarp` looks the map up in `FlyWarpDataPtr`:

```asm
.flyWarpDataPtrLoop
	ld a, [hli]
	inc hl
	cp b                        ; b = wDestinationMap
	jr z, .foundFlyWarpMatch
	inc hl
	inc hl
	jr .flyWarpDataPtrLoop      ; <- NO terminator. NO bounds check.
```

`FlyWarpDataPtr` has **13 entries and no end marker**. Give it a map that isn't in the table and the
loop **runs off the end of the table**, matches a byte of unrelated ROM, reads the two bytes after it
as a **pointer**, and copies six bytes from wherever that lands into `wCurrentTileBlockMapViewPointer`
+ the player's coordinates.

**The only legal values:**

> Pallet Town · Viridian City · Pewter City · Cerulean City · Lavender Town · Vermilion City ·
> Celadon City · Fuchsia City · Cinnabar Island · Indigo Plateau · Saffron City · **Route 4** ·
> **Route 10**

(The last two are the Mt. Moon and Rock Tunnel Poké Center routes — the game Flies you to a *route*,
not a town.)

### Hazard 2 — `wDungeonWarpDestinationMap` + `wWhichDungeonWarp`: **12 legal PAIRS**

Same shape, in `DungeonWarpList` — which *does* have a `db -1` terminator that the loop **doesn't
check for**. The **pair** must be one of these twelve:

| Map | Which hole |
|---|---|
| Seafoam Islands B1F | 1, 2 |
| Seafoam Islands B2F | 1, 2 |
| Seafoam Islands B3F | 1, 2 |
| Seafoam Islands B4F | 1, 2 |
| Victory Road 2F | 2 |
| Pokémon Mansion 1F | 1, 2 |
| Pokémon Mansion 2F | 3 |

Note the hole number is **1-based**, and `Victory Road 2F` has **no hole 1**. A map that's in the
list with a hole number that isn't paired with it is just as broken as a map that isn't in the list.

> **AND WE ARE CURRENTLY WRITING GARBAGE INTO BOTH.**
>
> `AreaWarps::randomize()` **and `AreaWarps::setTo()`** — the second of which runs on any "place the
> player on this map" — do this:
>
> ```cpp
> auto dungeonWarp = MapsDB::inst()->search()->isGood()->isType("Cave")->pickRandom();
> dungeonWarpDestMap = dungeonWarp->getInd();                       // ANY cave. 7 are legal.
> specialWarpDestMap = MapsDB::inst()->search()->isGood()->pickRandom()->getInd();  // ANY map. 13 are legal.
> whichDungeonWarp = Random::inst()->rangeExclusive(0, ...);        // 0-based. The game is 1-based.
> warpedFromWarp   = Random::inst()->rangeExclusive(0, ...);        // a byte nothing reads
> ```
>
> Three of those four are **wrong**, and two of them can hand a real console an out-of-table map id.
> It is **dormant today** only because `MapsDB` is never deep-linked at boot (see `status.md` → Open
> issues) — **the moment that landmine is defused, this goes live.** Fix it in the same pass, exactly
> as `AreaAudio::setTo()` was fixed before *its* path went live.
>
> The right behaviour: `setTo()` should **not invent warp state at all** — a map has no opinion about
> where your last Fly went. It should set the **warp list** from the map's `warpOut` and **leave the
> state bytes alone**.

---

## 6. The complete table — every warp byte, one row each

`!` = **regenerated / rewritten on load** (the yellow-exclamation group).
`❌` = written by the game, **never read** by it.
`⚠️` = a value that can destabilize the console.

| ! | Save | WRAM | Real name | Plain-English name | Range |
|---|---|---|---|---|---|
| | `0x265A` | `$D3AE` | `wNumberOfWarps` | **Doors on this map** | 0–32 |
| | `0x265B` | `$D3AF` | `wWarpEntries` | **The doors** (Y, X, arrival point, destination map) × 32 | — |
| | `0x26DB` | `$D42F` | `wDestinationWarpID` | **Arriving at door #** (`$FF` = don't move me) | 0–255 |
| | `0x2611` | `$D365` | `wLastMap` | **Outside is…** (where `$FF` doors lead) | map id |
| | `0x29C5` | `$D719` | `wLastBlackoutMap` | **Wake up at…** (blackout / Dig / Escape Rope) | map id |
| ⚠️ | `0x29C6` | `$D71A` | `wDestinationMap` | **Fly / special warp sends you to…** | 13 legal |
| ⚠️ | `0x29C9` | `$D71D` | `wDungeonWarpDestinationMap` | **Falling drops you onto…** | 7 legal |
| ⚠️ | `0x29CA` | `$D71E` | `wWhichDungeonWarp` | **…through hole #** (1-based) | 1–3 |
| **!** | `0x29D9` b3 | `$D72D` | `BIT_WARP_FROM_CUR_SCRIPT` | **Script is warping you now** | flag |
| **!** | `0x29D9` b4 | `$D72D` | `BIT_ON_DUNGEON_WARP` | **Standing on a hole** (no wild battles) | flag |
| | `0x29DE` b2 | `$D732` | `BIT_FLY_OR_DUNGEON_WARP` | **A special warp is in progress** | flag |
| | `0x29DE` b3 | `$D732` | `BIT_FLY_WARP` | **Arrive with the drop-in animation** | flag |
| | `0x29DE` b4 | `$D732` | `BIT_DUNGEON_WARP` | **You fell down a hole** | flag |
| | `0x29DE` b6 | `$D732` | `BIT_ESCAPE_WARP` | **Dig / Escape Rope / blacked out** | flag |
| | `0x29DF` b2 | `$D733` | `BIT_FORCED_WARP` | **Doors fire without walking into them** | flag |
| ❌ | `0x29E7` | `$D73B` | `wWarpedFromWhichWarp` | **Came in through door #** (nothing reads it) | 0–255 |
| ❌ | `0x29E8` | `$D73C` | `wWarpedFromWhichMap` | **Came from map** (nothing reads it) | map id |

**Neighbours that belong to other panels but are warp-adjacent:**
`wMovementFlags` (`0x29E2`) bits 0/1/2 = `BIT_STANDING_ON_DOOR` / `BIT_EXITING_DOOR` /
`BIT_STANDING_ON_WARP` (v2: `AreaPlayer`'s `standingOnDoor` / `movingThroughDoor` / `standingOnWarp`);
`wStatusFlags7` bit 7 = `BIT_USED_FLY` (v2: `AreaPlayer::flyOutofBattle`);
`wCurMapTileset` bit 7 (`0x2613`) = `BIT_NO_PREVIOUS_MAP`, **the linchpin of §1**.

**Not in the save at all:** `wStandingOnWarpPadOrHole` (it lives outside the Main Data block) — so
"warp pad vs. hole" is derived from the tile, not stored. See
[`tiles.md`](tiles.md) → `warp_pad_hole_tile_ids`.

---

## 7. The bugs this found in v2 (fix before building UI on them)

Exactly the shape of the sprite pass: the model is a straight port of v1, and it carries its guesses.

| | v2 has | The truth |
|---|---|---|
| 1 | `AreaMap::blackoutDest` — a **bool** at `0x29DE` bit 6, doc'd *"Flag (may be unused)"* | It is **`BIT_ESCAPE_WARP`** — Dig / Escape Rope / blackout. Not a destination, not unused, and not `AreaMap`'s business. **Rename + move to the warp panel.** |
| 2 | `AreaWarps::skipJoypadCheckWarps` — *"Skips check for warp after not collided (Forced Warp)??"* | It is **`BIT_FORCED_WARP`**. The `??` can go. |
| 3 | `AreaWarps::setTo()` / `randomize()` invent `dungeonWarpDestMap`, `specialWarpDestMap`, `whichDungeonWarp`, `warpedFromWarp` | **Three of the four are illegal values** (§5), two can hurt a console. `setTo()` should set the **warp list** and **touch no state byte**. Dormant only until the `MapsDB` deep-link lands. |
| 4 | `whichDungeonWarp` randomized `rangeExclusive(0, n)` | The game's hole numbers are **1-based** (`IsPlayerOnDungeonWarp` writes `wCoordIndex`). A 0 never matches. |
| 5 | `warpedFromWarp` / `warpedfromMap` presented as meaningful state | **Nothing in the game reads them.** Say so. |
| 6 | `wLastMap` + `wLastBlackoutMap` live in `WorldGeneral`, invisible to anyone editing warps | The two most consequential warp bytes in the save. **Surface them on the map screen.** (No C++ needed — they are already modelled.) |
| 7 | `AreaPlayer::isBattle` / `isTrainerBattle` (`0x29D9` b6/b7) | Really **`BIT_TALKED_TO_TRAINER`** / **`BIT_PRINT_END_BATTLE_TEXT`** — and **both are zeroed on load** with the rest of `wStatusFlags3`. |

---

## 7b. The cry-wolf trap — and it nearly shipped

The hazard's warning has **two** conditions, not one, and the screenshot review is what found it.

The fixture save — an entirely ordinary file — holds `dungeonWarpDestMap = 194` (Victory Road 2F) and
`whichDungeonWarp = 0`. That **pair is not in `DungeonWarpList`**, so the first cut lit a red `!` on
both fields.

But look at what the game does:

```asm
IsPlayerOnDungeonWarp::
	xor a
	ld [wWhichDungeonWarp], a   ; <- ZERO. The very first instruction.
	...
```

**0 is the resting value.** The game writes it every time you are *not* standing on a hole — which is
almost always — so **essentially every save anybody has ever made carries one**. And `BIT_DUNGEON_WARP`
is off, so `PrepareForSpecialWarp` never reaches the table at all: the console will never look at
either byte.

So the warning is **true and useless**: it fires on every file ever opened, and it is exactly the
mistake the sprite *"your cast has changed"* notice made in its first cut (see
[`sprites.md`](sprites.md) → Part 6). **Noise is a bug.**

> ### The rule
>
> **`legal`** — is the value in the table? *(a fact)*
> **`armed`** — will the console actually read it, as things stand? *(a different fact)*
>
> The **red `!`** fires only on **`!legal && armed`**. An out-of-table value nothing is going to read
> gets a **quiet grey line**, not an alarm.
>
> `armed` for the fly destination is `BIT_FLY_OR_DUNGEON_WARP`; for the hole pair it is that **and**
> `BIT_DUNGEON_WARP` — because that is precisely which branch of `PrepareForSpecialWarp` reaches which
> table.

And the **map and the hole are judged separately** — the first cut failed *both* whenever the pair was
wrong, so Victory Road 2F (a perfectly good hole map) came up flagged because of the 0 beside it. Two
fields, two questions.

Pinned by `tst_warps::guns_dontCryWolfOnAnOrdinarySave`.

## 8. Tools

- **`scripts/emu/probe_warp_persistence.py`** — boots the real cartridge with three tampered saves
  and reports, byte by byte, which warp values the console keeps and which it rewrites. Local-only,
  ROM-gated (SKIPs without `assets/references/backup.gb`); see
  [`emulator-verification.md`](emulator-verification.md).

## 9. Sources

Everything above is read out of `pret/pokered` and then **checked against the cartridge**:

| What | Where |
|---|---|
| The warp list, `CheckWarpsNoCollision`, `WarpFound1/2`, `LoadMapHeader` | `home/overworld.asm` |
| The linchpin (`set BIT_NO_PREVIOUS_MAP` on save load) | `engine/menus/save.asm` → `LoadMainData` |
| `PrepareForSpecialWarp`, `LoadSpecialWarpData`, the fly/dungeon tables | `engine/overworld/special_warps.asm` |
| `FlyWarpDataPtr`, `DungeonWarpList`, `DungeonWarpData` | `data/maps/special_warps.asm` |
| Arrival positioning (`LoadDestinationWarpPosition`) | `engine/overworld/tilesets.asm` |
| `IsPlayerOnDungeonWarp` (`wWhichDungeonWarp` is 1-based) | `engine/overworld/hidden_events.asm` |
| `BIT_ESCAPE_WARP` writers (Dig / Escape Rope / blackout) | `engine/menus/start_sub_menus.asm`, `engine/items/item_effects.asm`, `engine/events/black_out.asm` |
| Every flag constant | `constants/ram_constants.asm` |
| The alias that kills `wStatusFlags3` | `ram/wram.asm` (`wCableClubDestinationMap::` immediately above `wStatusFlags3:: db`) |
