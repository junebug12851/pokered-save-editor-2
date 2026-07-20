# Signs — the placards, and the map text behind them

**Status:** researched 2026-07-14, read out of `pret/pokered` (the disassembly clone at
`~/Documents/projects/pokered`) and cross-checked against the existing model. The design that
consumes it is [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 6**.

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format),
> [`warps.md`](warps.md) (the **same persistence linchpin**, and the same ROM block),
> [`sprites.md`](sprites.md) (the same "restored on re-entry" story).

---

## 0. The one-paragraph version

A **sign** is a tile you can read — a placard, a poster, the label on a building. The map's sign
list says *which tile* is a sign and *which line of the map's text* it prints. It is **16 entries
max** (`MAX_BG_EVENTS`), and it sits in the save right beside the warp list, in the **same ROM
block, loaded by the same routine, under the same linchpin** — so an edited sign is live on
Continue exactly as an edited warp is. Unlike warps and sprites, **our model of it is already
correct** (no wrong names, no dead bytes, no hazards). What was missing is the *text*: the
save holds a **text id**, and until now our data held only the id — not the words. Those words live
in `pret/pokered`, one text table per map, and this is the note that says how to get them.

**The address maths (same as warps):** the Main Data block starts at file offset `0x25A3` =
`wMainDataStart` (`$D2F7`), so

> **`WRAM address = save offset + 0xAD54`**

---

## 1. The save layout — three arrays, verified against `wram.asm`

```
wNumSigns::   db                       ; how many signs on this map
wSignCoords:: ds MAX_BG_EVENTS * 2     ; Y, X   (note: Y first, like warps)
wSignTextIDs::ds MAX_BG_EVENTS         ; one text id per sign
```

`MAX_BG_EVENTS EQU 16` (`constants/map_data_constants.asm`). So:

| | Save | WRAM | What |
|---|---|---|---|
| **Count** | `0x275C` | `$D4B0` | `wNumSigns` — 0–16. |
| **Coords** | `0x275D` | `$D4B1` | `wSignCoords` — **16 × { Y, X }** = 32 bytes. |
| **Text ids** | `0x277D` | `$D4D1` | `wSignTextIDs` — **16 × { txtId }** = 16 bytes. |

Note the split: unlike a warp (whose 4 bytes are contiguous), a sign's **coords and text id live in
two different arrays**, indexed by the same sign number. `SignData::load/save` already reads them
that way (`(2*i)+0x275D` for Y,X; `(1*i)+0x277D` for the id). **Coords are Y then X**, matching the
`; Y, X` comment in the ROM and matching warps.

> ✅ **Our model is right.** `AreaSign` (count at `0x275C`, cap 16) and `SignData` (the two arrays)
> match the cartridge byte-for-byte. This is the rare research pass that found **no bug to fix first**
> — contrast the sprite pass (four) and the warp pass (seven). Phase 6 can build straight on the
> model; it does not need a "6a: make the model true".

---

## 2. THE LINCHPIN — an edited sign is live, and it is the warp mechanism exactly

`.loadSignData` is **inside `LoadMapHeader`** (`home/overworld.asm`), immediately after the warp
loop — it rebuilds the sign list **from ROM** on every map load:

```asm
.loadSignData
	ld a, [hli]           ; number of signs   <- ROM
	ld [wNumSigns], a
	and a
	jr z, .loadSpriteData
	ld c, a
	ld de, wSignTextIDs   ; <- ROM
	...
```

So on the face of it every sign byte an editor writes is erased the instant the map loads — **but
`LoadMapHeader` never reaches `.loadSignData` on a Continue**, because it opens with the same guard
that saves the warps ([`warps.md`](warps.md) §1):

```asm
	bit BIT_NO_PREVIOUS_MAP, b
	ret nz                ; <- RETURNS. No header. No warps. No SIGNS. No sprites.
```

and `LoadMainData` sets `BIT_NO_PREVIOUS_MAP` on the saved tileset byte as it reads the save. **One
guard protects warps, signs and sprites together** — they are loaded by one routine.

> ### The rule, stated the way the panel must state it
>
> **An edited sign is really there.** Load the save and the placard is where you put it, saying what
> you aimed it at.
>
> **And the game restores the map's original signs the moment the player leaves the map and walks
> back in** — by then `BIT_NO_PREVIOUS_MAP` is clear and the next load reads the header from ROM.
>
> Word-for-word the warp/sprite story. Shown plainly, never hidden, never silently "fixed." The same
> honest note the warp panel carries is owed here.

*(Not separately re-probed on the cartridge: it is the identical code path already verified for warps
by `scripts/emu/probe_warp_persistence.py` — same routine, same guard, one instruction later. If Phase
6 ever wants belt-and-braces, the probe is a two-line extension: tamper a sign, assert it survives.)*

---

## 3. What the text id IS — a 1-based index into the map's text table

A sign in the ROM is a **`bg_event`**:

```asm
	def_bg_events
	bg_event 13, 13, TEXT_PALLETTOWN_OAKSLAB_SIGN   ; x, y, text id
	bg_event  7,  9, TEXT_PALLETTOWN_SIGN
	...
```

The third argument is a **text id**, and it is a **1-based index into that map's
`def_text_pointers` table** (in `scripts/<Map>.asm`). Entry N resolves to a label, and the label
resolves — through `text_far` — to the actual string in `text/<Map>.asm`:

```asm
; scripts/PalletTown.asm
PalletTown_TextPointers:
	def_text_pointers
	dw_const PalletTownOakText,              TEXT_PALLETTOWN_OAK              ; 1
	dw_const PalletTownGirlText,             TEXT_PALLETTOWN_GIRL             ; 2
	dw_const PalletTownFisherText,           TEXT_PALLETTOWN_FISHER           ; 3
	dw_const PalletTownOaksLabSignText,      TEXT_PALLETTOWN_OAKSLAB_SIGN     ; 4
	dw_const PalletTownSignText,             TEXT_PALLETTOWN_SIGN             ; 5
	dw_const PalletTownPlayersHouseSignText, TEXT_PALLETTOWN_PLAYERSHOUSE_SIGN; 6
	dw_const PalletTownRivalsHouseSignText,  TEXT_PALLETTOWN_RIVALSHOUSE_SIGN ; 7

; text/PalletTown.asm
_PalletTownOaksLabSignText::
	text "OAK #MON"
	line "RESEARCH LAB"
	done
```

So Pallet Town sign #0 (`txtId = 4`) prints **"OAK #MON / RESEARCH LAB"**. This matches `maps.json`
exactly (its Oak's-lab sign carries `text: 4`). **`#` is the game's control char for "POKé"** —
`_PalletTownOaksLabSignText` reads *"OAK POKéMON RESEARCH LAB"* in game.

### 3a. Not every text id is a sign, and that is the point of the grouping

A map's text table is **shared** by everything on the map that talks:

| A text id can be referenced by… | Category |
|---|---|
| a **`bg_event`** | **Sign** — a placard, meant to be read. |
| an **`object_event`** (an NPC/sprite) | **Person** — dialogue. Often a `text_asm` *script*, not a plain string. |
| **nothing on this map** (or a hidden/script-only use) | **Other** — a text slot the map defines but no object points at. |

The console does **not** enforce which category a `wSignTextIDs` byte names — a sign will happily
print an NPC's dialogue if you point it there. So the picker (Phase 6d) offers **every text id the
map has**, and — per project leadership (2026-07-14) — **groups them** (Signs · People · Other), each showing
its real words where a plain string exists, and a `(scripted text)` marker where the entry is
`text_asm` and has no single literal.

### 3b. Hack ids — a full byte, and out-of-table means "no such line"

`txtId` is one byte (0–255). A map has only **N** text entries (Pallet Town: 7). An id of **0**, or
one **> N**, points at nothing the map defines — `DisplayTextID` runs off the end of the table into
whatever follows. Per the project rule: **shown, editable, never refused, never rewritten** — the
picker states plainly "this map has 7 text entries; id 9 points past them," the same way the warp
hazards say what they say.

---

## 4. What this means for OUR code

| | Today | What Phase 6 needs |
|---|---|---|
| **Save model** | ✅ `AreaSign` + `SignData` are byte-correct. | Nothing. Build on it. |
| **DB sign data** | `MapDBEntrySign` has `x`, `y`, `textID` — **but no words**, and no notion of the map's whole text table. | The map's **grouped text table** (id → string / script-marker → category), extracted from `pret/pokered` and shipped. |
| **The panel** | no sign UI at all. | The add-sign tool, the Signs canvas layer, and the Details panel with X/Y + the grouped text picker (Phase 6b–6d). |

**No model bug to fix first** — but the **honest "restored on re-entry" note** the warp panel earned
is owed to the sign panel too (§2).

### The extraction (Phase 6a)

A new self-validating importer (`scripts/import_sign_text.py`, precedent: `import_tile_traits.py`,
`import_sprites.py`) parses, per map:

1. `data/maps/objects/<Map>.asm` — the `bg_event` and `object_event` lines → **which text ids are
   signs, which are people** (by position in the file / the `TEXT_*` constant they name).
2. `scripts/<Map>.asm` — the `def_text_pointers` list → the **id → label** map and the map's text
   **count**.
3. `text/<Map>.asm` (and `text_far` targets) → the **label → actual string**, decoded through the
   game's text control codes (`#` → POKé, `line`/`para`/`cont`/`@`, etc. — reuse the font/text
   decoding the app already has).

Output rides in `maps.json` (the standing file-format rule: extend the map entries we already ship,
don't invent a parallel file) as a per-map **text table** — each entry `{ string, category }`,
indexed 1..N — so `MapDBEntry` can hand QML a grouped list. Self-validating: re-derive the count from
`wNumSigns`-shaped data, assert every `maps.json` sign's `text` id lands inside the map's table.

---

## 5. Sources

| What | Where |
|---|---|
| `wNumSigns` / `wSignCoords` / `wSignTextIDs`, `MAX_BG_EVENTS` | `ram/wram.asm`, `constants/map_data_constants.asm` |
| `.loadSignData` (inside `LoadMapHeader`, behind `BIT_NO_PREVIOUS_MAP`) | `home/overworld.asm` |
| `bg_event` / `object_event` / `def_bg_events` macros | `macros/scripts/maps.asm`, `data/maps/objects/*.asm` |
| `def_text_pointers`, the id → label tables | `scripts/*.asm` |
| The actual strings | `text/*.asm` |
| The persistence linchpin (already cartridge-verified for the shared routine) | [`warps.md`](warps.md) §1, `scripts/emu/probe_warp_persistence.py` |
