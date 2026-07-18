# The fossil — two bytes, and the one that actually decides {#fossil-revival}

The research behind [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 20**, and the last
field of v1's map page to come across.

> **Fairy Fox, 2026-07-17:** *"throw in the very last bit of data for map, fossil item given resulting
> pokemon. That will complete all the map data transference from v1 to v2 for map page."*

## 1. The one-line answer

| | |
|---|---|
| **Save fields** | `wFossilItem` = file **`0x29BB`** (`$D70F`) · `wFossilMon` = file **`0x29BC`** (`$D710`) |
| **Kind** | two **plain bytes** — an **item id** and a **species index**. Not flags |
| **Home** | `SECTION "Main Data"`, immediately after `wSafariSteps` (`0x29B9`, 2 bytes) |
| **Already modelled?** | **Yes** — `WorldOther::fossilItemGiven` / `fossilPkmnResult`, both at the right offset |
| **Location** | **Cinnabar Lab Fossil Room (170)**, the **Scientist at (5, 2)** — ⚠️ the one that **WALKS** |
| **What reads them** | `GiveFossilToCinnabarLab` (writes) · `CinnabarLabFossilRoom` (reads `wFossilMon` to hand you the mon) · `LoadFossilItemAndMonName` (reads both, for the text) |

**Address check:** `wSafariSteps` = `0x29B9` (verified in [`gym-safari-state.md`](gym-safari-state.md))
is a `dw`, so it occupies `0x29B9`–`0x29BA`; `wFossilItem` is the next byte. pret's own comments name
them: *"item given to cinnabar lab"* and *"mon that will result from the item"*.

**A fourth model that was already correct.** Like `WorldTrades`, `WorldTowns` and `WorldCompleted`,
`WorldOther` had both offsets right. Nothing to fix — only to surface.

## 2. How the game fills them — `engine/events/cinnabar_lab.asm`

```asm
	ld a, [hl]                    ; the fossil you picked out of the menu
	ldh [hItemToRemoveID], a
	cp DOME_FOSSIL
	jr z, .choseDomeFossil
	cp HELIX_FOSSIL
	jr z, .choseHelixFossil
	ld b, AERODACTYL              ; <- the ELSE branch (see the trap below)
	jr .fossilSelected
.choseHelixFossil
	ld b, OMANYTE
	jr .fossilSelected
.choseDomeFossil
	ld b, KABUTO
.fossilSelected
	ld [wFossilItem], a           ; the ITEM
	ld a, b
	ld [wFossilMon], a            ; the MON it will become
```

| You hand over | You get back |
|---|---|
| **Dome Fossil** | **Kabuto** |
| **Helix Fossil** | **Omanyte** |
| **Old Amber** — *or literally anything else* | **Aerodactyl** |

⚠️ **`AERODACTYL` is a FALLTHROUGH, not a match.** The code never tests for `OLD_AMBER`. It tests for
Dome, tests for Helix, and **anything that is neither becomes Aerodactyl**. In normal play you can
never reach it with a non-fossil, because the menu is built from `wFilteredBagItems` (fossils only) —
but the *branch* has no floor under it. Worth knowing before anyone "fixes" the table.

## 3. ⚠️ THE TRAP — the two bytes are independent, and `wFossilMon` is the one that decides

**The game does not re-derive the mon from the item when it hands the Pokémon over.**
`CinnabarLabFossilRoom.asm`:

```asm
.done_reviving
	call LoadFossilItemAndMonNameBank1D
	ld hl, .FossilIsBackToLifeText
	call PrintText
	SetEvent EVENT_LAB_HANDING_OVER_FOSSIL_MON
	ld a, [wFossilMon]            ; <- reads the BYTE, not the item
	ld b, a
	ld c, 30                      ; <- always LEVEL 30
	call GivePokemon
```

So, for a save editor, the two bytes have **different powers** and it matters which you touch:

- **`wFossilMon` (`0x29BC`) is the payload.** Whatever species index sits here is what walks out of
  the machine — at **level 30**, from a full 1–255 range. Set it to Mewtwo and the lab hands you
  Mewtwo. This is a genuine, clean hack the console honours.
- **`wFossilItem` (`0x29BB`) is only ever a NAME after the fact.** Once the fossil is taken, the only
  remaining reader is `LoadFossilItemAndMonName` → `GetItemName`, which fills the *text*. Editing it
  changes what the scientist **says**, not what you get.

> ⭐ **So "fossil item given → resulting Pokémon" is not a derived pair in the save.** It looks like a
> cause and its effect, and after the moment of handover it is two independent bytes that merely
> agree. **They are not kept in sync by the game and must not be silently synced by us** — a
> mismatched pair (Dome Fossil → Mewtwo) is a legal, working, interesting save, and the derived-value
> doctrine (*"keep it in sync by default, offer to break sync"*) would be **wrong** here: there is no
> derivation left to preserve. Show both, edit both, explain which one bites.

## 4. The state machine is event flags, not these bytes

The two bytes are the *payload*; three **event flags** (already shipped among the 2,560) are the
*state*:

| Flag | Meaning |
|---|---|
| `EVENT_GAVE_FOSSIL_TO_LAB` | you handed it over |
| `EVENT_LAB_STILL_REVIVING_FOSSIL` | come back later |
| `EVENT_LAB_HANDING_OVER_FOSSIL_MON` | it's ready |

Set together by `GiveFossilToCinnabarLab`; the Scientist's script walks them. ⚠️ **If `GivePokemon`
fails** (party *and* box full), the script does `ResetEvents` on all three — deliberately rewinding
so you can come back. So the bytes can legitimately hold a mon you never received.

**Which means the Fossil section is only meaningful next to those flags** — the bytes alone don't say
whether a fossil is in the machine. The panel should show them together.

## 5. ⚠️ Two objects, two kinds of storage, one small room

**Cinnabar Lab Fossil Room (170) has exactly two people, and they do different things:**

| Object | Where | What |
|---|---|---|
| **Scientist 1** | **(5, 2)** — `WALK, LEFT_RIGHT` | **the fossil machine** (`GiveFossilToCinnabarLab`) |
| **Scientist 2** | **(7, 6)** — `STAY, UP` | the **SAILOR trade** (Ponyta → Seel) — [`in-game-trades.md`](in-game-trades.md) |

⚠️ **This pair is what caught a real bug in `import_trades.py`** (2026-07-17). The first cut let the
gap between a `text_asm` label and its `TRADE_FOR_*` span **across other labels**, so it matched
Scientist**1**'s label and Scientist**2**'s trade — filing SAILOR at **(5,2)** instead of (7,6).
**Nine trades still resolved and every total still looked right.** It was caught only by diffing the
importer's output against a hand-read of the source.

> ⭐ **The lesson, and it is this file's most portable one:** *a lazy gap in a regex is not a fence.*
> `(?:.*\n)*?` will happily reach into the next block to satisfy the match, and the result is a parse
> that succeeds with the wrong data — the worst failure mode this project has. The gap is now
> `(?!^\w+:)`-guarded. **Two objects of the same sprite class in one room is exactly the shape that
> exposes it**, and there is only one such room in the game.

⚠️ **Scientist 1 WALKS**, so (5,2) is the **spawn** tile — where the object is defined and where the
save's sprite slot starts — not where the man is standing right now. Same rule as the two walking
trade Youngsters.

## 6. Durability

Both bytes sit in `SECTION "Main Data"` between `wSafariSteps` (**5 of 6 console-verified durable**,
[`gym-safari-state.md`](gym-safari-state.md)) and `wTownVisitedFlag`/`wCompletedInGameTradeFlags`
(**both console-verified durable**, 2026-07-17). Nothing in the disassembly writes them outside
`GiveFossilToCinnabarLab`.

✅ **Console-verified durable** — `scripts/emu/probe_fossil.py` (2026-07-17). The neighbourhood is a
strong argument and it is still only an argument; the bytes were asked directly.

| Scenario | Result |
|---|---|
| **B** `DOME_FOSSIL` / `KABUTO` — the pair the game would write | **both intact** |
| **C** `DOME_FOSSIL` / **`MEWTWO`** — the mismatched pair | **both intact — `wFossilMon` still MEWTWO** |
| **D** both zeroed | **stay zero** — the loader writes neither |

⭐ **C settles the doctrine, and that is why it was worth running.** Had the console re-derived the
mon from the item, C would have come back `KABUTO`. It came back `MEWTWO`. **The two bytes are
independent; there is no derivation to preserve; the sync doctrine does not apply.** Show both, edit
both, sync neither.

### 🐺 …and the baseline caught a cry-wolf before it could ship

**BaseSAV — an ordinary save that has never touched the fossil quest — reads
`wFossilItem = 0x2A (HELIX_FOSSIL)` and `wFossilMon = 0x62`.** That is a *mismatched* pair (Helix
revives into Omanyte, `0xB2`, not `0x62`) sitting in a perfectly healthy file.

These bytes are **never initialised at new game** — they hold whatever was there until
`GiveFossilToCinnabarLab` writes them. So **a nonsense pair is the RESTING STATE, not a symptom.**

> ⚠️ **Do NOT warn on a mismatched pair.** It would fire on essentially every save anyone ever
> opened — exactly the mistake the `dungeonWarpDestMap` warning made in its first cut (see
> [`../status.md`](../status.md), 2026-07-14) and the sprite *"your cast has changed"* notice before
> it. The rule stands: **`legal` and `armed` are two different questions**, and only the event flags
> (§4) say whether the console will ever read these bytes. Out-of-table-but-inert gets a quiet note,
> never a red **!**.

## 7. Traps, collected

1. **`wFossilMon` decides, `wFossilItem` only names** (§3) — the pair is not re-derived at handover.
2. **They are NOT kept in sync** (§3) — a mismatched pair is legal and works. Do not "helpfully" sync
   them; there is no derivation to preserve.
3. **Aerodactyl is a fallthrough, not a match** (§2) — nothing tests for Old Amber.
4. **The revived mon is always level 30** (§4).
5. **The state is three event flags, not these bytes** (§4) — and they can be *rewound* if your party
   and box are both full.
6. **The Fossil Room's two Scientists are different features** (§5) — and telling them apart is what
   caught the importer bug.
7. **Scientist 1 walks** (§5) — (5,2) is a spawn tile.

---

**See also:** [`in-game-trades.md`](in-game-trades.md) (the room's *other* Scientist) ·
[`gym-safari-state.md`](gym-safari-state.md) (`wSafariSteps`, the neighbour that anchors the address)
· [`event-flags.md`](event-flags.md) (the three flags that are the actual state machine).
