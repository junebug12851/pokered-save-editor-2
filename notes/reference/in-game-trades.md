# In-game trades — the ten NPC swaps, and the two bytes that remember them {#in-game-trades}

The research behind [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 17**.

> **Fairy Fox, 2026-07-17:** *"We now need to add trade data in, there's like 10 trades that exist and
> they all need to be in the persistent map storage panel. However not all of them may have map
> coordinates, im fine with including unused ones if they have map coordinates but if its unused and
> truly has no coordinates it has to go in general."*

Their count is exact: **10 trades**, `NUM_NPC_TRADES`. **9 are located** on a real NPC's tile; **1 is
unused and has no coordinates at all** → the **General** page (§7).

Everything below is read from `pret/pokered`, and the save behaviour is
**console-verified** — `scripts/emu/probe_in_game_trades.py`.

## 1. The one-line answer

| | |
|---|---|
| **Save field** | `wCompletedInGameTradeFlags` — **2 bytes**, file **`0x29E3`–`0x29E4`** (WRAM `$D737`–`$D738`) |
| **Count** | **10** trades = 10 bits. **Bits 2–7 of `0x29E4` are spare** and the game never touches them |
| **Bit math** | bit `i` = byte `0x29E3 + i/8`, mask `1 << (i%8)` — the same `FlagAction` the event flags use |
| **Home** | **`SECTION "Main Data"`** — inside the saved block, same neighbourhood as the event flags |
| **Already modelled?** | **Yes.** `WorldTrades`, at the correct offset, with the correct count. See §6 |
| **Has a location?** | **9 of 10** — the trading NPC's own tile. The 10th is unused (§7) |
| **What the flag DOES** | Gates the *whole* trade: set = the NPC only ever says their after-trade line again |

**The bit does one job and does it completely.** `DoInGameTradeDialogue` `FLAG_TEST`s it first; if
set, it prints `TRADETEXT_AFTER_TRADE` and returns. **Nothing else gates a trade** — not an event
flag, not a script step. So clearing the bit **genuinely re-arms the trade**: the NPC will offer, and
take, the swap again. That is a real, clean save-editing effect and the reason this feature is worth
having.

## 2. The ten, from `data/events/trades.asm`

The table is `TradeMons`, one `npctrade` row each, `table_width 3 + NAME_LENGTH`:

```asm
MACRO npctrade
; give mon, get mon, dialog id, nickname
	db \1, \2, \3
	dname \4, NAME_LENGTH
ENDM
```

| Bit | Constant | You give | You get | Dialogue | **Nickname** | Trader (class) | Map | (x, y) |
|---:|---|---|---|---|---|---|---|---:|
| **0** | `TRADE_FOR_TERRY` | Nidorino | Nidorina | Casual | **TERRY** | Youngster ⚠️ walks | Route 11 Gate 2F (86) | (4, 2) |
| **1** | `TRADE_FOR_MARCEL` | Abra | Mr. Mime | Casual | **MARCEL** | Gameboy Kid | Route 2 Trade House (48) | (4, 1) |
| **2** | `TRADE_FOR_CHIKUCHIKU` | Butterfree | Beedrill | Happy | **CHIKUCHIKU** | — | **UNUSED — none** | — |
| **3** | `TRADE_FOR_SAILOR` | Ponyta | Seel | Casual | **SAILOR** | Scientist | Cinnabar Lab Fossil Room (170) | (7, 6) |
| **4** | `TRADE_FOR_DUX` | Spearow | Farfetch'd | Happy | **DUX** | Little Girl | Vermilion Trade House (196) | (3, 5) |
| **5** | `TRADE_FOR_MARC` | Slowbro | Lickitung | Casual | **MARC** | Youngster ⚠️ walks | Route 18 Gate 2F (191) | (4, 2) |
| **6** | `TRADE_FOR_LOLA` | Poliwhirl | Jynx | Evolution | **LOLA** | Gambler | Cerulean Trade House (63) | (1, 2) |
| **7** | `TRADE_FOR_DORIS` | Raichu | Electrode | Evolution | **DORIS** | Gramps | Cinnabar Lab Trade Room (168) | (1, 4) |
| **8** | `TRADE_FOR_CRINKLES` | Venonat | Tangela | Happy | **CRINKLES** | Beauty | Cinnabar Lab Trade Room (168) | (5, 5) |
| **9** | `TRADE_FOR_SPOT` | Nidoran♂ | Nidoran♀ | Happy | **SPOT** | Little Girl | Underground Path Route 5 (71) | (2, 3) |

Map ids are the game's own (`constants/map_constants.asm`) and are **`maps.json`'s `ind`** — verified
on all eight, and every map's `sprites` count matches pret's object count exactly.

## 2a. ⚠️ THE NAME IS THE POKÉMON'S NICKNAME — there is no trader name

**The single easiest thing in this feature to get wrong, and the brief got it wrong**, so it is
written here first. *"Trade data including character/trader name"* — but **TERRY is not a man. TERRY
is the Nidorina.**

The macro's own comment says so — `; give mon, get mon, dialog id, nickname` — and the code is
unambiguous:

```asm
	ld a, [hli]              ; give species
	ld a, [hli]              ; receive species
	ld a, [hli]              ; dialog id
	push af
	ld de, wInGameTradeMonNick   ; <- the 4th field goes HERE
	ld bc, NAME_LENGTH
	call CopyData
```

and `InGameTrade_CopyDataToReceivedMon` copies `wInGameTradeMonNick` straight into
`wPartyMonNicks` for the mon you just received. **DUX is the Farfetch'd's nickname, and it is on your
Farfetch'd forever.**

**The traders have no personal names at all.** They are sprite classes, and the classes *repeat* —
two Youngsters, two Little Girls, two Scientists — so a class name **cannot** identify a trade on its
own.

> **Ruling (leadership, 2026-07-17):** *"if the trader doesnt have a nickname then use there class
> name same logic as pokemon for species name and nickname"*.
>
> So a trader is modelled **exactly like a Pokémon**: a **species** (the class — Youngster, Gambler,
> Gramps) and a **nickname** (which they haven't got). No nickname → **show the class name**. The
> received Pokémon keeps the same shape: species (Farfetch'd) + nickname (DUX). One rule, both
> subjects.

## 2b. The three dialogue sets, and the lie in the middle one

`TRADE_DIALOGSET_CASUAL` / `_EVOLUTION` / `_HAPPY` pick which of `TradeTextPointers1-3` the NPC
speaks from. **`_EVOLUTION` does not mean the mon evolves** — pret's own comment on the table:

> *The two instances of `TRADE_DIALOGSET_EVOLUTION` are a leftover from the Japanese Blue trades,
> which used species that evolve. … English Red/Blue uses the original JP Red/Green trades but with
> the JP Blue post-trade text.*

So **LOLA (Poliwhirl→Jynx)** and **DORIS (Raichu→Electrode)** make their trader say *"The POLIWHIRL
you traded to me went and evolved!"* — about a Pokémon that **cannot evolve**. It is a genuine
localisation scar, it is in the shipped English cartridge, and the UI should say so rather than
tidying it away.

## 2c. The trade-evolution check is DEAD in English Red/Blue

`InGameTrade_CheckForTradeEvo` fires only if the **received** mon's name starts `G` (GRAVELER) or
`SP` (HAUNTER's early English name "SPECTRE"):

```asm
	ld a, [wInGameTradeReceiveMonName]
	cp 'G' ; GRAVELER
	jr z, .nameMatched
	cp 'S'          ; "SPECTRE" (HAUNTER)
	ret nz
	ld a, [wInGameTradeReceiveMonName + 1]
	cp 'P'
	ret nz
```

The ten received species are Nidorina, Mr. Mime, Beedrill, Seel, Farfetch'd, Lickitung, Jynx,
Electrode, Tangela, Nidoran♀. **Only SEEL starts with `S`, and its second letter is `E`, not `P`.**
So the routine **never fires**. It is a checked-name check against names that no longer exist —
pret's comment confirms the final release replaced the Graveler and Haunter trades.

## 3. What you actually receive — the four facts a details view owes

From `InGameTrade_DoTrade` / `InGameTrade_PrepareTradeData` / `InGameTrade_CopyDataToReceivedMon`:

| | |
|---|---|
| **Level** | **The level of the Pokémon you gave.** `ld a, [wWhichPokemon]` → the party mon's `wPartyMon1Level` → `wCurEnemyLevel`. Trade a level-5 Spearow, get a level-5 Farfetch'd |
| **Nickname** | The table's 4th field (§2a). `$80` is written to `wMonDataLocation` **to prevent you naming it** |
| **OT name** | Literally **"TRAINER"** — `InGameTrade_TrainerString: dname "<TRAINER>"`, and `constants/charmap.asm`: `charmap "<TRAINER>", $5d ; "TRAINER"` |
| **OT ID** | **Random** — `call Random` → `hRandomAdd` → `wTradedEnemyMonOTID`. ⚠️ Which is why a traded mon can and does **disobey** |

**You must be holding the exact species asked for.** `InGameTrade_DoTrade` compares
`wCurPartySpecies` against `wInGameTradeGiveMonSpecies` and prints `TRADETEXT_WRONG_MON` otherwise.
Any level, any nickname, any DVs — only the species is checked.

## 4. Where a trade lives on the map — the join is EXACT

A trade is reached by **talking to an NPC**, and that NPC is an ordinary `object_event` with a text
id. The trade is bound to the **text**, not the tile:

```asm
CeruleanTradeHouseGamblerText:
	text_asm
	ld a, TRADE_FOR_LOLA
	ld [wWhichTrade], a
	predef DoInGameTradeDialogue
```

So the link is **(map id, text id) → trade index**, mechanically, from the source — never proximity.
And `maps.json` already carries both halves: each `sprites[]` entry has its `text` (1-based), and
`textEntries[]` has the matching `id`.

**The join has a beautiful tell.** A trade NPC's `textEntries` row has **`string: null`** — because
its text is `text_asm`, *code*, not words. Every one of the nine trade NPCs is a null string in our
own data today. **The trades are the missing words**, and filling them in is what this feature is.

⚠️ **Two of the nine traders WALK.** TERRY's and MARC's Youngsters are
`object_event 4, 2, SPRITE_YOUNGSTER, WALK, LEFT_RIGHT, …`. The coordinate above is the **spawn**
tile — where the object is *defined*, which is where the save's sprite slot starts and where the
box belongs. It is not a promise about where the man is standing right now.

⚠️ **Cinnabar Lab Trade Room holds two trades** (DORIS at (1,4), CRINKLES at (5,5)) — two spots, two
tabs, one page. It is the map that proves the tabs earn their keep.

## 5. Drawing it — the kind, the unit, the outline

Per the standard in [`map-storage-locations.md`](map-storage-locations.md) §2g:

| | |
|---|---|
| **kind** | `trade` |
| **unit** | **`halfBlock`** (16×16) — an `object_event` rides `wYCoord`/`wXCoord`, the walk grid |
| **section** | `trade` → the Map Storage panel's **Trades** section |
| **outline** | **dashed + hollow** — *fixed*. You change **whether the trade is done**, never where it is |
| **colour** | its layer's swatch, like everything else. The Layers panel is the legend |

⚠️ **The cell will usually render solid anyway, and that is correct** — the trader is a movable
sprite in the same cell, and rule 4 of the standard says *one movable makes the whole cell solid*.
The trade's own tab is still dashed-family; the box is the cell's handle, not the trade's.

⚠️ **Coordinates come from the ROM (`MapDBEntry::sprites`), not the save's sprite slot** — the same
call `filterFlag` already makes, for the same reason: the trade exists at the tile its NPC is
*defined* on whether or not that NPC is currently drawn there. Consistency with the neighbouring kind
beats a cleverer answer.

## 6. Our model — already correct, and this is worth saying plainly

`WorldTrades` (`worldtrades.{h,cpp}`) reads `getBitField(0x29E3, 2)` and caps at `tradeCount = 10`.
**Offset right, count right, byte count right.** ✅ **A rare pass with nothing to fix first** — the
opposite of sprites (four bugs), tilesets (three), warps (seven).

✅ **And the spare bits are safe.** `setBitField` guards every write with `if ((i + n) < src.size())`,
and `save()` hands it exactly 10 bools — so **bits 2–7 of `0x29E4` are never written**. This was
checked, not assumed, because "writes 2 bytes but only owns 10 bits" is precisely the shape that
breaks this project's first rule. It doesn't.

**What is missing is everything above the bits:** there is no trade DB, no names, no species, no
coordinates, no UI. `WorldTrades` is ten anonymous booleans.

**Why a grep for `InGameTrade` / `TRADE_FOR_` / `tradeFlags` found nothing:** the model carries
none of those tokens. The file is `worldtrades.cpp` and the field is `completedTrades`. **Search the
concept, not the upstream spelling** — a near-miss here would have meant re-modelling a field that
was already right.

## 7. The unused one — CHIKUCHIKU, and why it goes to General

`TRADE_FOR_CHIKUCHIKU` (bit 2, Butterfree → Beedrill, nicknamed **CHIKUCHIKU** — Japanese for the
prickling of something sharp) is marked `; unused` in **both** the table and the constants, and a
sweep of `scripts/*.asm` confirms it: **nine `TRADE_FOR_*` references, and this is not one of them.**

No script → no NPC → no text id → **no coordinates**. Per §"The standing rule" in
`map-storage-locations.md`: *a spot whose location cannot be established gets **NO box** — never a
guessed one.* So CHIKUCHIKU gets **no map, no box, no tab**.

**But the bit is real, live, and editable** — it sits in the save between MARCEL's and SAILOR's, and
the console will read it if anything ever asks. It is exactly the "placeless save data" case.

> **Ruling (leadership, 2026-07-17):** *"there was supposed to be a general page left in for exactly
> this reason please re-create it"*.

**Recorded honestly:** no General page has ever existed — `git log -S` across `MapStoragePanel.qml`
and `storagePages` finds none, and the plan never specified one. It was **intent that never landed**,
not a regression. It is created now, as the first page of the Map Storage combo, and it is the home
for every future piece of save data that belongs to no map.

## 8. Traps, collected

1. **The nickname is not the trader** (§2a). The brief said trader name; the data has none.
2. **`TRADE_DIALOGSET_EVOLUTION` doesn't mean evolution** (§2b) — a JP Blue leftover, about two
   Pokémon that can't evolve.
3. **The trade-evo check is dead code** (§2c) — it tests for species the final game removed.
4. **Two traders walk** (§4) — the coord is the spawn tile, not a live position.
5. **One map, two trades** (§4) — Cinnabar Lab Trade Room.
6. **The received mon's OT ID is random** (§3) — so it can disobey. Not a bug; the game does that.
7. **The received mon takes the *given* mon's level** (§3) — not a fixed level.
8. **Nine `TRADE_FOR_*` in scripts, ten in the table** (§7). The gap is the whole point; a count that
   comes out 10 from `scripts/` means the parser matched something it shouldn't.

## 9. Console verification

`scripts/emu/probe_in_game_trades.py` — the standing rule: a source read is a hypothesis until the
cartridge agrees. Three questions, because the flag being *durable* is what the whole UI rests on:

1. **Is the bit kept across a Continue?** (`0x29E3` bit *n* set → save → boot → read `$D737`.)
2. **Does a set bit actually mute the offer?** — talk to the NPC and confirm the after-trade line.
3. **Does clearing it re-arm the trade?** — the editing effect we are selling.

Findings land here.

---

**See also:** [`map-storage-locations.md`](map-storage-locations.md) (the location model + the
standard) · [`town-visited.md`](town-visited.md) (the other flag array on the same panel, and the
one that *isn't* durable) · [`../plans/map-screen.md`](../plans/map-screen.md) → Phase 17.
