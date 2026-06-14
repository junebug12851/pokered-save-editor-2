# Gen 1 Save File Structure {#savestructure}

Human-readable companion to `structure.bt` (the 010 Editor binary template authored by
Twilight). The `.bt` file is the **byte-exact oracle** — when the two ever disagree, the
`.bt` wins and this document is wrong. This page exists so the layout can be read, reviewed,
and cross-referenced (it is the offset oracle the test suite is written against) without
opening 010 Editor.

A Pokémon Red/Blue save (`.sav`) is exactly **32 KiB (`0x8000` bytes)** — a verbatim dump of
the cartridge's battery-backed SRAM. It is divided into **four 8 KiB (`0x2000`) banks**.

| Bank | File range | Contents |
|------|------------|----------|
| Bank 0 | `0x0000`–`0x1FFF` | Hall of Fame records + sprite/scratch buffers |
| Bank 1 | `0x2000`–`0x3FFF` | **Main data** — the live game state (player, party, current box, world) |
| Bank 2 | `0x4000`–`0x5FFF` | PC storage **Boxes 1–6** + their checksums |
| Bank 3 | `0x6000`–`0x7FFF` | PC storage **Boxes 7–12** + their checksums |

## Memory ↔ SAV address conversion

The Main Data block in Bank 1 is a byte-for-byte image of the game's working RAM (WRAM).
To convert between an in-game WRAM address and its file offset:

```
file_offset = wram_addr - 0xAD54        (convert from memory to SAV)
wram_addr   = file_offset + 0xAD54      (convert from SAV to memory)
```

For example Main Data begins at file `0x25A3`, which is WRAM `0xD2F7` (`wPokedexOwned`).

---

## Bank 0 — Hall of Fame & scratch (`0x0000`–`0x1FFF`)

| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| `0x0000` | `0x188` | Sprite buffer 0 | Decompression/scratch space, not meaningful in a saved file |
| `0x0188` | `0x188` | Sprite buffer 1 | |
| `0x0310` | `0x188` | Sprite buffer 2 | |
| `0x0498` | `0x100` | Unused | |
| `0x0598` | `50 × HOF_RECORD` | Hall of Fame | 50 records, 6 mons each (see `HOF_RECORD`) |
| — | `0x7A8` | Unused | Pads out to the end of the bank |

`HOF_RECORD_POKEMON` is `species(1) + level(1) + name(0xB) + padding(0x3)` = 16 bytes;
`HOF_RECORD` is 6 of them.

---

## Bank 1 — Main data (`0x2000`–`0x3FFF`)

| Offset | Size | Field |
|--------|------|-------|
| `0x2000` | `0x598` | Unused |
| `0x2598` | `0xB` | **Player name** (terminated Gen-1 text) |
| `0x25A3` | … | **Main Data** block (table below) |

### Main Data block (begins `0x25A3` = WRAM `0xD2F7`)

The block follows the WRAM layout exactly, so each field's WRAM address is
`file_offset + 0xAD54`. Key verified offsets:

| File offset | WRAM | Size | Field | Notes |
|-------------|------|------|-------|-------|
| `0x25A3` | `0xD2F7` | 19 | Pokédex **owned** | Bitfield, 151 species + 1 unused bit (`POKEDEX_BITS`) |
| `0x25B6` | `0xD30A` | 19 | Pokédex **seen** | Same bitfield layout |
| `0x25C9` | `0xD31D` | 42 | Bag items | `count(1) + 20×{id,qty} + terminator(1)` |
| `0x25F3` | `0xD347` | 3 | **Money** | 3-byte big-endian BCD |
| `0x25F6` | `0xD34A` | 11 | Rival name | |
| `0x2601` | `0xD355` | 1 | Options | Text speed / battle style / battle animation bits |
| `0x2602` | `0xD356` | 1 | **Badges** | 8 badge bits (Boulder…Earth) |
| `0x2604` | `0xD358` | 1 | Letter delay flags | |
| `0x2605` | `0xD359` | 2 | Player ID | Trainer ID |

After the header above, Main Data continues (in this order; see `.bt` for exact field
sizes) with the **Map / overworld state** block (`MAP`: music, current map, coordinates,
tileset, connections, warps, signs, sprite movement data), the **PC box items** list
(`count + 50×{id,qty} + terminator`), the current-box index (`CUR_BOX`: 7-bit box number
+ "don't format" flag), HOF record count, **slot-machine coins** (2 bytes BCD), the
**missable-objects** bitfield + list, the large **game-progress** event flags
(`GAME_PROG`), owned hidden items/coins, the player movement mode (walk/bike/surf), towns
visited, safari steps, fossil state, a long run of **`VARIOUS_FLAGS1`–`8`** and
**`DEFEATED_GYMS`** bitfields, the 320-byte **completed-game-events** array, the
enemy/grass-encounter scratch unions, **play time**, **Safari Zone** state, and the
**Daycare** record (`dayCareInUse + name + OT + BOX_MON`).

### Sprite, party, and current-box sections (still Bank 1)

| Section | Contents |
|---------|----------|
| `SPRITE_DATA_SEC` | Player + 15 overworld sprites (`SPRITE_DATA_ENTRY`) and their extra data (`SPRITE_DATA_EXTRA_ENTRY`) |
| `PARTY_DATA_SEC` | `partyCount(1)` + 6 species bytes + terminator, then 6 × `PARTY_MON`, then party OT names and nicknames |
| `BOX_DATA_SEC` | The **current box** working copy (same layout as a stored box, below) |

### Bank 1 trailer

| Field | Notes |
|-------|-------|
| Tileset type | 1 byte |
| **Main data checksum** | 1 byte at file `0x3523` — 8-bit checksum over the Main Data region (`0x2598`–`0x3522`). The editor recomputes this on save; it does not reject a file with a bad one. |
| Unused | `0xADC` bytes padding to `0x3FFF` |

---

## Banks 2 & 3 — PC storage boxes

Each bank holds **six boxes** as `BOX_DATA_SEC`, followed by checksum bytes and padding.

| | Bank 2 | Bank 3 |
|---|--------|--------|
| Boxes | 1–6 | 7–12 |
| Box data | `0x4000`–`0x5A4B` (6 × `0x462`) | `0x6000`–`0x7A4B` |
| **All-boxes checksum** | `0x5A4C` | `0x7A4C` |
| Per-box checksums (6) | `0x5A4D`–`0x5A52` | `0x7A4D`–`0x7A52` |
| Unused | `0x5AD` bytes to `0x5FFF` | `0x5AD` bytes to `0x7FFF` |

> The bank-2 all-boxes checksum lives at **`0x5A4C`** over range **`0x1A4C`** (6 boxes ×
> `0x462`). Getting this off by one corrupts Box 6's last data byte — see
> `notes/reference/fix-patterns.md`.

### `BOX_DATA_SEC` (one box = `0x462` = 1122 bytes)

| Size | Field |
|------|-------|
| 1 | Count (number of mons in the box) |
| 21 | Species list (terminated) |
| `20 × 33` | 20 × `BOX_MON` (33 bytes each) |
| `20 × 11` | Original-trainer names |
| `20 × 11` | Nicknames |

---

## Reusable record types

### `BOX_MON` (33 bytes) — a stored Pokémon

`species(1)`, `hp(2)`, `boxLevel(1)`, `status(1)`, `type1(1)`, `type2(1)`, `catchRate(1)`,
`moves(4)`, `originalTrainer(2)`, `exp(3)`, `hpExp(2)`, `attackExp(2)`, `defenseExp(2)`,
`speedExp(2)`, `specialExp(2)`, `IV(2)`, `PP(4)`.

- **`IV`** packs four 4-bit nibbles: attack, defense, speed, special (HP IV is derived from
  the parity of the others).
- **`PP`** is, per move, a 6-bit current-PP value + a 2-bit PP-Up count.

### `PARTY_MON` (44 bytes) — a party Pokémon

A `BOX_MON` plus the battle-ready computed stats: `level(1)`, `maxHP(2)`, `attack(2)`,
`defense(2)`, `speed(2)`, `special(2)`.

### Other small records

- **`ITEM_LIST_ENTRY`** — `{ id, amount }` (2 bytes); item lists are
  `count + N×entry + 0xFF terminator`.
- **`INDEX_LIST_ENTRY`** — `{ id, index }` (2 bytes).
- **`WARP_ENTRY`** — `{ y, x, destWarp, destMap }` (4 bytes).
- **`POKEDEX_BITS`** — 152 single-bit flags (151 species + 1 unused), 19 bytes,
  least-significant-bit-first by national dex order.

---

## See also

- `structure.bt` — the authoritative 010 Editor binary template (this document's source).
- `notes/reference/gen1-knowledge.md` — save-format + gameplay domain knowledge.
- `notes/reference/fix-patterns.md` — checksum offsets and corruption fixes.
