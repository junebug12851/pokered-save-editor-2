# Wild Pokémon — the grass & water encounter tables

The map's two wild-encounter tables (grass and water), how they sit in the save, and the two things
our model had wrong. Verified against `pret/pokered` and a real cartridge save (`BaseSAV`).

> Not to be confused with the **post-battle cooldown** flag, which is a different byte with its own
> note: [`wild-encounter-cooldown.md`](wild-encounter-cooldown.md).

## What the save holds

Two tables, each **one rate byte + ten slots**, gated so that a rate of **0 means "no wild Pokémon of
that kind here"** (the game skips the list entirely):

| Field | Offset | Notes |
|---|---|---|
| `wGrassRate` | `0x2B33` | 0 = no wild grass Pokémon. Higher = encounters more often. |
| `wGrassMons` | `0x2B34` | 10 slots × 2 bytes = 20 bytes. |
| *(gap)* | `0x2B48`–`0x2B4F` | 8 unused bytes (`ds 8` in `wram.asm`). |
| `wWaterRate` | `0x2B50` | 0 = no wild water Pokémon (surf/fish). |
| `wWaterMons` | `0x2B51` | 10 slots × 2 bytes. |

A slot is **two bytes, in the order `LEVEL, then SPECIES`** — the cartridge's own layout
(`pret/pokered` `data/wild/maps/<Map>.asm`: `db level, species`). The game copies each map's wild data
verbatim into the WRAM buffer (`LoadWildData` is a straight `CopyData`), which is *why* the save holds
it and why the **MissingNo. glitch** exists at all — it is leftover wild data being read as Pokémon
(the comment in `engine/battle/core.asm` calls `wLinkEnemyTrainerName == wGrassRate` out by name).

The **species is the internal index**, not the Pokédex number — e.g. `165` = RATTATA, `36` = PIDGEY.
(In our DB this is `PokemonDBEntry::ind`; the Pokédex number is a *separate*, 0-based field.)

Verified on `BaseSAV` (Pallet Town, rate 0, so these are leftover bytes): `0x2B34 = 3` (a level),
`0x2B35 = 165` = RATTATA (a species). Species-first would put 165 in the level byte — it doesn't.

## Slot rarity is the slot's POSITION

There are ten slots and a slot's chance is **where it sits in the list**, not which mon it is. From
`pret/pokered` `data/wild/probabilities.asm` (`WildMonEncounterSlotChances`, x / 256):

| Slot | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|---|
| /256 | 51 | 51 | 39 | 25 | 25 | 25 | 13 | 13 | 11 | 3 |
| ≈ % | 19.9 | 19.9 | 15.2 | 9.8 | 9.8 | 9.8 | 5.1 | 5.1 | 4.3 | 1.2 |

(They sum to exactly 256.) So **reordering the ten slots re-weights the encounter table** — the editor
shows this percentage per slot and lets you drag to reorder. All ten slots are read when the rate is
> 0, so a table has no "empty" slots: fill them all.

## Persistence — an edit is LIVE on Continue (same linchpin as warps & signs)

`LoadWildData` is called from **inside `LoadMapHeader`** (`home/overworld.asm`), *after* the
`BIT_NO_PREVIOUS_MAP` early-return:

```
LoadMapHeader::
    ...
    bit BIT_NO_PREVIOUS_MAP, b
    ret nz                 ; <-- Continue bails here
    ...                    ; header copy, warps, signs, sprites
    callfar LoadWildData   ; <-- never reached on Continue
```

`LoadMainData` *sets* `BIT_NO_PREVIOUS_MAP` on the saved tileset byte as it reads the save, so on the
**Continue path `LoadMapHeader` returns before `LoadWildData` runs** — and the save's own wild tables
are the ones the console uses. This is the **exact linchpin the warps/signs research established and
the emulator confirmed** ([`warps.md`](warps.md)): an edited table is genuinely live on Continue, and
the game restores the map's original wild Pokémon when the player **leaves the map and walks back in**.
(Verified from the disassembly here; the mechanism itself is console-verified via the warp/sign
probes. A dedicated `scripts/emu` probe for wild data is a cheap follow-up but not blocking — same
routine, same bit.)

## What our model had WRONG (fixed 2026-07-15)

Two real bugs, both the kind the standing "research lands in the notes" rule exists to catch:

1. **Byte order was inverted.** `AreaPokemonWild::load/save` read `index` (species) *first* and `level`
   *second* — the opposite of the cartridge's `db level, species`. So every real save was read with
   species and level swapped, and every write produced cartridge-wrong bytes. **Fixed**: level is the
   first byte, species the second. Pinned by `tst_area_pokemon::wildTables_byteOrderIsLevelThenSpecies`
   (sets level 7 + species 165, flattens, asserts `0x2B34 = 7`, `0x2B35 = 165`).

2. **The DB's Pokédex number is 0-based.** `PokemonDBEntry::pokedex` is 0-based (Bulbasaur = 0), but the
   mon-icon files are 1-based (`001-bulbasaur.svg`). The `PokemonBoxView` already compensated with
   `(itemDex + 1)`; the new Wild Pokémon panel does the same in `WildMonList.iconUrl`. (Not a save bug —
   a UI icon-path detail — but it cost a render pass, so it's written down.)

## Where it lives in the app

- **Model:** `AreaPokemon` (`savefile/.../area/areapokemon.{h,cpp}`) — `grassRate`/`waterRate` + two
  `AreaPokemonWild[10]` tables. `AreaPokemonWild` is `(level, species-index)`.
- **MapModel** exposes `grassRate`/`waterRate`/`grassEnabled`/`waterEnabled`, `grassMons()`/`waterMons()`
  (each slot as `{slot, index, level, name, dex, glitch, percent}`), and the per-slot species/level
  setters + `moveGrassMon`/`moveWaterMon` (reorder). Each setter writes only the byte(s) it names.
- **UI:** `WildPokemonPanel.qml` (left dock) → two `WildMonList.qml` sections (grass, water). Enable
  switch (`rate > 0`), an encounter-chance slider (Low↔High), and the ten slots drawn like the Pokémon
  box: percent upper-left, editable level upper-right, artwork centre, species picker on click, drag to
  reorder. **Enabling a blank table seeds ten random mons** (like the box's new-random); **disabling
  never clears** — it only sets the rate to 0, so re-enabling brings the same table back.

See [`../plans/map-screen.md`](../plans/map-screen.md) → Phase 8 (Encounters).
